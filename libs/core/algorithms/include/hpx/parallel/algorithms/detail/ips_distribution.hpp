//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Blockwise in-place distribution engine shared by IPS^2Ra (radix sort) and
// IPS^4o (samplesort), after Axtmann, Witt, Ferizovic and Sanders,
// "Engineering In-place (Shared-memory) Sorting Algorithms", ACM TOPC 2022.
//
// Given a range of n elements and a classifier mapping each element to one of
// K buckets, ips_distribute reorders the range so that bucket k occupies
// [bucket_start[k], bucket_start[k + 1]), using O(K * B) auxiliary memory
// where B is the block size. The phases are:
//
//   1. Local classification: elements are moved into per-bucket buffers of B
//      elements; every full buffer is flushed back to the front of the range
//      as a homogeneous block. Afterwards [0, W) holds full blocks and the
//      buffers hold fewer than B elements per bucket.
//   2. Prefix sums: bucket sizes become bucket boundaries. Each bucket is
//      assigned the block-aligned region [align_up(start), align_up(end)).
//   3. Block permutation: every block is swapped into the aligned region of
//      its bucket. Per bucket, a write pointer w marks the end of correctly
//      placed blocks and a read pointer r marks the last unprocessed block.
//   4. Cleanup: aligned regions do not coincide with exact bucket boundaries,
//      so the partial head and tail of every bucket are filled from the
//      bucket buffer and from blocks that spilled into the next bucket.
//
// ips_distribute is the sequential version. ips_distribute_parallel runs the
// same phases on p workers: each worker classifies its own stripe, empty
// blocks are compacted out of every bucket region, blocks are permuted using
// atomic per-bucket read/write pointers, and the cleanup is split into two
// passes over the buckets so that it needs no ordering between them.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    // Default block size: 2 KiB worth of elements, as in the reference
    // implementation, but at least one element.
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr std::size_t ips_default_block_size =
        sizeof(T) >= 2048 ? 1 : 2048 / sizeof(T);

    // Auxiliary state for one (sequential) distribution. It is allocated once
    // and reused across recursion levels; every buffer is empty between calls
    // to ips_distribute.
    //
    // Buffers are vectors with reserved capacity rather than raw arrays so that
    // the element type only needs to be move constructible and move
    // assignable.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct ips_distribution_buffers
    {
        ips_distribution_buffers(std::size_t num_buckets, std::size_t block)
          : block_size(block)
          , buckets(num_buckets)
          , bucket_start(num_buckets + 1)
          , write(num_buckets)
          , read(num_buckets)
        {
            HPX_ASSERT(block_size != 0);
            for (auto& b : buckets)
                b.reserve(block_size);
            swap[0].reserve(block_size);
            swap[1].reserve(block_size);
            overflow.reserve(block_size);
        }

        std::size_t num_buckets() const noexcept
        {
            return buckets.size();
        }

        std::size_t block_size;

        // Partial blocks produced by local classification, one per bucket.
        std::vector<std::vector<T>> buckets;

        // Carry buffers for block permutation.
        std::vector<T> swap[2];

        // Receives the single block whose aligned slot extends past the end of
        // the range, if any.
        std::vector<T> overflow;

        // Output: bucket boundaries relative to the start of the range.
        std::vector<std::size_t> bucket_start;

        // Block permutation pointers, relative to the start of the range.
        // read[k] < write[k] means bucket k has no unprocessed blocks left.
        std::vector<std::size_t> write;
        std::vector<std::ptrdiff_t> read;
    };

    namespace ips {

        template <typename T, typename RandomIt>
        void load_block(std::vector<T>& buf, RandomIt src, std::size_t block)
        {
            buf.clear();
            std::move(src, src + static_cast<std::ptrdiff_t>(block),
                std::back_inserter(buf));
        }

        template <typename T, typename RandomIt>
        void store_block(std::vector<T>& buf, RandomIt dest)
        {
            std::move(buf.begin(), buf.end(), dest);
            buf.clear();
        }

        constexpr std::size_t align_up(std::size_t v, std::size_t b) noexcept
        {
            return (v + b - 1) / b * b;
        }

        ///////////////////////////////////////////////////////////////////////
        // Phase 1: local classification. Returns W, the end of the full-block
        // prefix. Bucket sizes are accumulated into buf.bucket_start[0..K).
        template <typename RandomIt, typename T, typename Classifier>
        std::size_t classify_locally(RandomIt first, std::size_t n,
            Classifier& classify, ips_distribution_buffers<T>& buf)
        {
            std::size_t const block = buf.block_size;
            std::fill(buf.bucket_start.begin(), buf.bucket_start.end(), 0);

            std::size_t written = 0;
            for (std::size_t i = 0; i != n; ++i)
            {
                auto it = first + static_cast<std::ptrdiff_t>(i);
                std::size_t const b = classify(*it);
                HPX_ASSERT(b < buf.num_buckets());

                ++buf.bucket_start[b];
                auto& bucket = buf.buckets[b];
                bucket.push_back(HPX_MOVE(*it));

                // We have read i + 1 elements and written `written`, and the
                // buffers hold the difference, so written + block <= i + 1:
                // the flush only overwrites slots that were already consumed.
                if (bucket.size() == block)
                {
                    store_block(
                        bucket, first + static_cast<std::ptrdiff_t>(written));
                    written += block;
                }
            }
            return written;
        }

        ///////////////////////////////////////////////////////////////////////
        // Phase 2: exclusive prefix sum over the bucket sizes, and initial
        // read/write pointers for block permutation.
        template <typename T>
        void compute_boundaries(
            std::size_t full_end, ips_distribution_buffers<T>& buf)
        {
            std::size_t const block = buf.block_size;
            std::size_t const k = buf.num_buckets();

            std::size_t sum = 0;
            for (std::size_t b = 0; b != k; ++b)
            {
                std::size_t const cnt = buf.bucket_start[b];
                buf.bucket_start[b] = sum;
                sum += cnt;
            }
            buf.bucket_start[k] = sum;

            for (std::size_t b = 0; b != k; ++b)
            {
                std::size_t const astart = align_up(buf.bucket_start[b], block);
                std::size_t const aend =
                    align_up(buf.bucket_start[b + 1], block);

                // The last full block inside this bucket's aligned region, if
                // any; blocks only exist in [0, full_end).
                buf.write[b] = astart;
                buf.read[b] =
                    static_cast<std::ptrdiff_t>((std::min) (aend, full_end)) -
                    static_cast<std::ptrdiff_t>(block);
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // Phase 3: block permutation.
        //
        // Pick up the last unprocessed block of a bucket and carry it to the
        // write pointer of its destination bucket. If that slot holds a block
        // that already belongs there, skip it; if it holds an unprocessed block
        // of another bucket, swap and continue carrying the evicted block; if
        // the slot is free, drop the block and pick up the next one.
        template <typename RandomIt, typename T, typename Classifier>
        void permute_blocks(RandomIt first, std::size_t n, Classifier& classify,
            ips_distribution_buffers<T>& buf)
        {
            std::size_t const block = buf.block_size;
            std::size_t const k = buf.num_buckets();
            auto at = [first](std::size_t pos) {
                return first + static_cast<std::ptrdiff_t>(pos);
            };

            for (std::size_t src = 0; src != k; ++src)
            {
                while (buf.read[src] >=
                    static_cast<std::ptrdiff_t>(buf.write[src]))
                {
                    std::size_t const r =
                        static_cast<std::size_t>(buf.read[src]);
                    buf.read[src] -= static_cast<std::ptrdiff_t>(block);

                    int cur = 0;
                    load_block(buf.swap[cur], at(r), block);
                    std::size_t dest = classify(buf.swap[cur].front());

                    for (;;)
                    {
                        std::size_t const w = buf.write[dest];
                        buf.write[dest] += block;

                        if (static_cast<std::ptrdiff_t>(w) > buf.read[dest])
                        {
                            // Free slot. Only the slot straddling the end of
                            // the range can extend past n, and at most one
                            // block is ever written there.
                            if (w + block > n)
                            {
                                HPX_ASSERT(buf.overflow.empty());
                                buf.overflow.swap(buf.swap[cur]);
                            }
                            else
                            {
                                store_block(buf.swap[cur], at(w));
                            }
                            break;
                        }

                        std::size_t const evicted = classify(*at(w));
                        if (evicted == dest)
                            continue;    // already in place

                        load_block(buf.swap[1 - cur], at(w), block);
                        store_block(buf.swap[cur], at(w));
                        cur = 1 - cur;
                        dest = evicted;
                    }
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // Phase 4: cleanup.
        //
        // After permutation, the blocks of bucket b cover [astart, w) where
        // astart = align_up(start). Relative to the exact range [start, end):
        //  - the head [start, astart) is free (it lies in the aligned region of
        //    bucket b - 1, whose spill-over has already been moved out because
        //    buckets are processed in order),
        //  - the tail [w, end) is free if w < end,
        //  - otherwise the last block spilled w - end elements into the head of
        //    bucket b + 1 (or into the overflow buffer if w > n).
        // The free slots are filled from the spilled elements and from the
        // bucket buffer; the counts always match.
        template <typename RandomIt, typename T>
        void cleanup(
            RandomIt first, std::size_t n, ips_distribution_buffers<T>& buf)
        {
            std::size_t const block = buf.block_size;
            std::size_t const k = buf.num_buckets();
            auto at = [first](std::size_t pos) {
                return first + static_cast<std::ptrdiff_t>(pos);
            };

            for (std::size_t b = 0; b != k; ++b)
            {
                std::size_t const start = buf.bucket_start[b];
                std::size_t const end = buf.bucket_start[b + 1];
                std::size_t const astart = align_up(start, block);
                std::size_t const w = buf.write[b];
                bool const has_blocks = w > astart;

                // The overflow block belongs at [w - block, w); put the part
                // that fits into the range there. Those slots are past the
                // full-block prefix and hence free.
                std::size_t overflow_pos = 0;
                if (has_blocks && w > n)
                {
                    HPX_ASSERT(buf.overflow.size() == block);
                    overflow_pos = n - (w - block);
                    std::move(buf.overflow.begin(),
                        buf.overflow.begin() +
                            static_cast<std::ptrdiff_t>(overflow_pos),
                        at(w - block));
                }

                // Free slots in [start, end), in order: the head, then the
                // tail after the blocks.
                std::size_t fill = start;
                std::size_t const head_end = has_blocks ? astart : end;
                auto put = [&](T&& v) {
                    // Named outside HPX_ASSERT as well: contract assertions do
                    // not capture variables implicitly.
                    HPX_UNUSED(end);
                    if (fill == head_end && has_blocks)
                        fill = w;
                    HPX_ASSERT(fill < end);
                    *at(fill++) = HPX_MOVE(v);
                };

                if (has_blocks && w > end)
                {
                    for (std::size_t i = end; i < (std::min) (w, n); ++i)
                        put(HPX_MOVE(*at(i)));
                    if (w > n)
                    {
                        for (std::size_t i = overflow_pos; i != block; ++i)
                            put(HPX_MOVE(buf.overflow[i]));
                        buf.overflow.clear();
                    }
                }

                for (auto& v : buf.buckets[b])
                    put(HPX_MOVE(v));
                buf.buckets[b].clear();

                // Either the free slots end at the bucket end, or the blocks
                // reach past it and only the head needed filling.
                HPX_ASSERT(fill == ((has_blocks && w >= end) ? astart : end));
            }
        }
    }    // namespace ips

    ///////////////////////////////////////////////////////////////////////////
    // Distribute [first, first + n) into buf.num_buckets() buckets according to
    // `classify`, which maps an element to a bucket index. On return,
    // buf.bucket_start[k] is the offset of bucket k and buf.bucket_start[K]
    // equals n. The order within a bucket is unspecified.
    //
    // Exception safety: if `classify` throws, elements held in the buffers are
    // lost. Classifiers used by the sorting algorithms are non-throwing.
    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename T,
        typename Classifier>
    void ips_distribute(RandomIt first, std::size_t n, Classifier&& classify,
        ips_distribution_buffers<T>& buf)
    {
        std::size_t const full_end =
            ips::classify_locally(first, n, classify, buf);

        // Bucket sizes are in bucket_start[0..K) at this point. If one bucket
        // received everything, the blocks are already in place.
        std::size_t single = buf.num_buckets();
        for (std::size_t b = 0; b != buf.num_buckets(); ++b)
        {
            if (buf.bucket_start[b] == n)
            {
                single = b;
                break;
            }
        }

        ips::compute_boundaries(full_end, buf);

        if (single != buf.num_buckets())
            buf.write[single] = full_end;
        else
            ips::permute_blocks(first, n, classify, buf);

        ips::cleanup(first, n, buf);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parallel distribution.

    // Read and write pointer of one bucket during parallel block permutation.
    // Both live in one atomic word so that they can be read and updated
    // together: the upper 32 bits hold the write block index, the lower 32
    // bits hold the read block index plus one (so that "no block left" is
    // representable). Block indices are relative to the start of the range.
    //
    // `reading` counts threads that may be copying a block out of this
    // bucket. A thread that is about to write into a slot that was just read
    // waits for it to drop to zero, so it cannot overwrite a block that is
    // still being copied.
    HPX_CXX_CORE_EXPORT struct ips_bucket_pointers
    {
        static constexpr std::uint64_t read_mask = 0xffff'ffffull;

        void set(std::uint64_t write_blk, std::uint64_t read_end_blk) noexcept
        {
            HPX_ASSERT(write_blk <= read_mask && read_end_blk <= read_mask);
            wr.store(
                (write_blk << 32) | read_end_blk, std::memory_order_relaxed);
            reading.store(0, std::memory_order_relaxed);
        }

        // Claim the last unprocessed block. On success, `block` is its index
        // and the caller must call done_reading() once it has copied it.
        bool try_claim_read(std::uint64_t& block) noexcept
        {
            reading.fetch_add(1, std::memory_order_acq_rel);
            std::uint64_t v = wr.load(std::memory_order_acquire);
            for (;;)
            {
                std::uint64_t const end_blk = v & read_mask;
                if (end_blk <= (v >> 32))
                {
                    reading.fetch_sub(1, std::memory_order_release);
                    return false;
                }
                if (wr.compare_exchange_weak(v, v - 1,
                        std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    block = end_blk - 1;
                    return true;
                }
            }
        }

        void done_reading() noexcept
        {
            reading.fetch_sub(1, std::memory_order_release);
        }

        // Claim the slot at the write pointer. Returns its block index and
        // whether it is free (already read, or never held a full block). If
        // it is not free it holds an unprocessed block that now belongs to the
        // caller.
        std::pair<std::uint64_t, bool> claim_write() noexcept
        {
            std::uint64_t const v =
                wr.fetch_add(std::uint64_t(1) << 32, std::memory_order_acq_rel);
            std::uint64_t const w = v >> 32;
            return {w, w >= (v & read_mask)};
        }

        void wait_for_readers() const noexcept
        {
            for (int k = 0; reading.load(std::memory_order_acquire) != 0; ++k)
            {
                if (k >= 64)
                    std::this_thread::yield();
            }
        }

        std::uint64_t write_block() const noexcept
        {
            return wr.load(std::memory_order_relaxed) >> 32;
        }

        std::uint64_t read_end() const noexcept
        {
            return wr.load(std::memory_order_relaxed) & read_mask;
        }

        std::atomic<std::uint64_t> wr{0};
        std::atomic<std::uint32_t> reading{0};
    };

    // Auxiliary state for one parallel distribution with a fixed number of
    // workers: per-worker classification buffers, plus shared bucket
    // pointers and cleanup storage. Memory is O(p * K * B).
    HPX_CXX_CORE_EXPORT template <typename T>
    struct ips_parallel_distribution_buffers
    {
        ips_parallel_distribution_buffers(
            std::size_t num_threads, std::size_t num_buckets, std::size_t block)
          : block_size(block)
          , spill(num_buckets)
          , bucket_start(num_buckets + 1)
          , write(num_buckets)
          , stripe_begin(num_threads + 1)
          , stripe_full_end(num_threads)
          , pointers(new hpx::util::cache_aligned_data<
                ips_bucket_pointers>[num_buckets])
        {
            HPX_ASSERT(num_threads != 0);
            workers.reserve(num_threads);
            for (std::size_t i = 0; i != num_threads; ++i)
                workers.emplace_back(num_buckets, block);
            for (auto& s : spill)
                s.reserve(block);
            overflow.reserve(block);
        }

        std::size_t num_threads() const noexcept
        {
            return workers.size();
        }

        std::size_t num_buckets() const noexcept
        {
            return spill.size();
        }

        ips_bucket_pointers& bucket_pointers(std::size_t b) noexcept
        {
            return pointers[b].data_;
        }

        std::size_t block_size;

        // Per-worker classification buffers, carry buffers and bucket counts
        // (in bucket_start[0..K)).
        std::vector<ips_distribution_buffers<T>> workers;

        // Elements of bucket b that the permutation placed into the head of
        // bucket b + 1, collected during the first cleanup pass.
        std::vector<std::vector<T>> spill;

        std::vector<T> overflow;

        // Output: bucket boundaries relative to the start of the range.
        std::vector<std::size_t> bucket_start;

        // Final write pointer of every bucket, in elements.
        std::vector<std::size_t> write;

        // Worker i classifies [stripe_begin[i], stripe_begin[i + 1]) and
        // leaves full blocks in [stripe_begin[i], stripe_full_end[i]).
        std::vector<std::size_t> stripe_begin;
        std::vector<std::size_t> stripe_full_end;

        std::unique_ptr<hpx::util::cache_aligned_data<ips_bucket_pointers>[]>
            pointers;
    };

    namespace ips {

        // Gather the spilled elements of bucket b (see cleanup()) into
        // spill[b]. Touches only slots at or past the end of bucket b that
        // its own blocks occupy, so buckets can be processed concurrently.
        template <typename RandomIt, typename T>
        void collect_spill(RandomIt first, std::size_t n, std::size_t b,
            ips_parallel_distribution_buffers<T>& buf)
        {
            std::size_t const block = buf.block_size;
            std::size_t const end = buf.bucket_start[b + 1];
            std::size_t const astart = align_up(buf.bucket_start[b], block);
            std::size_t const w = buf.write[b];
            if (w <= astart || w <= end)
                return;

            auto at = [first](std::size_t pos) {
                return first + static_cast<std::ptrdiff_t>(pos);
            };
            auto& spill = buf.spill[b];

            if (w <= n)
            {
                std::move(at(end), at(w), std::back_inserter(spill));
                return;
            }

            // The last block went to the overflow buffer. Its part below n
            // belongs at [w - block, n); [w - block, end) of that is part of
            // this bucket, the rest spills.
            HPX_ASSERT(buf.overflow.size() == block);
            std::size_t const own = end - (w - block);
            HPX_ASSERT(own <= n - (w - block));
            std::move(buf.overflow.begin(),
                buf.overflow.begin() + static_cast<std::ptrdiff_t>(own),
                at(w - block));
            std::move(buf.overflow.begin() + static_cast<std::ptrdiff_t>(own),
                buf.overflow.end(), std::back_inserter(spill));
            buf.overflow.clear();
        }

        // Fill the free slots of bucket b from spill[b] and from every
        // worker's buffer for b. Touches only [start, end) of bucket b.
        template <typename RandomIt, typename T>
        void fill_bucket(RandomIt first, std::size_t b,
            ips_parallel_distribution_buffers<T>& buf)
        {
            std::size_t const block = buf.block_size;
            std::size_t const start = buf.bucket_start[b];
            std::size_t const end = buf.bucket_start[b + 1];
            std::size_t const astart = align_up(start, block);
            std::size_t const w = buf.write[b];
            bool const has_blocks = w > astart;

            std::size_t fill = start;
            std::size_t const head_end = has_blocks ? astart : end;
            auto put = [&](T&& v) {
                // Named outside HPX_ASSERT as well: contract assertions do
                // not capture variables implicitly.
                HPX_UNUSED(end);
                if (fill == head_end && has_blocks)
                    fill = w;
                HPX_ASSERT(fill < end);
                *(first + static_cast<std::ptrdiff_t>(fill++)) = HPX_MOVE(v);
            };

            for (auto& v : buf.spill[b])
                put(HPX_MOVE(v));
            buf.spill[b].clear();

            for (auto& worker : buf.workers)
            {
                for (auto& v : worker.buckets[b])
                    put(HPX_MOVE(v));
                worker.buckets[b].clear();
            }

            HPX_ASSERT(fill == ((has_blocks && w >= end) ? astart : end));
        }
    }    // namespace ips

    ///////////////////////////////////////////////////////////////////////////
    // Parallel counterpart of ips_distribute, using buf.num_threads() workers.
    //
    // `run(count, f)` must invoke f(0), ..., f(count - 1), possibly
    // concurrently, and return once all of them have finished; every phase is
    // one such call. `classify` is invoked concurrently and must be
    // thread-safe.
    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename T,
        typename Classifier, typename Run>
    void ips_distribute_parallel(RandomIt first, std::size_t n,
        Classifier&& classify, ips_parallel_distribution_buffers<T>& buf,
        Run&& run)
    {
        std::size_t const p = buf.num_threads();
        std::size_t const k = buf.num_buckets();
        std::size_t const block = buf.block_size;
        auto at = [first](std::size_t pos) {
            return first + static_cast<std::ptrdiff_t>(pos);
        };

        // Block indices, plus one, must fit the 32 bits of the packed bucket
        // pointers.
        HPX_ASSERT(n / block + 1 < ips_bucket_pointers::read_mask);

        // Stripes consist of whole blocks so that the full blocks each worker
        // writes are block aligned. The last stripe takes the remainder.
        std::size_t const num_blocks = n / block;
        for (std::size_t i = 0; i != p; ++i)
        {
            std::size_t const blocks =
                num_blocks / p * i + (std::min) (i, num_blocks % p);
            buf.stripe_begin[i] = blocks * block;
        }
        buf.stripe_begin[p] = n;

        // Phase 1: stripe-local classification.
        run(p, [&](std::size_t i) {
            std::size_t const begin = buf.stripe_begin[i];
            buf.stripe_full_end[i] = begin +
                ips::classify_locally(at(begin),
                    buf.stripe_begin[i + 1] - begin, classify, buf.workers[i]);
        });

        // Phase 2: global bucket boundaries.
        std::size_t single = k;
        std::size_t sum = 0;
        for (std::size_t b = 0; b != k; ++b)
        {
            std::size_t cnt = 0;
            for (auto const& worker : buf.workers)
                cnt += worker.bucket_start[b];
            if (cnt == n)
                single = b;
            buf.bucket_start[b] = sum;
            sum += cnt;
        }
        buf.bucket_start[k] = sum;
        HPX_ASSERT(sum == n);

        // Phase 3: within every bucket's aligned region, move full blocks to
        // the front so that [write, read] holds exactly the unprocessed
        // blocks. Regions are disjoint, so buckets are independent.
        auto is_full = [&](std::size_t blk) {
            std::size_t const pos = blk * block;
            if (pos >= n)
                return false;
            auto const it = std::upper_bound(
                buf.stripe_begin.begin(), buf.stripe_begin.end() - 1, pos);
            std::size_t const s =
                static_cast<std::size_t>(it - buf.stripe_begin.begin()) - 1;
            return pos < buf.stripe_full_end[s];
        };

        run(p, [&](std::size_t i) {
            for (std::size_t b = i; b < k; b += p)
            {
                std::size_t const first_blk =
                    ips::align_up(buf.bucket_start[b], block) / block;
                std::size_t const end_blk =
                    ips::align_up(buf.bucket_start[b + 1], block) / block;

                std::size_t lo = first_blk;
                std::size_t hi = end_blk;
                std::size_t full = 0;
                for (std::size_t blk = first_blk; blk != end_blk; ++blk)
                    full += is_full(blk) ? 1 : 0;

                for (;;)
                {
                    while (lo < hi && is_full(lo))
                        ++lo;
                    while (lo < hi && !is_full(hi - 1))
                        --hi;
                    if (lo >= hi)
                        break;
                    // lo is empty and hi - 1 is full; once moved, hi - 1 is
                    // outside [lo, hi) and is never looked at again.
                    std::move(
                        at((hi - 1) * block), at(hi * block), at(lo * block));
                    ++lo;
                    --hi;
                }

                buf.bucket_pointers(b).set(first_blk, first_blk + full);
            }
        });

        // Phase 4: block permutation. Worker i starts at bucket i * K / p and
        // then visits every other bucket, so contention is spread out.
        if (single == k)
        {
            run(p, [&](std::size_t i) {
                auto& swap = buf.workers[i].swap;
                std::size_t const start_bucket = i * k / p;

                for (std::size_t step = 0; step != k; ++step)
                {
                    std::size_t const src = (start_bucket + step) % k;
                    std::uint64_t r = 0;
                    while (buf.bucket_pointers(src).try_claim_read(r))
                    {
                        int cur = 0;
                        ips::load_block(swap[cur], at(r * block), block);
                        buf.bucket_pointers(src).done_reading();
                        std::size_t dest = classify(swap[cur].front());

                        for (;;)
                        {
                            auto& ptrs = buf.bucket_pointers(dest);
                            auto const [w, is_free] = ptrs.claim_write();
                            std::size_t const pos = w * block;

                            if (is_free)
                            {
                                ptrs.wait_for_readers();
                                if (pos + block > n)
                                {
                                    HPX_ASSERT(buf.overflow.empty());
                                    buf.overflow.swap(swap[cur]);
                                }
                                else
                                {
                                    ips::store_block(swap[cur], at(pos));
                                }
                                break;
                            }

                            std::size_t const evicted = classify(*at(pos));
                            if (evicted == dest)
                                continue;    // already in place

                            ips::load_block(swap[1 - cur], at(pos), block);
                            ips::store_block(swap[cur], at(pos));
                            cur = 1 - cur;
                            dest = evicted;
                        }
                    }
                }
            });

            for (std::size_t b = 0; b != k; ++b)
                buf.write[b] = buf.bucket_pointers(b).write_block() * block;
        }
        else
        {
            // Everything is in one bucket and compaction already put its
            // blocks in place.
            for (std::size_t b = 0; b != k; ++b)
            {
                auto const& ptrs = buf.bucket_pointers(b);
                buf.write[b] =
                    (b == single ? ptrs.read_end() : ptrs.write_block()) *
                    block;
            }
        }

        // Phase 5: cleanup, as two passes so that buckets are independent
        // within each pass.
        run(p, [&](std::size_t i) {
            for (std::size_t b = i; b < k; b += p)
                ips::collect_spill(first, n, b, buf);
        });
        run(p, [&](std::size_t i) {
            for (std::size_t b = i; b < k; b += p)
                ips::fill_bucket(first, b, buf);
        });
    }
}    // namespace hpx::parallel::detail
