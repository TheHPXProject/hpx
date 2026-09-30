//  Copyright (c) 2023-2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/gasnet_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/util.hpp>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace hpx::util {

    namespace detail {

        bool detect_gasnet_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
            std::string gasnet_environment_strings =
                cfg.get_entry("hpx.parcel.gasnet.env", default_env);

            hpx::string_util::char_separator<char> sep(";,: ");
            hpx::string_util::tokenizer tokens(gasnet_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                char* env = std::getenv(tok.c_str());
                if (env)
                {
                    LBT_(debug)
                        << "Found GASNET environment variable: " << tok << "="
                        << std::string(env) << ", enabling GASNET support\n";
                    return true;
                }
            }

            LBT_(info)
                << "No known GASNET environment variable found, disabling "
                   "GASNET support\n";
            return false;
        }
    }    // namespace detail

    bool gasnet_environment::check_gasnet_environment(
        util::runtime_configuration const& cfg)
    {
#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_MODULE_GASNET_BASE)
        // The gasnet parcelport is used if it is not explicitly disabled
        // (hpx.parcel.gasnet.enable, default 1).
        if (!get_entry_as(cfg, "hpx.parcel.gasnet.enable", true))
        {
            LBT_(info) << "GASNET support disabled via configuration settings\n";
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    gasnet_environment::mutex_type gasnet_environment::mtx_{};

    bool gasnet_environment::enabled_ = false;
    bool gasnet_environment::has_called_init_ = false;

    gex_Client_t gasnet_environment::client_ = GEX_CLIENT_INVALID;
    gex_EP_t gasnet_environment::ep_ = GEX_EP_INVALID;
    gex_TM_t gasnet_environment::tm_ = GEX_TM_INVALID;
    gex_Segment_t gasnet_environment::segment_ = GEX_SEGMENT_INVALID;
    void* gasnet_environment::segment_addr_ = nullptr;
    std::size_t gasnet_environment::segment_size_ = 0;
    std::vector<void*> gasnet_environment::remote_segment_addrs_;
    std::size_t gasnet_environment::max_local_segment_size_ = 0;

    void gasnet_environment::init(
        int* argc, char*** argv, util::runtime_configuration& cfg)
    {
        scoped_lock l(mtx_);
        if (enabled_)
            return;    // don't call twice

        has_called_init_ = false;

        enabled_ = check_gasnet_environment(cfg);
        if (!enabled_)
        {
            cfg.add_entry("hpx.parcel.gasnet.enable", "0");
            return;
        }

        cfg.add_entry("hpx.parcel.bootstrap", "gasnet");

        // Create the GASNet-EX client, primordial endpoint and team.  This
        // is collective over the whole job and carries an implicit barrier.
        // The optional GEX_FLAG_USES_GASNET1 legacy support is deliberately
        // NOT requested: we only need gex_* calls plus the restricted-context
        // helpers gasnet_exit() and gasnet_getMaxLocalSegmentSize(), which
        // are available without it.
        if (gex_Client_Init(&client_, &ep_, &tm_, "HPX", argc, argv, 0) !=
            GASNET_OK)
        {
            // explicitly disable gasnet if not run by a GASNet job launcher
            cfg.add_entry("hpx.parcel.gasnet.enable", "0");
            enabled_ = false;

            throw std::runtime_error(
                "gasnet_environment::init: gex_Client_Init failed (is the "
                "application launched under a GASNet job launcher such as "
                "gasnetrun_*?)");
        }

        // The parcelport issues GASNet-EX calls concurrently from the parcels
        // io driver strands, which requires a thread-multiple (PAR/PARSYNC)
        // build of GASNet.  QueryMaxThreads() reports the number of threads
        // that may call the GASNet API inside this process (1 in a SEQ build),
        // so refuse to run under a single-threaded build, where concurrent
        // io-driver RMA would be undefined behavior.
        std::uint64_t const max_threads = gex_System_QueryMaxThreads();
        if (max_threads < 2)
        {
            cfg.add_entry("hpx.parcel.gasnet.enable", "0");
            enabled_ = false;
            throw std::runtime_error(
                "gasnet_environment::init: GASNet build does not support "
                "thread-multiple mode (gex_System_QueryMaxThreads() == " +
                std::to_string(max_threads) + "); a PAR gex build is required");
        }

        // Size of the single registered segment this locality attaches.  The
        // parcelport mailbox carves its tx/rx pools and credit matrices out
        // of the first bytes of this segment, so attaching at the largest
        // size the runtime permits guarantees the carve always fits.
        max_local_segment_size_ =
            static_cast<std::size_t>(gasnet_getMaxLocalSegmentSize());
        if (max_local_segment_size_ == 0)
        {
            // Some conduits may conservatively disallow segment attachment.
            cfg.add_entry("hpx.parcel.gasnet.enable", "0");
            enabled_ = false;
            throw std::runtime_error(
                "gasnet_environment::init: no usable registered segment "
                "(gasnet_getMaxLocalSegmentSize() == 0); the gasnet "
                "parcelport requires a registered segment for RMA");
        }

        // Collective attach of our (zero-initialized by the runtime, but we
        // still zero the mailbox carve later) segment, bound to the
        // primordial endpoint.  Implicit barrier on return.
        if (gex_Segment_Attach(&segment_, tm_, max_local_segment_size_) !=
            GASNET_OK)
        {
            cfg.add_entry("hpx.parcel.gasnet.enable", "0");
            enabled_ = false;
            throw std::runtime_error(
                "gasnet_environment::init: gex_Segment_Attach failed");
        }
        HPX_ASSERT(segment_ != GEX_SEGMENT_INVALID);

        segment_addr_ = gex_Segment_QueryAddr(segment_);
        segment_size_ =
            static_cast<std::size_t>(gex_Segment_QuerySize(segment_));
        HPX_ASSERT(segment_addr_ != nullptr);
        HPX_ASSERT(segment_size_ == max_local_segment_size_);

        // Discover the base address of every peer's segment in that peer's
        // own address space.  After the collective attach every primordial
        // segment is implicitly 'published', so the non-blocking query below
        // succeeds for every rank.  Each query event is waited before the
        // next query re-uses its OUT arguments.
        std::size_t const npes = static_cast<std::size_t>(size());
        remote_segment_addrs_.clear();
        remote_segment_addrs_.reserve(npes);
        for (std::size_t r = 0; r < npes; ++r)
        {
            void* owneraddr = nullptr;
            void* localaddr = nullptr;
            std::uintptr_t len = 0;
            gex_Event_t const ev = gex_EP_QueryBoundSegmentNB(
                tm_, static_cast<gex_Rank_t>(r), &owneraddr, &localaddr, &len,
                0);
            gex_Event_Wait(ev);
            if (gex_Event_Test(ev) != GASNET_OK || owneraddr == nullptr)
            {
                throw std::runtime_error(
                    std::string("gasnet_environment::init: "
                                "gex_EP_QueryBoundSegmentNB failed for rank ") +
                    std::to_string(r) + " (result " +
                    std::to_string(gex_Event_Test(ev)) + ")");
            }
            remote_segment_addrs_.push_back(owneraddr);
        }

        // Make sure every PE has finished attaching its segment, discovered
        // the peer addresses and zeroed its carve before any parcel traffic
        // is allowed to start (the parcelport re-uses this barrier in
        // do_run()).
        barrier();

        has_called_init_ = true;

        cfg.set_num_localities(static_cast<std::uint32_t>(npes));

        int const this_rank = rank();
        if (this_rank == 0)
        {
            cfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            cfg.mode_ = hpx::runtime_mode::worker;
        }

        cfg.add_entry("hpx.parcel.gasnet.rank", std::to_string(this_rank));
        cfg.add_entry(
            "hpx.parcel.gasnet.processorname", get_processor_name());
    }

    void gasnet_environment::finalize() noexcept
    {
        scoped_lock l(mtx_);
        if (enabled_ && has_called_init_)
        {
            has_called_init_ = false;

            // GASNet-EX in specification v0.19 has no gex_Client_Close();
            // gasnet_exit() is the standard (restricted-context) termination
            // call: it tears down the job, joining any progress threads.
            // Only one client per process, and this module owns it.
            gasnet_exit(0);

            remote_segment_addrs_.clear();
        }
    }

    bool gasnet_environment::enabled() noexcept
    {
        return enabled_;
    }

    bool gasnet_environment::has_called_init() noexcept
    {
        return has_called_init_;
    }

    int gasnet_environment::rank() noexcept
    {
        if (client_ == GEX_CLIENT_INVALID)
        {
            return -1;
        }
        return static_cast<int>(gex_System_QueryJobRank());
    }

    int gasnet_environment::size() noexcept
    {
        if (client_ == GEX_CLIENT_INVALID)
        {
            return -1;
        }
        return static_cast<int>(gex_System_QueryJobSize());
    }

    std::string gasnet_environment::get_processor_name()
    {
        return std::to_string(rank());
    }

    gex_Client_t gasnet_environment::client() noexcept
    {
        return client_;
    }

    gex_EP_t gasnet_environment::ep() noexcept
    {
        return ep_;
    }

    gex_TM_t gasnet_environment::tm() noexcept
    {
        return tm_;
    }

    gex_Segment_t gasnet_environment::segment() noexcept
    {
        return segment_;
    }

    void* gasnet_environment::segment_addr() noexcept
    {
        return segment_addr_;
    }

    std::size_t gasnet_environment::segment_size() noexcept
    {
        return segment_size_;
    }

    std::size_t gasnet_environment::max_local_segment_size() noexcept
    {
        return max_local_segment_size_;
    }

    void* gasnet_environment::remote_segment_addr(int rank) noexcept
    {
        if (rank < 0 || rank >= static_cast<int>(remote_segment_addrs_.size()))
        {
            return nullptr;
        }
        return remote_segment_addrs_[static_cast<std::size_t>(rank)];
    }

    void gasnet_environment::barrier() noexcept
    {
        gex_Event_t const ev = gex_Coll_BarrierNB(tm_, 0);
        gex_Event_Wait(ev);
        HPX_ASSERT(gex_Event_Test(ev) == GASNET_OK);
    }

    void gasnet_environment::put(
        int rank, void* raddr, void const* laddr, std::size_t nbytes)
    {
        HPX_ASSERT(tm_ != GEX_TM_INVALID);
        gex_RMA_PutBlocking(tm_, static_cast<gex_Rank_t>(rank), raddr,
            const_cast<void*>(laddr), nbytes, 0);
    }

    void gasnet_environment::get(
        int rank, void* laddr, void const* raddr, std::size_t nbytes)
    {
        HPX_ASSERT(tm_ != GEX_TM_INVALID);
        gex_RMA_GetBlocking(tm_, laddr, static_cast<gex_Rank_t>(rank),
            const_cast<void*>(raddr), nbytes, 0);
    }

    gex_Event_t gasnet_environment::put_data_and_credit_nb(int rank, void* raddr,
        void const* laddr, std::size_t nbytes, void* credit_raddr,
        void const* credit_laddr)
    {
        HPX_ASSERT(tm_ != GEX_TM_INVALID);

        // The credit word is read by the asynchronous NBI put from
        // 'credit_laddr', so the caller must keep that word unchanged until
        // the returned region event completes (poll_event() == true).
        gex_NBI_BeginAccessRegion(0);

        // Ordering is the whole point: the credit word is performed at the
        // target only after the data bytes have been performed there, so a
        // receiver that observes produced>buffered sees the complete page.
        // Both destinations live in the target's registered segment, which
        // is exactly what GASNet requires for remote RMA targets.
        gex_RMA_PutNBI(tm_, static_cast<gex_Rank_t>(rank), raddr,
            const_cast<void*>(laddr), nbytes, nullptr, 0);
        gex_RMA_PutNBI(tm_, static_cast<gex_Rank_t>(rank), credit_raddr,
            const_cast<void*>(credit_laddr), sizeof(std::uint32_t), nullptr, 0);

        return gex_NBI_EndAccessRegion(0);
    }

    gex_Event_t gasnet_environment::put_uint32_nb(
        int rank, void* raddr, void const* laddr)
    {
        HPX_ASSERT(tm_ != GEX_TM_INVALID);

        gex_NBI_BeginAccessRegion(0);
        gex_RMA_PutNBI(tm_, static_cast<gex_Rank_t>(rank), raddr,
            const_cast<void*>(laddr), sizeof(std::uint32_t), nullptr, 0);
        return gex_NBI_EndAccessRegion(0);
    }

    void gasnet_environment::put_uint32(int rank, void* raddr,
        std::uint32_t value)
    {
        HPX_ASSERT(tm_ != GEX_TM_INVALID);

        std::uint32_t const tmp = value;
        gex_RMA_PutBlocking(
            tm_, static_cast<gex_Rank_t>(rank), raddr,
            const_cast<std::uint32_t*>(&tmp), sizeof(tmp), 0);
    }

    bool gasnet_environment::poll_event(gex_Event_t ev) noexcept
    {
        HPX_ASSERT(ev != nullptr);
        return gex_Event_Test(ev) == GASNET_OK;
    }
}    // namespace hpx::util