//  Copyright (c) 2023-2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)) ||   \
    defined(HPX_HAVE_MODULE_GASNET_BASE)

#include <hpx/gasnet_base/gasnet.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/synchronization.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    // Wrapper around the GASNet-EX client lifetime and the registered
    // segment shared by every locality, plus the small set of RMA
    // primitives the gasnet parcelport builds on.
    //
    // All RMA helpers funnel through gex_RMA_* on the primordial team
    // created by gex_Client_Init().  Ordering between a data transfer and
    // the credit word that publishes it is provided by an NBI access
    // region (gex_NBI_BeginAccessRegion()/..EndAccessRegion()+gex_Event_Wait):
    // operations issued inside the region are performed in order at the
    // target, which stands in for putmem + shmem_fence + atomic_set in the
    // openshmem design this parcelport mirrors.
    struct HPX_CORE_EXPORT gasnet_environment
    {
        static bool check_gasnet_environment(runtime_configuration const& cfg);

        static void init(int* argc, char*** argv, runtime_configuration& cfg);
        static void finalize() noexcept;

        static bool enabled() noexcept;
        static bool has_called_init() noexcept;

        static int rank() noexcept;
        static int size() noexcept;

        static std::string get_processor_name();

        // GESNet-EX objects handed back by gex_Client_Init().
        static gex_Client_t client() noexcept;
        static gex_EP_t ep() noexcept;
        static gex_TM_t tm() noexcept;

        // The single registered segment attached on every locality in
        // init().  Its base address and size are identical in *semantics* on
        // every PE (the same carve is applied at offset 0 on each PE), but
        // the base addresses generally differ between locality address
        // spaces.  Remote RMA targets are computed as
        // remote_segment_addr(rank) + local_offset.
        static gex_Segment_t segment() noexcept;
        static void* segment_addr() noexcept;
        static std::size_t segment_size() noexcept;

        // Largest size (bytes) gex_Segment_Attach() accepts on this runtime.
        static std::size_t max_local_segment_size() noexcept;

        // Base address of the segment attached by 'rank' in that rank's own
        // address space (queried once during init() via
        // gex_EP_QueryBoundSegmentNB()).  May be nullptr if unavailable.
        static void* remote_segment_addr(int rank) noexcept;

        // Collective barrier (gex_Coll_BarrierNB + gex_Event_Wait) over the
        // primordial team.
        static void barrier() noexcept;

        // RMA: blocking put of 'nbytes' from local 'laddr' to remote
        // 'raddr' on 'rank'.
        static void put(
            int rank, void* raddr, void const* laddr, std::size_t nbytes);

        // RMA: blocking get of 'nbytes' from remote 'raddr' on 'rank' into
        // local 'laddr'.
        static void get(int rank, void* laddr, void const* raddr,
            std::size_t nbytes);

        // Ordered data + credit publish, non-blocking.  Transfers 'nbytes'
        // of data to 'raddr' and then writes the 4-byte credit word to
        // 'credit_raddr' on the same destination, both inside a single NBI
        // access region, and returns the region's completion event without
        // waiting for it.  The credit value is performed at the target only
        // after the data, which is the guarantee the mailbox_array
        // single-writer credit protocol needs (GASNet substitute for putmem;
        // shmem_fence; atomic_set).  'laddr' must stay untouched and
        // 'credit_laddr' must keep its value until poll_event() reports the
        // returned event complete.
        static gex_Event_t put_data_and_credit_nb(int rank, void* raddr,
            void const* laddr, std::size_t nbytes, void* credit_raddr,
            void const* credit_laddr);

        // Non-blocking 4-byte put (the receiver's credit-return path).
        // Returns the put's local-completion event; 'laddr' must keep its
        // value until poll_event() reports the event complete.
        static gex_Event_t put_uint32_nb(
            int rank, void* raddr, void const* laddr);

        // Blocking 4-byte put (used off the transport progress path).
        static void put_uint32(int rank, void* raddr, std::uint32_t value);

        // Non-blocking completion probe for the events returned by the
        // non-blocking puts above.  Returns true once the operation has
        // completed (the event is then consumed), false while it is still in
        // flight.  Never blocks.
        static bool poll_event(gex_Event_t ev) noexcept;

        using mutex_type = hpx::spinlock;
        using scoped_lock = std::unique_lock<mutex_type>;

    private:
        static mutex_type mtx_;

        static bool enabled_;
        static bool has_called_init_;

        static gex_Client_t client_;
        static gex_EP_t ep_;
        static gex_TM_t tm_;
        static gex_Segment_t segment_;
        static void* segment_addr_;
        static std::size_t segment_size_;
        static std::vector<void*> remote_segment_addrs_;
        static std::size_t max_local_segment_size_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#else

#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {
    struct HPX_CORE_EXPORT gasnet_environment
    {
        static bool check_gasnet_environment(runtime_configuration const& cfg);
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#endif