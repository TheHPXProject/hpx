//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <shmem.h>

namespace hpx::parcelset::policies::openshmem {

    // TX/RX page-arena + per-pair 64-bit credit-word protocol
    //
    // ONE zeroed symmetric allocation (shmem_calloc) holds everything:
    //     tx       : npes * SLOTS * mtu   (my outbound staging/scratch pages)
    //     rx       : npes * SLOTS * mtu   (my shared inbound landing pages)
    //     produced : npes * npes * 4      (uint32, low-32 of per-pair word)
    //     consumed : npes * npes * 4      (uint32, high-32 of per-pair word)
    //   -> one 64-bit credit per (sender i, receiver j): low32 = produced,
    //      high32 = consumed.  Payload lives in the tx/rx page pools which
    //      are O(npes * SLOTS) and linear in the number of PEs; the npes^2
    //      credit words are tiny 4-byte atomics (no npes^2 page grid).
    //
    // Slot-writer ownership (avoids torn writes with only atomic_set):
    //     produced[i*npes+j] written by sender (tx) i onto j;  receiver j reads local
    //     consumed[i*npes+j] written by receiver (rx) j onto i; sender i reads local
    //   -> each 32-bit half has a single writer; no RMW needed.
    //
    // Validated (2-PE) transport rule:
    //   - each side reads its associated counter locally (plain load)
    //   - each side remotely writes the peer's counter via atomic_set(..., peer)
    //   - both sides atomic_fetch the peer purely to force delivery of the
    //     peer's inbound stores into local symmetric memory (send queue)
    //   - data carried by blocking putmem.  NO atomic_*_nbi / fetch_add.
    //
    // Receiver j's landing page for a page written by sender i lives in j's
    // shared rx pool at offset (i*SLOTS + (p % SLOTS)) * mtu.  Each sender is
    // assigned its own SLOTS-sized slot range, so concurrent senders never
    // reuse the same landing slot (per-destination single-flight, which the
    // parcelport enforces via active_dsts_, keeps this safe).
    //
    // One MTU-sized page == one chunk (message_header + payload), matching
    // how sender_connection stages a chunk into an mtu slot.

    class HPX_EXPORT mailbox_array
    {
    public:
        // Number of in-flight pages (chunks) allowed per (sender,receiver)
        // pair before the sender must wait for consumed credit.  Bounded by
        // the rx page range assigned to each sender.
        static constexpr std::size_t slots_per_dst = 8;

        mailbox_array() = default;

        mailbox_array(std::size_t num_pes, std::size_t my_pe, std::size_t mtu)
          : num_pes_(num_pes)
          , my_pe_(my_pe)
          , mtu_(mtu)
        {
            std::size_t const pages = num_pes_ * slots_per_dst * mtu_;
            std::size_t const words = num_pes_ * num_pes_;

            // one zeroed allocation; zeroed produced/consumed = correct
            // monotonic start state (0 pages produced/consumed).
            std::size_t const bytes =
                2 * pages + 2 * words * sizeof(std::uint32_t);

            heap_ = shmem_calloc(bytes, 1);
            if (!heap_)
            {
                std::fprintf(stderr,
                    "openshmem: shmem_calloc(%zu) failed on PE %zu\n", bytes,
                    my_pe_);
                std::abort();
            }

            unsigned char* base = static_cast<unsigned char*>(heap_);
            tx_beg_ = base;
            tx_end_ = base + pages;
            rx_beg_ = tx_end_;
            rx_end_ = rx_beg_ + pages;
            produced_beg_ = reinterpret_cast<std::uint32_t*>(rx_end_);
            produced_end_ = produced_beg_ + words;
            consumed_beg_ = produced_end_;
            consumed_end_ = consumed_beg_ + words;

            // Publish the zeroed state to all PEs before any peer drives
            // progress against us.
            shmem_barrier_all();

            // Local-only mirror counters (never symmetric, never remote).
            // produced_locals_[dst] = how many pages I have published to dst
            // consumed_locals_[src] = how many pages I have consumed from src
            //
            // These persist across send()/receive_() calls.  The symmetric
            // produced_beg_/consumed_beg_ copies are only updated by REMOTE
            // atomic_set requests, so a PE's own local copy of a counter it
            // writes stays stale — hence the mirrors.
            produced_locals_ = new std::uint32_t[num_pes_];
            consumed_locals_ = new std::uint32_t[num_pes_];
            for (std::size_t i = 0; i < num_pes_; ++i)
            {
                produced_locals_[i] = 0;
                consumed_locals_[i] = 0;
            }
        }

        ~mailbox_array()
        {
            delete[] produced_locals_;
            delete[] consumed_locals_;
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
          , produced_locals_(other.produced_locals_)
          , consumed_locals_(other.consumed_locals_)
          , num_pes_(other.num_pes_)
          , my_pe_(other.my_pe_)
          , mtu_(other.mtu_)
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
            other.produced_locals_ = nullptr;
            other.consumed_locals_ = nullptr;
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
                produced_locals_ = other.produced_locals_;
                consumed_locals_ = other.consumed_locals_;
                num_pes_ = other.num_pes_;
                my_pe_ = other.my_pe_;
                mtu_ = other.mtu_;
                other.heap_ = nullptr;
                other.tx_beg_ = nullptr;
                other.tx_end_ = nullptr;
                other.rx_beg_ = nullptr;
                other.rx_end_ = nullptr;
                other.produced_beg_ = nullptr;
                other.produced_end_ = nullptr;
                other.consumed_beg_ = nullptr;
                other.consumed_end_ = nullptr;
                other.produced_locals_ = nullptr;
                other.consumed_locals_ = nullptr;
            }
            return *this;
        }

        // Return the symmetric scratch page that a sender uses to stage the
        // chunk for the current credit slot of (dst_pe). Each ring slot has
        // its own dedicated staging page (per-slot TX pages).
        unsigned char* tx_page(std::size_t dst_pe) const
        {
            std::size_t const slot =
                produced_locals_[dst_pe] % slots_per_dst;
            return tx_beg_ + (dst_pe * slots_per_dst + slot) * mtu_;
        }

        unsigned char* get_buffer(std::size_t pe) const
        {
            return tx_page(pe);
        }

        // Non-blocking scan: return the index of the first PE that has at
        // least one page we have not yet consumed, or -1 if none.
        //
        // Uses shmem_uint32_atomic_fetch, a remote atomic read of src's
        // produced counter, which (a) returns src's latest published value
        // and (b) drives delivery of src's inbound stores into our local
        // memory — the mechanism that makes a plain later local load see the
        // data.  Without this remote read, an idle receiver never triggers
        // delivery and would deadlock.
        int try_detect_pe_notification() const noexcept
        {
            for (std::size_t src = 0; src < num_pes_; ++src)
            {
                if (src == my_pe_)
                {
                    continue;
                }
                std::size_t const w = src * num_pes_ + my_pe_;

                // Pull delivery of src's inbound stores (putmem + produced
                // atomic_set) into the local symmetric memory, then read the
                // published value locally.  A remote shmem_uint32_atomic_fetch
                // return value would read src's own copy of the slot (which
                // is 0), not our copy.
                progress_to(src);
                std::uint32_t const produced = produced_beg_[w];
                std::uint32_t const consumed = consumed_locals_[src];
                if (produced > consumed)
                {
                    return static_cast<int>(src);
                }
            }
            return -1;
        }

        // progress_to(peer): a remote atomic_fetch to the peer "pushes" the
        // peer's inbound stores into our local symmetric memory (the delivery
        // rule).  Called inside every blocking wait.
        void progress_to(std::size_t const peer) const noexcept
        {
            std::size_t const w = my_pe_ * num_pes_ + peer;
            shmem_uint32_atomic_fetch(
                &produced_beg_[w], static_cast<int>(peer));
            shmem_uint32_atomic_fetch(
                &consumed_beg_[w], static_cast<int>(peer));
        }

        // Blocking send of one mtu-sized page (chunk) to dst_pe.
        // Copies 'count' bytes from our staging page (get_buffer()) into
        // dst's shared rx slot range, publishes produced, and applies the
        // credit wait (may keep up to slots_per_dst pages in flight).
        void send(std::size_t const dst_pe, std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            // Works for both remote (dst != me) and local (dst == me, a
            // self-putmem into our own rx pool) destinations.
            std::size_t const w = my_pe_ * num_pes_ + dst_pe;

            // mirrors of the two halves
            std::uint32_t produced = produced_locals_[dst_pe];    // mine

            // dst's consumed counter: for a local send it is our own drain
            // mirror (same thread); otherwise the delivered remote copy.
            std::uint32_t consumed = (dst_pe == my_pe_) ?
                consumed_locals_[dst_pe] :
                consumed_beg_[w];

            // credit wait: may write while produced - consumed < slots_per_dst
            while (static_cast<int>(produced - consumed) >=
                static_cast<int>(slots_per_dst))
            {
                if (dst_pe != my_pe_)
                {
                    progress_to(dst_pe);            // deliver dst's consumed
                    consumed = consumed_beg_[w];    // read local (delivered)
                }
                else
                {
                    consumed = consumed_locals_[dst_pe];    // our drain mirror
                }
            }

            std::size_t const slot = produced % slots_per_dst;

            // Staging source: my per-dst staging page (tx mirror of the rx
            // slot), keyed by dst_pe so two different destinations use
            // disjoint staging pages (matches tx_page()).
            std::size_t const stage_slot =
                (dst_pe * slots_per_dst + slot) * mtu_;

            // Landing target: dst's shared rx pool, keyed by self PE (the
            // sender), so each sender has its own disjoint per-src ring and
            // different senders cannot collide on the same guard pages (the
            // cause of duplicate delivery at 4 PEs).
            std::size_t const rx_slot = (my_pe_ * slots_per_dst + slot) * mtu_;

            // data: blocking putmem staging -> dst's rx page.
            shmem_putmem(rx_beg_ + rx_slot, tx_beg_ + stage_slot, count,
                static_cast<int>(dst_pe));

            // publish produced (low-32; self owns it)
            produced = produced + 1;
            shmem_uint32_atomic_set(
                &produced_beg_[w], produced, static_cast<int>(dst_pe));
            produced_locals_[dst_pe] = produced;    // update our mirror
            shmem_fence();

            // delivery (push): issue a remote atomic_fetch to the peer right
            // after publishing produced, so our inbound stores (the
            // atomic_set we just posted onto dst) are forced into dst's local
            // view — the same ping-pong the validated harness relies on when
            // both sides continuously fetch each other. Without this, an idle
            // receiver's local load/scan may never observe dst's copy.
            if (dst_pe != my_pe_)
            {
                progress_to(dst_pe);
            }
        }

        // Blocking receive of one page (chunk) from src_pe.  Copies 'count'
        // bytes off our shared rx page into out_buf, then returns the credit
        // (publishes consumed) so src may reuse the slot.
        bool receive_(std::size_t const src_pe, unsigned char* const out_buf,
            std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            std::size_t const w = src_pe * num_pes_ + my_pe_;

            std::uint32_t const consumed = consumed_locals_[src_pe];    // mine

            // wait for src to publish page 'consumed' (data ready)
            while (produced_beg_[w] <= consumed)
            {
                progress_to(src_pe);    // deliver src's produced
            }

            std::size_t const slot = consumed % slots_per_dst;
            // Read from src's per-src ring in our shared rx pool
            // ((src_pe * slots + slot) * mtu_).  The sender keys its landing
            // ring by its own pe (== our src_pe), so each sender has a
            // disjoint ring and a chunk from one source is never
            // aliased/overwritten by a different source (the duplicate
            // delivery / duplicate_component_id bug at 4 PEs).
            std::size_t const rx_slot = (src_pe * slots_per_dst + slot) * mtu_;

            std::memcpy(out_buf, rx_beg_ + rx_slot, count);

            // return the credit: publish consumed (high-32; self owns it) onto src
            shmem_uint32_atomic_set(
                &consumed_beg_[w], consumed + 1, static_cast<int>(src_pe));
            consumed_locals_[src_pe] = consumed + 1;    // update our mirror
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
            return receive_(static_cast<std::size_t>(pe), output, count);
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
        std::uint32_t* produced_locals_ = nullptr;
        std::uint32_t* consumed_locals_ = nullptr;

        std::size_t num_pes_ = 0;
        std::size_t my_pe_ = 0;
        std::size_t mtu_ = 0;
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
