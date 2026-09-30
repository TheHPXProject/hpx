//  Copyright (c) 2023-2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/gasnet_base.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::gasnet {

    // TX/RX page-arena + per-pair 64-bit credit-word protocol
    //
    // ONE carve of the local GASNet registered segment holds everything:
    //     tx       : npes * num slots * mtu   (my outbound staging/scratch pages)
    //     rx       : npes * num slots * mtu   (my shared inbound landing pages)
    //     produced : npes * npes * 4      (uint32, low-32 of per-pair word)
    //     consumed : npes * npes * 4      (uint32, high-32 of per-pair word)
    //     scratch  : npes * 4             (per-PE fingerprint, uniformity check)
    //   -> one 64-bit credit per (sender i, receiver j): low32 = produced,
    //      high32 = consumed.  Payload lives in the tx/rx page pools which
    //      are O(npes * SLOTS) and linear in the number of PEs; the npes^2
    //      credit words are tiny 4-byte words (no npes^2 page grid).
    //
    // Every PE carves the same byte count off the front of its own segment
    // (offset 0), so a remote target for a given local offset is simply
    // remote_segment_addr(rank) + offset.  Per-PE segment base addresses are
    // collected once in gasnet_environment::init() via
    // gex_EP_QueryBoundSegmentNB() and exposed through
    // gasnet_environment::remote_segment_addr().  There is no symmetric
    // heap: all RMA addresses are derived from the segment-base table.
    //
    // Slot-writer ownership (avoids torn writes):
    //     produced[i*npes+j] written by sender (tx) i onto j;  receiver j reads
    //                                                     its own copy locally
    //     consumed[i*npes+j] written by receiver (rx) j onto i; sender i reads
    //                                                     its own copy locally
    //   -> each 32-bit half has a single writer; a plain (relaxed) atomic load
    //      on the reading side and a GASNet RMA put on the writing side.
    //
    // Transport rule (mirrors the validated OpenSHMEM harness):
    //   - each side reads its associated counter locally (relaxed load)
    //   - each side writes the peer's counter via GASNet RMA put
    //   - ordering between the data put and the produced-credit put is
    //     guaranteed by an NBI access region (see
    //     gasnet_environment::put_data_and_credit_nb()): the credit becomes
    //     visible at the receiver only after the data page has been
    //     performed there, replacing putmem + shmem_fence + atomic_set.
    //   - GASNet RDMA lands inbound puts directly in our registered segment,
    //     so no delivery-push (the openshmem design's remote atomic_fetch)
    //     is needed: progress_to() is a local no-op, and counter reads are
    //     plain local loads.  This removes two remote round-trips per probe.
    //
    // Receiver j's landing page for a page written by sender i lives in j's
    // shared rx pool at offset (i*SLOTS + (p % SLOTS)) * mtu.  Each sender is
    // assigned its own SLOTS-sized slot range, so concurrent senders never
    // reuse the same landing slot (per-destination single-flight, which the
    // parcelport enforces via the sender's per-destination reservation, keeps
    // this safe).
    //
    // The local mirror counters are atomic: with several threads driving
    // progress (one io_service driver per arena plus any HPX thread that runs
    // the background-works callback), the arena scan may read a source's
    // consumed mirror while the thread delivering that source's pages writes
    // it.  Relaxed ordering suffices — they are local-only watermarks used to
    // derive slot indices and to decide "published vs consumed".
    //
    // One MTU-sized page == one chunk (message_header + payload), matching
    // how sender_connection stages a chunk into an mtu slot.

    namespace detail {

        // Wire header of a chunk, shared by sender_connection (writer) and
        // receiver_connection (reader).  Each page carries the full header so
        // every chunk is independently addressable; the receiver validates
        // each page before trusting a single field.
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

        // CRC32, table-driven, over the payload bytes only.  The header
        // fields are validated structurally (exact geometry) and by
        // message-id/sequence cross-checks, so the checksum does not need
        // to cover the header itself — which keeps the sender and receiver
        // able to recompute it over a single contiguous byte range (the
        // transferred payload region).
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
        // 'payload_size' bytes each.  Every non-last chunk fills the payload
        // entirely; the last chunk carries the remainder (0 for an empty
        // message).  This is the single source of truth for the chunk-boundary
        // arithmetic shared by the sender (stage_chunk) and the receiver's
        // validation path (validate_message_page and the geometry check).
        inline std::size_t chunk_size_for(std::uint64_t chunk_index,
            std::uint64_t num_chunks, std::uint64_t total_size,
            std::size_t payload_size) noexcept
        {
            std::uint64_t const offset = chunk_index * payload_size;
            return static_cast<std::size_t>((chunk_index == num_chunks - 1) ?
                    (total_size - offset) :
                    payload_size);
        }

        // Structural sanity of one page header (no checksum).  Rejects values
        // that stage_chunk() could not have produced or that would break
        // downstream chunk geometry / allocation sizes.
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
        // checksum).  Fills 'chunk_size' only on success; the caller must not
        // trust any header field after a false return.  'use_checksum' makes
        // the payload CRC pass optional (geometry and message-id/sequence
        // cross-checks always run); sender and receiver must agree on it.
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
        // (sender,receiver) pair before the sender must wait for consumed
        // credit.  Overridable at runtime via hpx.parcel.gasnet.slots
        // (must be identical on every PE; build-time default comes from
        // HPX_WITH_PARCELPORT_GASNET_SLOTS).  Raisable to pipeline more
        // chunks across high-latency links; every slot costs one MTU page of
        // registered tx/rx memory per PE, so lower it for very large runs.
        static constexpr std::size_t default_slots_per_dst = 8;

        // Max number of source PEs probed per receive-scan pass, exactly as
        // in the openshmem mailbox: the scan window slides over the arena (one
        // source per call, the same rotation that guarantees fairness).  With
        // GASNet the probe is a single local load (no remote fetch), so the
        // window chiefly bounds the per-pass loop length at large n.
        static constexpr std::size_t default_probe_window = 8;

        // Period (in cumulative back-pressure polls) between the stall
        // tripwire lines emitted by the credit and region waiters.  A healthy
        // transport never approaches 2^20 returns that found the window full
        // or an NB operation still in flight; sustained emission indicates the
        // peer is draining slowly or a credit word was lost (the latter would
        // otherwise surface only as the do_stop() 30s abort).
        static constexpr std::uint64_t stall_warn_interval =
            static_cast<std::uint64_t>(1) << 20;

        // Per-PE registered-segment footprint in bytes for a given geometry
        // (identical on every PE by construction): two O(npes*slots*mtu)
        // page pools plus two O(npes^2) 32-bit credit matrices and one
        // O(npes) fingerprint scratch region, i.e.
        //
        //   2 * npes * slots * mtu + 2 * npes^2 * 4 + npes * 4 bytes
        //
        // With the defaults (slots=8, mtu=64 KiB) the footprint is ~1 MiB per
        // added PE, so a 1024-PE run occupies ~1 GiB of registered memory.
        // The constructor asserts the carve fits the segment attached in
        // hpx::util::gasnet_environment::init().
        static constexpr std::size_t symmetric_bytes(
            std::size_t npes, std::size_t slots, std::size_t mtu) noexcept
        {
            return 2 * npes * slots * mtu +
                2 * npes * npes * sizeof(std::uint32_t) +
                npes * sizeof(std::uint32_t);
        }

        mailbox_array() = default;

        mailbox_array(std::size_t num_pes, std::size_t my_pe,
            std::size_t mtu, std::size_t slots_per_dst, bool checksum = true,
            std::size_t probe_window = default_probe_window)
          : num_pes_(num_pes)
          , my_pe_(my_pe)
          , mtu_(mtu)
          , slots_per_dst_(slots_per_dst)
          , checksum_(checksum)
          , probe_window_(probe_window)
        {
            HPX_ASSERT(slots_per_dst_ > 0);
            HPX_ASSERT(num_pes_ > 0);
            HPX_ASSERT(my_pe_ < num_pes_);

            std::size_t const bytes =
                symmetric_bytes(num_pes_, slots_per_dst_, mtu_);

            // The mailbox is carved out of the front of the local GASNet
            // registered segment attached during hpx::util::gasnet_environment::init().
            // Every PE carves the same byte count at offset 0, so remote RMA
            // targets are derived by adding a local offset to the peer's
            // segment base address.
            if (hpx::util::gasnet_environment::segment_addr() == nullptr ||
                bytes > hpx::util::gasnet_environment::segment_size())
            {
                HPX_THROW_EXCEPTION(hpx::error::out_of_memory,
                    "gasnet::mailbox_array::mailbox_array",
                    "mailbox carve of " + std::to_string(bytes) +
                        " bytes does not fit the registered segment (" +
                        std::to_string(hpx::util::gasnet_environment::segment_size()) +
                        " bytes) on PE " + std::to_string(my_pe_));
            }

            unsigned char* base =
                static_cast<unsigned char*>(hpx::util::gasnet_environment::segment_addr());
            base_ = base;
            carved_bytes_ = bytes;

            std::size_t const pages = num_pes_ * slots_per_dst_ * mtu_;
            tx_beg_ = base;
            tx_end_ = base + pages;
            rx_beg_ = tx_end_;
            rx_end_ = rx_beg_ + pages;
            produced_beg_ = reinterpret_cast<std::uint32_t*>(rx_end_);
            produced_end_ = produced_beg_ + num_pes_ * num_pes_;
            consumed_beg_ = produced_end_;
            consumed_end_ = consumed_beg_ + num_pes_ * num_pes_;
            scratch_beg_ = reinterpret_cast<std::uint32_t*>(consumed_end_);

            // Zeroed produced/consumed = correct monotonic start state (0
            // pages produced/consumed).  tx/rx pages are always fully staged
            // before use, but a deterministic, zeroed carve keeps debug runs
            // reproducible.  The memset is purely local; the do_run() barrier
            // (see parcelport_gasnet.cpp) guarantees no peer can begin putting
            // into our rx pages or credit words before every PE has finished
            // this zeroing.  (This replaces the shmem_barrier_all() the
            // openshmem counterpart placed in its constructor, which cannot be
            // reproduced here because C++ construction is not a collective
            // operation in GASNet.)
            std::memset(base, 0, bytes);

            // Local-only mirror counters.  produced_locals_[dst] = how many
            // pages I have published to dst, consumed_locals_[src] = how many
            // pages I have consumed from src.  These persist across
            // send()/receive_() calls; the remote copies of our own counters
            // are updated by RMA writes from the peer, not by our stores, so
            // the local mirrors track the values we communicate.
            produced_locals_ = new std::atomic<std::uint32_t>[num_pes_];
            consumed_locals_ = new std::atomic<std::uint32_t>[num_pes_];
            scan_rotate_ = new std::atomic<std::uint32_t>[num_pes_];
            for (std::size_t i = 0; i < num_pes_; ++i)
            {
                produced_locals_[i].store(0, std::memory_order_relaxed);
                consumed_locals_[i].store(0, std::memory_order_relaxed);
                scan_rotate_[i].store(0, std::memory_order_relaxed);
            }

            // Non-blocking RMA transport state: one event slot per dst/src
            // (single outstanding region in each direction), one durable
            // 32-bit credit source word per direction (the async puts read it
            // after try_send()/try_receive_() return), and the stall
            // tripwire counters.
            send_events_.assign(num_pes_, GEX_EVENT_INVALID);
            credit_events_.assign(num_pes_, GEX_EVENT_INVALID);
            send_credit_src_.assign(num_pes_, 0u);
            recv_credit_src_.assign(num_pes_, 0u);
            send_stall_.assign(num_pes_, 0u);
            recv_stall_.assign(num_pes_, 0u);
        }

        ~mailbox_array()
        {
            delete[] produced_locals_;
            delete[] consumed_locals_;
            delete[] scan_rotate_;
        }

        mailbox_array(mailbox_array const&) = delete;
        mailbox_array& operator=(mailbox_array const&) = delete;

        mailbox_array(mailbox_array&& other) noexcept
          : base_(other.base_)
          , carved_bytes_(other.carved_bytes_)
          , tx_beg_(other.tx_beg_)
          , tx_end_(other.tx_end_)
          , rx_beg_(other.rx_beg_)
          , rx_end_(other.rx_end_)
          , produced_beg_(other.produced_beg_)
          , produced_end_(other.produced_end_)
          , consumed_beg_(other.consumed_beg_)
          , consumed_end_(other.consumed_end_)
          , scratch_beg_(other.scratch_beg_)
          , produced_locals_(other.produced_locals_)
          , consumed_locals_(other.consumed_locals_)
          , scan_rotate_(other.scan_rotate_)
          , num_pes_(other.num_pes_)
          , my_pe_(other.my_pe_)
          , mtu_(other.mtu_)
          , slots_per_dst_(other.slots_per_dst_)
          , checksum_(other.checksum_)
          , probe_window_(other.probe_window_)
          , send_events_(std::move(other.send_events_))
          , credit_events_(std::move(other.credit_events_))
          , send_credit_src_(std::move(other.send_credit_src_))
          , recv_credit_src_(std::move(other.recv_credit_src_))
          , send_stall_(std::move(other.send_stall_))
          , recv_stall_(std::move(other.recv_stall_))
        {
            other.base_ = nullptr;
            other.carved_bytes_ = 0;
            other.tx_beg_ = nullptr;
            other.tx_end_ = nullptr;
            other.rx_beg_ = nullptr;
            other.rx_end_ = nullptr;
            other.produced_beg_ = nullptr;
            other.produced_end_ = nullptr;
            other.consumed_beg_ = nullptr;
            other.consumed_end_ = nullptr;
            other.scratch_beg_ = nullptr;
            other.produced_locals_ = nullptr;
            other.consumed_locals_ = nullptr;
            other.scan_rotate_ = nullptr;
        }

        mailbox_array& operator=(mailbox_array&& other) noexcept
        {
            if (this != &other)
            {
                delete[] produced_locals_;
                delete[] consumed_locals_;
                base_ = other.base_;
                carved_bytes_ = other.carved_bytes_;
                tx_beg_ = other.tx_beg_;
                tx_end_ = other.tx_end_;
                rx_beg_ = other.rx_beg_;
                rx_end_ = other.rx_end_;
                produced_beg_ = other.produced_beg_;
                produced_end_ = other.produced_end_;
                consumed_beg_ = other.consumed_beg_;
                consumed_end_ = other.consumed_end_;
                scratch_beg_ = other.scratch_beg_;
                produced_locals_ = other.produced_locals_;
                consumed_locals_ = other.consumed_locals_;
                scan_rotate_ = other.scan_rotate_;
                num_pes_ = other.num_pes_;
                my_pe_ = other.my_pe_;
                mtu_ = other.mtu_;
                slots_per_dst_ = other.slots_per_dst_;
                checksum_ = other.checksum_;
                probe_window_ = other.probe_window_;
                send_events_ = std::move(other.send_events_);
                credit_events_ = std::move(other.credit_events_);
                send_credit_src_ = std::move(other.send_credit_src_);
                recv_credit_src_ = std::move(other.recv_credit_src_);
                send_stall_ = std::move(other.send_stall_);
                recv_stall_ = std::move(other.recv_stall_);
                other.base_ = nullptr;
                other.carved_bytes_ = 0;
                other.tx_beg_ = nullptr;
                other.tx_end_ = nullptr;
                other.rx_beg_ = nullptr;
                other.rx_end_ = nullptr;
                other.produced_beg_ = nullptr;
                other.produced_end_ = nullptr;
                other.consumed_beg_ = nullptr;
                other.consumed_end_ = nullptr;
                other.scratch_beg_ = nullptr;
                other.produced_locals_ = nullptr;
                other.consumed_locals_ = nullptr;
                other.scan_rotate_ = nullptr;
            }
            return *this;
        }

        // Return the registered staging page that a sender uses to stage the
        // chunk for the current credit slot of (dst_pe). Each ring slot has
        // its own dedicated staging page (per-slot TX pages).  The page lives
        // in our own segment; it is only ever the *source* of an RMA put, so
        // it needs no remote registration.
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

        // Remote address on 'rank' of a pointer that lies inside our carve of
        // the registered segment: every PE carved the same layout at offset 0
        // of its own segment, so the offset identifies the target byte on the
        // peer.  All RMA destinations (rx pages, peer-owned parts of the
        // credit words, fingerprint words) are computed this way; GASNet
        // requires remote put/get targets to live inside the peer's segment.
        void* remote_ptr(std::size_t rank, void const* local) const noexcept
        {
            HPX_ASSERT(local >= base_ && local < base_ + carved_bytes_);
            return static_cast<unsigned char*>(
                       hpx::util::gasnet_environment::remote_segment_addr(
                           static_cast<int>(rank))) +
                (static_cast<unsigned char const*>(local) - base_);
        }

        unsigned char* scratch_beg() const noexcept
        {
            return reinterpret_cast<unsigned char*>(scratch_beg_);
        }

        // CRC32 + sequence guard over received pages.  Disable only when the
        // checksum pass shows up in profiling; turning it off removes the
        // corruption tripwire.  Must be identical on every PE, like slots and
        // mtu.
        bool checksum_enabled() const noexcept
        {
            return checksum_;
        }

        // Total number of pages transferred so far (published + consumed),
        // summed over the local mirrors.  Only used by do_stop() to tell a
        // slow-but-moving drain from a stalled one: every successful
        // try_send()/try_receive_() increments exactly one of the two arrays,
        // so a changing sum proves transport activity regardless of how many
        // (or which) connections are queued.  Cheap: two relaxed loads per PE.
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
        // 'arena_cnt' concurrent scanners each thread polls only its own
        // disjoint range of source PEs [lo, hi).  Returns the index of the
        // first PE in this arena that has at least one page we have not yet
        // consumed, or -1 if none.
        //
        // Each probe is a single local relaxed load of src's produced counter
        // (produced_beg_[src*npes+my_pe]): GASNet RDMA leaves the sender's
        // put of the data page *and* that counter directly in our registered
        // segment, so no remote read is needed to "pull" delivery — the bytes
        // are already visible.  This is the GASNet counterpart of the
        // openshmem code's shmem_uint32_atomic_fetch ping-pong, and it costs
        // zero remote round-trips.
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
            // source can hog this arena.  A fixed low-to-high scan always
            // returns the lowest index, so on every PE the busiest low-index
            // peer (typically the AGAS console, PE 0) gets served
            // exclusively: higher-index sources are never probed and their
            // sender windows fill, so with 3+ localities bootstrap deadlocks.
            // Resuming just past the last served source guarantees every
            // ready source is probed within 'width' consecutive calls.
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

                // Acquire so a same-PE self-send's release-store of produced
                // is ordered after its data-page memcpy (weak-memory cores).
                // For a remote send the data/credit pair arrives ordered by
                // the originator's NBI access region, and the acquire then
                // orders our downstream reads of the landing page.
                std::uint32_t const produced =
                    std::atomic_ref<std::uint32_t>(
                        produced_beg_[w])
                        .load(std::memory_order_acquire);
                std::uint32_t const consumed =
                    consumed_locals_[src].load(std::memory_order_relaxed);
                if (produced > consumed)
                {
                    return static_cast<int>(src);
                }
            }
            return -1;
        }

        // progress_to(peer): **local no-op** in the GASNet port.  The
        // openshmem counterpart issues a remote shmem_uint32_atomic_fetch to
        // the peer purely to *push* delivery of the peer's inbound stores into
        // local symmetric memory (an OpenSHMEM-over-UCX quirk).  GASNet RMA
        // puts land directly in our registered segment, so the counter values
        // are already visible to plain local loads.  Kept as a method so the
        // call sequence mirrors the openshmem design; the loads it issues are
        // purely documentary.
        void progress_to(std::size_t const peer) const noexcept
        {
            std::size_t const w = my_pe_ * num_pes_ + peer;
            (void) std::atomic_ref<std::uint32_t>(
                produced_beg_[w])
                .load(std::memory_order_relaxed);
            (void) std::atomic_ref<std::uint32_t>(
                consumed_beg_[w])
                .load(std::memory_order_relaxed);
        }

        // True when the single outstanding data+credit access region to
        // dst_pe (if any) has locally completed, i.e. the TX slot for dst_pe
        // may be restaged and the next region issued; false while the
        // previous region is still in flight.  Polls the stored event via
        // gasnet_environment::poll_event(), which never blocks; the event is
        // consumed on completion.  Always true for self-sends, which issue no
        // region.  Callers MUST re-check this before staging a new page into
        // a TX slot whose event is not yet complete (mandatory when
        // slots_per_dst_ == 1, where every chunk restages the same page).
        bool send_ready(std::size_t const dst_pe) noexcept
        {
            gex_Event_t& ev = send_events_[dst_pe];
            if (ev == GEX_EVENT_INVALID)
            {
                return true;
            }
            if (!hpx::util::gasnet_environment::poll_event(ev))
            {
                return false;
            }
            ev = GEX_EVENT_INVALID;
            return true;
        }

        // Non-blocking send of one mtu-sized page (chunk) to dst_pe.
        // Copies 'count' bytes from our staging page (get_buffer()) into
        // dst's shared rx slot range and publishes produced.  Returns false
        // without transferring when no credit is left or the single
        // outstanding access region to dst has not yet completed locally;
        // the caller must requeue and retry later.  Never blocks.
        bool try_send(std::size_t const dst_pe,
            std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            // Single outstanding region per destination: GASNet orders
            // operations only within one NBI access region, so successive
            // regions (successive credit word values) could reach dst out of
            // order and let a receiver drain a not-yet-arrived page.  We may
            // stage and issue only after the previous region's event
            // completes.
            if (!send_ready(dst_pe))
            {
                note_send_stall(dst_pe);
                return false;
            }

            // Works for both remote (dst != me) and local (dst == me, a
            // self-copy into our own rx pool) destinations.
            std::size_t const w = my_pe_ * num_pes_ + dst_pe;

            // mirrors of the two halves
            std::uint32_t produced =
                produced_locals_[dst_pe].load(std::memory_order_relaxed);

            // dst's consumed counter: for a local send it is our own drain
            // mirror (same thread); otherwise the RDMA-delivered remote copy,
            // read directly from our segment.
            // Acquire so that (self path) the receiver's release-store of the
            // returned credit is ordered after its data-page memcpy, and
            // (remote path) our subsequent slot reuse observes the credit put
            // that the dst issued after draining the page.
            std::uint32_t consumed = (dst_pe == my_pe_) ?
                consumed_locals_[dst_pe].load(std::memory_order_acquire) :
                std::atomic_ref<std::uint32_t>(consumed_beg_[w])
                    .load(std::memory_order_acquire);

            // Poll the credit (max slots_per_dst_ pages in flight).  If the
            // counter is still short, the caller requeues instead of blocking.
            if (static_cast<int>(produced - consumed) >=
                static_cast<int>(slots_per_dst_))
            {
                note_send_stall(dst_pe);
                return false;    // no credit: requeue and retry later
            }

            std::size_t const slot = produced % slots_per_dst_;

            // Staging source: my per-dst staging page (tx mirror of the rx
            // slot), keyed by dst_pe so two different destinations use
            // disjoint staging pages (matches tx_page()).
            std::size_t const stage_slot =
                (dst_pe * slots_per_dst_ + slot) * mtu_;

            // Landing target: dst's shared rx pool, keyed by self PE (the
            // sender), so each sender has its own disjoint per-src ring and
            // different senders cannot collide on the same guard pages.
            std::size_t const rx_slot = (my_pe_ * slots_per_dst_ + slot) * mtu_;

            produced = produced + 1;

            if (dst_pe == my_pe_)
            {
                // Self-send: local copy of the data and a local store of the
                // produced credit.  Release-store orders the data-page
                // memcpy before the credit becomes visible (a progress thread
                // on another core polls it).  No GASNet traffic is required
                // for locality-to-self parcels (very common during bootstrap).
                std::memcpy(
                    rx_beg_ + rx_slot, tx_beg_ + stage_slot, count);
                std::atomic_ref<std::uint32_t>(produced_beg_[w])
                    .store(produced, std::memory_order_release);
                produced_locals_[dst_pe].store(
                    produced, std::memory_order_release);
                return true;
            }

            // Remote send: stage -> dst's rx page, then publish the produced
            // credit, both ordered by one NBI access region (the credit is
            // performed at the target only after the data).  The credit
            // word is staged into a durable per-dst slot because the async
            // put reads it after this call returns, and the region event is
            // recorded so the caller cannot restage the TX slot (or issue the
            // next region) until the put has locally completed.
            void* const raddr = remote_ptr(dst_pe, rx_beg_ + rx_slot);
            void* const rcredit = remote_ptr(dst_pe, &produced_beg_[w]);
            // Release fence: the staging-page writes done by the caller on
            // this thread must be globally visible before the GEX put engine
            // reads that source buffer (weak-memory hosts).
            std::atomic_thread_fence(std::memory_order_release);
            send_credit_src_[dst_pe] = produced;
            send_events_[dst_pe] =
                hpx::util::gasnet_environment::put_data_and_credit_nb(
                    static_cast<int>(dst_pe), raddr, tx_beg_ + stage_slot,
                    count, rcredit, &send_credit_src_[dst_pe]);
            produced_locals_[dst_pe].store(
                produced, std::memory_order_relaxed);    // update our mirror

            return true;
        }

        // Non-blocking receive of one page (chunk) from src_pe.  Copies
        // 'count' bytes off our shared rx page into out_buf, then returns
        // the credit (publishes consumed) so src may reuse the slot.  If the
        // page is not yet published, returns false; the caller must requeue
        // and retry later.  Never blocks.
        bool try_receive_(std::size_t const src_pe,
            unsigned char* const out_buf, std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            // Single outstanding credit-return region per source: the credit
            // word returned for a drained page must reach src before we drain
            // the next one, or src could observe a reordered high watermark
            // and reuse a slot we have not yet finished reading.  No credit
            // is in flight for self-sends (local store), so this passes.
            gex_Event_t& cev = credit_events_[src_pe];
            if (cev != GEX_EVENT_INVALID)
            {
                if (!hpx::util::gasnet_environment::poll_event(cev))
                {
                    note_recv_stall(src_pe);
                    return false;
                }
                cev = GEX_EVENT_INVALID;
            }

            std::size_t const w = src_pe * num_pes_ + my_pe_;

            std::uint32_t const consumed =
                consumed_locals_[src_pe].load(std::memory_order_relaxed);

            // Poll for src to publish page 'consumed' (data ready).  The
            // counter was RMA-written into our registered segment by src, so
            // a local load observes it directly (no delivery fetch needed).
            // Acquire matches the sender's release (self path) or the RMA
            // credit put's visibility (remote path): once we observe the
            // page published, the landing-page bytes are safe to memcpy.
            std::uint32_t produced =
                std::atomic_ref<std::uint32_t>(
                    produced_beg_[w])
                    .load(std::memory_order_acquire);
            if (produced <= consumed)
            {
                return false;    // not ready: requeue and retry later
            }

            std::size_t const slot = consumed % slots_per_dst_;
            // Read from src's per-src ring in our shared rx pool
            // ((src_pe * slots + slot) * mtu_).
            std::size_t const rx_slot = (src_pe * slots_per_dst_ + slot) * mtu_;

            std::memcpy(out_buf, rx_beg_ + rx_slot, count);

            // return the credit: publish consumed (high-32; self owns it)
            // onto src so it may overwrite the slot we just drained.
            if (src_pe == my_pe_)
            {
                std::atomic_ref<std::uint32_t>(consumed_beg_[w])
                    .store(consumed + 1, std::memory_order_release);
            }
            else
            {
                // Release fence: the data-page memcpy above is a read of our
                // RNIC-facing landing buffers; fence before freeing the slot
                // so the credit put observes a fully-drained page (weak-memory
                // hosts).  GEX v0.19 has no gex_System_Fence(), so this C++
                // fence is the portable local barrier.  The credit word is
                // staged into a durable per-src slot (async put reads it after
                // this call) and its event recorded for the single-flight
                // gate above.
                std::atomic_thread_fence(std::memory_order_release);
                recv_credit_src_[src_pe] = consumed + 1;
                credit_events_[src_pe] =
                    hpx::util::gasnet_environment::put_uint32_nb(
                        static_cast<int>(src_pe),
                        remote_ptr(src_pe, &consumed_beg_[w]),
                        &recv_credit_src_[src_pe]);
            }
            consumed_locals_[src_pe].store(
                consumed + 1, std::memory_order_release);    // update mirror

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
        // that the chunks it collects all belong to one message.
        std::uint64_t next_message_id() noexcept
        {
            return message_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
        }

    private:
        // Stall tripwire: emit a diagnostic line every stall_warn_interval
        // cumulative back-pressure returns on a given (pair, direction), and
        // keep counting.  A single credit put lost to the network would
        // otherwise stall that pair forever, surfacing only as the do_stop()
        // 30s abort at shutdown.
        void note_send_stall(std::size_t const dst_pe) noexcept
        {
            if (++send_stall_[dst_pe] >= stall_warn_interval)
            {
                send_stall_[dst_pe] = 0;
                std::fprintf(stderr,
                    "gasnet::mailbox_array: PE %zu has had %llu back-pressure "
                    "polls (window full or NB region in flight) to PE %zu\n",
                    my_pe_, static_cast<unsigned long long>(stall_warn_interval),
                    dst_pe);
            }
        }

        void note_recv_stall(std::size_t const src_pe) noexcept
        {
            if (++recv_stall_[src_pe] >= stall_warn_interval)
            {
                recv_stall_[src_pe] = 0;
                std::fprintf(stderr,
                    "gasnet::mailbox_array: PE %zu has had %llu back-pressure "
                    "polls (credit-return region in flight) from PE %zu\n",
                    my_pe_, static_cast<unsigned long long>(stall_warn_interval),
                    src_pe);
            }
        }

        unsigned char* base_ = nullptr;
        std::size_t carved_bytes_ = 0;

        unsigned char* tx_beg_ = nullptr;
        unsigned char* tx_end_ = nullptr;
        unsigned char* rx_beg_ = nullptr;
        unsigned char* rx_end_ = nullptr;
        std::uint32_t* produced_beg_ = nullptr;
        std::uint32_t* produced_end_ = nullptr;
        std::uint32_t* consumed_beg_ = nullptr;
        std::uint32_t* consumed_end_ = nullptr;
        std::uint32_t* scratch_beg_ = nullptr;
        std::atomic<std::uint32_t>* produced_locals_ = nullptr;
        std::atomic<std::uint32_t>* consumed_locals_ = nullptr;
        mutable std::atomic<std::uint32_t>* scan_rotate_ = nullptr;

        std::size_t num_pes_ = 0;
        std::size_t my_pe_ = 0;
        std::size_t mtu_ = 0;
        std::size_t slots_per_dst_ = 0;
        bool checksum_ = true;
        std::size_t probe_window_ = default_probe_window;

        // Per-destination / per-source non-blocking RMA state:
        //   send_events_   : completion event of the single outstanding
        //                    data+credit access region per dst (GEX_EVENT_INVALID
        //                    when none); send_ready() reaps it.
        //   credit_events_ : completion event of the single outstanding
        //                    credit-return region per src.
        //   send_credit_src_ / recv_credit_src_: durable 32-bit words the
        //                    async puts read as their credit source, kept
        //                    alive until the region event completes.
        //   send_stall_ / recv_stall_: cumulative back-pressure counters
        //                    feeding the stall tripwire warnings.
        std::vector<gex_Event_t> send_events_;
        std::vector<gex_Event_t> credit_events_;
        std::vector<std::uint32_t> send_credit_src_;
        std::vector<std::uint32_t> recv_credit_src_;
        std::vector<std::uint64_t> send_stall_;
        std::vector<std::uint64_t> recv_stall_;

        std::atomic<std::uint64_t> message_sequence_{0};
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif