//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#include <shmem.h>

namespace hpx::parcelset::policies::openshmem {

    // TX/RX page-arena + per-pair 64-bit credit-word protocol
    //
    namespace detail {

        // Wire header of a chunk, shared by sender_connection (writer) and
        // receiver_connection (reader). Each page carries the full header.
        struct message_header
        {
            std::uint64_t size;
            std::uint64_t data_size;
            std::uint32_t num_chunks;
            std::uint32_t chunk_index;
            std::uint32_t total_size_low;
            std::uint32_t total_size_high;
            std::uint64_t message_id;
            std::uint32_t checksum;
        };

        static_assert(sizeof(message_header) % 8 == 0,
            "message_header must be 8-byte aligned");

        constexpr std::size_t header_size = sizeof(message_header);

        // Bounds a corrupt num_chunks field so downstream chunk-vector
        // resizes stay sane during validation.
        constexpr std::size_t max_message_chunks = 1u << 20;

        // checksum over the payload bytes only.
        constexpr std::array<std::uint32_t, 256> make_crc32_table()
        {
            std::array<std::uint32_t, 256> table{};
            for (std::size_t i = 0; i < table.size(); ++i)
            {
                std::uint32_t c = static_cast<std::uint32_t>(i);
                for (int k = 0; k < 8; ++k)
                {
                    c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                }
                table[i] = c;
            }
            return table;
        }

        inline std::uint32_t crc32(
            unsigned char const* data, std::size_t size) noexcept
        {
            static constexpr std::array<std::uint32_t, 256> table =
                make_crc32_table();
            std::uint32_t crc = 0xFFFFFFFFu;
            for (std::size_t i = 0; i < size; ++i)
            {
                crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
            }
            return crc ^ 0xFFFFFFFFu;
        }

        inline std::uint64_t message_total_size(message_header const& h) noexcept
        {
            return (std::uint64_t(h.total_size_high) << 32) | h.total_size_low;
        }

        // Payload size (bytes) of the chunk at 'chunk_index' of a message of
        // 'total_size' bytes split into 'num_chunks' chunks of at most
        inline std::size_t chunk_size_for(std::uint64_t chunk_index,
            std::uint64_t num_chunks, std::uint64_t total_size,
            std::size_t payload_size) noexcept
        {
            std::uint64_t const offset = chunk_index * payload_size;
            return static_cast<std::size_t>((chunk_index == num_chunks - 1) ?
                    (total_size - offset) :
                    payload_size);
        }

        // Structural sanity of one page header.
        inline bool message_header_geometry_valid(
            message_header const& h, std::size_t mtu) noexcept
        {
            if (header_size >= mtu)
                return false;
            std::size_t const payload_size = mtu - header_size;
            if (payload_size == 0)
                return false;

            if (h.message_id == 0)
                return false;
            if (h.num_chunks == 0 || h.num_chunks > max_message_chunks)
                return false;
            if (h.chunk_index >= h.num_chunks)
                return false;

            std::uint64_t const total_size = message_total_size(h);
            if (h.size != total_size || h.data_size != total_size)
                return false;
            if (total_size > max_message_chunks * (std::uint64_t) payload_size)
                return false;

            std::uint64_t const offset =
                std::uint64_t(h.chunk_index) * payload_size;
            if (offset > total_size)
                return false;

            if (chunk_size_for(
                    h.chunk_index, h.num_chunks, total_size, payload_size) >
                payload_size)
                return false;

            std::uint64_t const expected_chunks =
                (total_size + payload_size - 1) / payload_size;
            if (h.num_chunks != (total_size == 0 ? 1 : expected_chunks))
                return false;

            return true;
        }

        // Extract + fully validate a received page (geometry + payload
        // checksum).
        inline bool validate_message_page(unsigned char const* page,
            std::size_t mtu, std::size_t& chunk_size,
            bool use_checksum = true) noexcept
        {
            message_header header;
            std::memcpy(&header, page, sizeof(header));

            if (!message_header_geometry_valid(header, mtu))
                return false;

            std::size_t const payload_size = mtu - header_size;
            std::uint64_t const total_size = message_total_size(header);
            chunk_size = chunk_size_for(header.chunk_index, header.num_chunks,
                total_size, payload_size);

            return !use_checksum || header.checksum ==
                crc32(page + header_size, chunk_size);
        }
    }    // namespace detail

    class HPX_EXPORT mailbox_array
    {
    public:
        // Default number of in-flight pages (chunks) allowed per
        // (sender,receiver) pair before the sender must wait for consumed slots
        static constexpr std::size_t default_slots_per_dst = 8;

        // Max number of source PEs probed per receive-scan pass. The scan
        // window slides over the arena (one source per call)
        static constexpr std::size_t default_probe_window = 8;

        // Expected size (bytes) of the OpenSHMEM symmetric data segment the
        // job was launched with.
        static constexpr std::size_t default_symmetric_memory_size = 1u << 30;

        // Per-PE symmetric-memory footprint in bytes for a given geometry
        // (identical on every PE by construction): two O(npes*slots*mtu)
        // tx/rx arenas, two O(npes^2) credit matrices, plus one O(npes)
        // fingerprint row that the parcelport's startup uniformity check
        // publishes into (see config_words()).  The fingerprint row lives in
        // this allocation rather than a separate shmem_malloc() because that
        // call is collective and do_run() is not reached by every PE in
        // lockstep (rank 0 runs in runtime_mode::console, the others in
        // runtime_mode::worker).
        static constexpr std::size_t symmetric_bytes(
            std::size_t npes, std::size_t slots, std::size_t mtu) noexcept
        {
            return 2 * npes * slots * mtu +
                (2 * npes * npes + npes) * sizeof(std::uint32_t);
        }

        mailbox_array() = default;

        mailbox_array(std::size_t num_pes, std::size_t my_pe,
            std::size_t mtu, std::size_t slots_per_dst,
            std::size_t symmetric_memory_size = default_symmetric_memory_size,
            bool checksum = true,
            std::size_t probe_window = default_probe_window)
          : num_pes_(num_pes)
          , my_pe_(my_pe)
          , mtu_(mtu)
          , slots_per_dst_(slots_per_dst)
          , symmetric_memory_size_(symmetric_memory_size)
          , checksum_(checksum)
          , probe_window_(probe_window)
        {
            HPX_ASSERT(slots_per_dst_ > 0);

            std::size_t const pages = num_pes_ * slots_per_dst_ * mtu_;

            // one zeroed allocation; zeroed produced/consumed = correct
            // monotonic start state (0 pages produced/consumed).
            std::size_t const bytes =
                symmetric_bytes(num_pes_, slots_per_dst_, mtu_);

            // Advisory warning (PE 0 only) when the mapping grows large
            // relative to the symmetric segment the runtime was actually
            if (my_pe_ == 0 && symmetric_memory_size_ > 0 &&
                bytes > symmetric_memory_size_ / 2)
            {
                std::fprintf(stderr,
                    "openshmem: warning: symmetric memory mapping is %zu "
                    "bytes (2 * npes * slots * mtu + (2 * npes^2 + npes) * 4 "
                    "with "
                    "npes=%zu slots=%zu mtu=%zu), more than half of the "
                    "configured symmetric segment (%zu bytes).  Lower "
                    "hpx.parcel.openshmem.slots (build-time "
                    "HPX_WITH_PARCELPORT_OPENSHMEM_SLOTS), run on fewer PEs, "
                    "or raise the symmetric heap at launch (e.g. oshrun -x "
                    "SMA_SYMMETRIC_SIZE=8G).\n",
                    bytes, num_pes_, slots_per_dst_, mtu_,
                    symmetric_memory_size_);
            }

            heap_ = shmem_calloc(bytes, 1);
            if (!heap_)
            {
                // Report a catchable failure so the parcelport (and the
                // application) can shut down gracefully
                HPX_THROW_EXCEPTION(hpx::error::out_of_memory,
                    "openshmem::mailbox_array::mailbox_array",
                    "shmem_calloc(" + std::to_string(bytes) +
                        ") failed on PE " + std::to_string(my_pe_));
            }

            unsigned char* base = static_cast<unsigned char*>(heap_);
            tx_beg_ = base;
            tx_end_ = base + pages;
            rx_beg_ = tx_end_;
            rx_end_ = rx_beg_ + pages;
            produced_beg_ = reinterpret_cast<std::uint32_t*>(rx_end_);
            produced_end_ = produced_beg_ + num_pes_ * num_pes_;
            consumed_beg_ = produced_end_;
            consumed_end_ = consumed_beg_ + num_pes_ * num_pes_;
            config_beg_ = reinterpret_cast<std::uint32_t*>(consumed_end_);
            config_end_ = config_beg_ + num_pes_;

            // Publish the zeroed state to all PEs before any peer drives
            // progress against us.
            shmem_barrier_all();

            // Local-only mirror counters (never symmetric, never remote).
            // produced_locals_[dst] = how many pages I have published to dst
            produced_locals_ = new std::atomic<std::uint32_t>[num_pes_];
            consumed_locals_ = new std::atomic<std::uint32_t>[num_pes_];
            scan_rotate_ = new std::atomic<std::uint32_t>[num_pes_];
            for (std::size_t i = 0; i < num_pes_; ++i)
            {
                produced_locals_[i].store(0, std::memory_order_relaxed);
                consumed_locals_[i].store(0, std::memory_order_relaxed);
                scan_rotate_[i].store(0, std::memory_order_relaxed);
            }
        }

        ~mailbox_array()
        {
            delete[] produced_locals_;
            delete[] consumed_locals_;
            delete[] scan_rotate_;
            if (heap_)
            {
                shmem_free(heap_);
            }
        }

        mailbox_array(mailbox_array const&) = delete;
        mailbox_array& operator=(mailbox_array const&) = delete;

        mailbox_array(mailbox_array&& other) noexcept
          : heap_(other.heap_)
          , tx_beg_(other.tx_beg_)
          , tx_end_(other.tx_end_)
          , rx_beg_(other.rx_beg_)
          , rx_end_(other.rx_end_)
          , produced_beg_(other.produced_beg_)
          , produced_end_(other.produced_end_)
          , consumed_beg_(other.consumed_beg_)
          , consumed_end_(other.consumed_end_)
          , config_beg_(other.config_beg_)
          , config_end_(other.config_end_)
          , produced_locals_(other.produced_locals_)
          , consumed_locals_(other.consumed_locals_)
          , scan_rotate_(other.scan_rotate_)
          , num_pes_(other.num_pes_)
          , my_pe_(other.my_pe_)
          , mtu_(other.mtu_)
          , slots_per_dst_(other.slots_per_dst_)
          , symmetric_memory_size_(other.symmetric_memory_size_)
          , checksum_(other.checksum_)
          , probe_window_(other.probe_window_)
        {
            other.heap_ = nullptr;
            other.tx_beg_ = nullptr;
            other.tx_end_ = nullptr;
            other.rx_beg_ = nullptr;
            other.rx_end_ = nullptr;
            other.produced_beg_ = nullptr;
            other.produced_end_ = nullptr;
            other.consumed_beg_ = nullptr;
            other.consumed_end_ = nullptr;
            other.config_beg_ = nullptr;
            other.config_end_ = nullptr;
            other.produced_locals_ = nullptr;
            other.consumed_locals_ = nullptr;
            other.scan_rotate_ = nullptr;
        }

        mailbox_array& operator=(mailbox_array&& other) noexcept
        {
            if (this != &other)
            {
                if (heap_)
                {
                    shmem_free(heap_);
                }
                delete[] produced_locals_;
                delete[] consumed_locals_;
                heap_ = other.heap_;
                tx_beg_ = other.tx_beg_;
                tx_end_ = other.tx_end_;
                rx_beg_ = other.rx_beg_;
                rx_end_ = other.rx_end_;
                produced_beg_ = other.produced_beg_;
                produced_end_ = other.produced_end_;
                consumed_beg_ = other.consumed_beg_;
                consumed_end_ = other.consumed_end_;
                config_beg_ = other.config_beg_;
                config_end_ = other.config_end_;
                produced_locals_ = other.produced_locals_;
                consumed_locals_ = other.consumed_locals_;
                scan_rotate_ = other.scan_rotate_;
                num_pes_ = other.num_pes_;
                my_pe_ = other.my_pe_;
                mtu_ = other.mtu_;
                slots_per_dst_ = other.slots_per_dst_;
                symmetric_memory_size_ = other.symmetric_memory_size_;
                checksum_ = other.checksum_;
                probe_window_ = other.probe_window_;
                other.heap_ = nullptr;
                other.tx_beg_ = nullptr;
                other.tx_end_ = nullptr;
                other.rx_beg_ = nullptr;
                other.rx_end_ = nullptr;
                other.produced_beg_ = nullptr;
                other.produced_end_ = nullptr;
                other.consumed_beg_ = nullptr;
                other.consumed_end_ = nullptr;
                other.config_beg_ = nullptr;
                other.config_end_ = nullptr;
                other.produced_locals_ = nullptr;
                other.consumed_locals_ = nullptr;
                other.scan_rotate_ = nullptr;
            }
            return *this;
        }

        // Return the symmetric scratch page that a sender uses to stage the
        // chunk for the current credit slot of (dst_pe).
        unsigned char* tx_page(std::size_t dst_pe) const
        {
            std::size_t const slot =
                produced_locals_[dst_pe].load(std::memory_order_relaxed) %
                slots_per_dst_;
            return tx_beg_ + (dst_pe * slots_per_dst_ + slot) * mtu_;
        }

        unsigned char* get_buffer(std::size_t pe) const
        {
            return tx_page(pe);
        }

        // checksum + sequence guard over received pages.  Disable only when the
        // checksum pass shows up in profiling
        bool checksum_enabled() const noexcept
        {
            return checksum_;
        }

        // Total number of pages transferred so far (published + consumed),
        // summed over the local mirrors.  Only used by do_stop()
        std::uint64_t transfer_count() const noexcept
        {
            std::uint64_t total = 0;
            for (std::size_t i = 0; i < num_pes_; ++i)
            {
                total += produced_locals_[i].load(std::memory_order_relaxed);
                total += consumed_locals_[i].load(std::memory_order_relaxed);
            }
            return total;
        }

        // Non-blocking scan of the source arena owned by 'arena_idx'.  With
        // 'arena_cnt' concurrent scanners each thread polls only its own slots
        int try_detect_pe_notification(std::size_t arena_idx = 0,
            std::size_t arena_cnt = 0) const noexcept
        {
            if (arena_cnt == 0)
            {
                arena_cnt = 1;
            }

            std::size_t const lo = (num_pes_ * arena_idx) / arena_cnt;
            std::size_t const hi = (num_pes_ * (arena_idx + 1)) / arena_cnt;
            std::size_t const width = hi - lo;
            if (width == 0)
            {
                return -1;
            }

            std::size_t const window = probe_window_ == 0 ?
                width :
                (std::min)(width, probe_window_);

            // Rotate the scan start point so no single (lowest-index) ready
            // source can hog this arena.
            std::size_t const slot =
                arena_idx < num_pes_ ? arena_idx : num_pes_ - 1;
            std::size_t start =
                lo + (scan_rotate_[slot].fetch_add(1,
                         std::memory_order_relaxed) %
                    width);

            for (std::size_t k = 0; k < window; ++k)
            {
                std::size_t const src = lo + ((start - lo + k) % width);

                if (src == my_pe_)
                {
                    continue;
                }
                std::size_t const w = src * num_pes_ + my_pe_;

                // Pull delivery of src's inbound stores (putmem + produced
                // atomic_set) into the local symmetric memory
                progress_to(src);
                std::uint32_t const produced = produced_beg_[w];
                std::uint32_t const consumed =
                    consumed_locals_[src].load(std::memory_order_relaxed);
                if (produced > consumed)
                {
                    return static_cast<int>(src);
                }
            }
            return -1;
        }

        // progress_to(peer): a remote atomic_fetch to the peer "pushes" the
        // peer's inbound stores into our local symmetric memory
        void progress_to(std::size_t const peer) const noexcept
        {
            std::size_t const w = my_pe_ * num_pes_ + peer;
            shmem_uint32_atomic_fetch(
                &produced_beg_[w], static_cast<int>(peer));
            shmem_uint32_atomic_fetch(
                &consumed_beg_[w], static_cast<int>(peer));
        }

        // Non-blocking send of one mtu-sized page (chunk) to dst_pe.
        // Copies 'count' bytes from our staging page (get_buffer())
        bool try_send(std::size_t const dst_pe,
            std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            // Works for both remote (dst != me) and local (dst == me, a
            // self-putmem into our own rx pool) destinations.
            std::size_t const w = my_pe_ * num_pes_ + dst_pe;

            // mirrors of the two halves
            std::uint32_t produced =
                produced_locals_[dst_pe].load(std::memory_order_relaxed);

            // dst's consumed counter: for a local send it is our own drain
            // mirror (same thread); otherwise the delivered remote copy.
            std::uint32_t consumed = (dst_pe == my_pe_) ?
                consumed_locals_[dst_pe].load(std::memory_order_relaxed) :
                consumed_beg_[w];

            // Poll the credit (max slots_per_dst_ pages in flight).  Deliver
            // dst's consumed counter exactly once; if still no credit, return
            // false to retry
            if (static_cast<int>(produced - consumed) >=
                static_cast<int>(slots_per_dst_))
            {
                if (dst_pe != my_pe_)
                {
                    progress_to(dst_pe);            // deliver dst's consumed
                    consumed = consumed_beg_[w];    // read local (delivered)
                }
                else
                {
                    consumed =
                        consumed_locals_[dst_pe].load(std::memory_order_relaxed);
                }

                if (static_cast<int>(produced - consumed) >=
                    static_cast<int>(slots_per_dst_))
                {
                    return false;    // no credit: requeue and retry later
                }
            }

            std::size_t const slot = produced % slots_per_dst_;

            // Staging source: my per-dst staging page (tx mirror of the rx
            // slot), keyed by dst_pe
            std::size_t const stage_slot =
                (dst_pe * slots_per_dst_ + slot) * mtu_;

            // Landing target: dst's shared rx pool, keyed by self PE (the
            // sender), so each sender has its own disjoint per-src ring
            std::size_t const rx_slot = (my_pe_ * slots_per_dst_ + slot) * mtu_;

            // data: blocking putmem staging -> dst's rx page.
            shmem_putmem(rx_beg_ + rx_slot, tx_beg_ + stage_slot, count,
                static_cast<int>(dst_pe));

            // publish produced (low-32; self owns it)
            produced = produced + 1;
            shmem_uint32_atomic_set(
                &produced_beg_[w], produced, static_cast<int>(dst_pe));
            produced_locals_[dst_pe].store(
                produced, std::memory_order_relaxed);    // update our mirror
            shmem_fence();

            // delivery (push): issue a remote atomic_fetch to the peer right
            // after publishing produced, so our inbound stores.
            if (dst_pe != my_pe_)
            {
                progress_to(dst_pe);
            }

            return true;
        }

        // Non-blocking receive of one page (chunk) from src_pe.  Copies
        // 'count' bytes off our shared rx page into out_buf, then returns
        bool try_receive_(std::size_t const src_pe,
            unsigned char* const out_buf, std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            std::size_t const w = src_pe * num_pes_ + my_pe_;

            std::uint32_t const consumed =
                consumed_locals_[src_pe].load(std::memory_order_relaxed);

            std::uint32_t produced = produced_beg_[w];

            // Poll for src to publish page 'consumed' (data ready).  Deliver
            // src's produced exactly once; if still not ready, requeue.
            if (produced <= consumed)
            {
                progress_to(src_pe);    // deliver src's produced
                produced = produced_beg_[w];
                if (produced <= consumed)
                {
                    return false;    // not ready: requeue and retry later
                }
            }

            std::size_t const slot = consumed % slots_per_dst_;
            // Read from src's per-src ring in our shared rx pool
            // ((src_pe * slots + slot) * mtu_).  The sender keys its landing
            std::size_t const rx_slot = (src_pe * slots_per_dst_ + slot) * mtu_;

            std::memcpy(out_buf, rx_beg_ + rx_slot, count);

            // return the credit: publish consumed (high-32; self owns it) onto src
            shmem_uint32_atomic_set(
                &consumed_beg_[w], consumed + 1, static_cast<int>(src_pe));
            consumed_locals_[src_pe].store(
                consumed + 1, std::memory_order_relaxed);    // update mirror
            shmem_fence();

            return true;
        }

        bool receive(
            unsigned char* const output, std::size_t const count) noexcept
        {
            int const pe = try_detect_pe_notification();
            if (pe < 0)
            {
                return false;
            }
            return try_receive_(static_cast<std::size_t>(pe), output, count);
        }

        constexpr std::size_t mtu() const noexcept
        {
            return mtu_;
        }

        // The startup wire-protocol uniformity check in the parcelport needs
        // one symmetric word per PE to publish its fingerprint into.  Hand out
        // the row carved from this allocation instead of a separate
        // shmem_malloc(): that call is collective, and do_run() is not reached
        // by every PE in lockstep (rank 0 runs in runtime_mode::console, the
        // others in runtime_mode::worker), so allocating there deadlocked the
        // job before main().  This carve is identical on every PE and was
        // already published behind the constructor's barrier.
        // const: the caller (check_protocol_config_uniformity) is a const
        // member of the parcelport and so sees a const mailbox_array.  The
        // returned pointer is a copy, so the pointee stays writable; only the
        // member pointer itself is const through this overload.
        std::uint32_t* config_words() const noexcept
        {
            return config_beg_;
        }

        constexpr std::size_t num_pes() const noexcept
        {
            return num_pes_;
        }

        constexpr std::size_t my_pe() const noexcept
        {
            return my_pe_;
        }

        // Monotonically increasing per-PE message identifier.  Every parcel
        // (message) gets a fresh id, which the receiver uses to cross-check
        std::uint64_t next_message_id() noexcept
        {
            return message_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
        }

    private:
        void* heap_ = nullptr;

        unsigned char* tx_beg_ = nullptr;
        unsigned char* tx_end_ = nullptr;
        unsigned char* rx_beg_ = nullptr;
        unsigned char* rx_end_ = nullptr;
        std::uint32_t* produced_beg_ = nullptr;
        std::uint32_t* produced_end_ = nullptr;
        std::uint32_t* consumed_beg_ = nullptr;
        std::uint32_t* consumed_end_ = nullptr;
        std::uint32_t* config_beg_ = nullptr;
        std::uint32_t* config_end_ = nullptr;
        std::atomic<std::uint32_t>* produced_locals_ = nullptr;
        std::atomic<std::uint32_t>* consumed_locals_ = nullptr;
        mutable std::atomic<std::uint32_t>* scan_rotate_ = nullptr;

        std::size_t num_pes_ = 0;
        std::size_t my_pe_ = 0;
        std::size_t mtu_ = 0;
        std::size_t slots_per_dst_ = 0;
        std::size_t symmetric_memory_size_ = 0;
        bool checksum_ = true;
        std::size_t probe_window_ = default_probe_window;

        std::atomic<std::uint64_t> message_sequence_{0};
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
