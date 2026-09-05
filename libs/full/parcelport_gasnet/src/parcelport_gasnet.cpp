//  Copyright (c) 2026 Tactical Computing Labs, LLC (Christopher Taylor)
//  Copyright (c) 2023 Christopher Taylor
//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/gasnet_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/parcelport_gasnet/locality.hpp>
#include <hpx/parcelport_gasnet/mailbox.hpp>
#include <hpx/parcelport_gasnet/parcelport_gasnet.hpp>
#include <hpx/parcelport_gasnet/receiver.hpp>
#include <hpx/parcelport_gasnet/sender.hpp>
#include <hpx/parcelport_gasnet/sender_connection.hpp>
#include <hpx/parcelset/parcelport_impl.hpp>
#include <hpx/plugin_factories/parcelport_factory.hpp>

#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/version.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace policies::gasnet {
        class HPX_EXPORT parcelport;
    }    // namespace policies::gasnet

    template <>
    struct connection_handler_traits<policies::gasnet::parcelport>
    {
        using connection_type = policies::gasnet::sender_connection;
        using send_early_parcel = std::true_type;
        using do_background_work = std::true_type;
        using send_immediate_parcels = std::true_type;
        using is_connectionless = std::true_type;

        static constexpr char const* type() noexcept
        {
            return "gasnet";
        }

        static constexpr char const* pool_name() noexcept
        {
            return "parcel-pool-gasnet";
        }

        static constexpr char const* pool_name_postfix() noexcept
        {
            return "-gasnet";
        }
    };

    namespace policies::gasnet {

        // Credit/wakeup AM handler (SHORT, 2-arg request). Runs inside the
        // transport's poll/dispatch on the single progress thread and writes
        // the LOCAL copy of the produced/consumed counter that our progress
        // loop reads -- the GASNet-EX equivalent of the OpenSHMEM remote
        // atomic_set. Dispatches through the file-scope static mailbox pointer
        // set once during initialization.
        void credit_am_handler(
            gex_Token_t /*token*/, gex_AM_Arg_t a0, gex_AM_Arg_t a1) noexcept
        {
            mailbox* m = get_gasnet_mailbox_ptr();
            if (m != nullptr)
            {
                m->handle_credit(
                    static_cast<std::uint32_t>(a0),
                    static_cast<std::uint32_t>(a1));
            }
        }

        class HPX_EXPORT parcelport : public parcelport_impl<parcelport>
        {
            using base_type = parcelport_impl<parcelport>;

            static parcelset::locality here(std::size_t my_pe)
            {
                return parcelset::locality(
                    locality(static_cast<std::int32_t>(my_pe)));
            }

            static std::size_t mtu(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.gasnet.mtu", HPX_PARCEL_GASNET_MTU);
            }

        public:
            using sender_type = sender;
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(0), notifier)
              , stopped_(false)
              , num_pes_(gasnet_environment::size())
              , my_pe_(gasnet_environment::rank())
              , mtu_(mtu(ini))
              , mailboxes_(num_pes_, my_pe_, mtu_)
              , sender_(&mailboxes_)
              , receiver_(*this)
            {
                here_ = here(my_pe_);
            }

            parcelport(parcelport const&) = delete;
            parcelport(parcelport&&) = delete;
            parcelport& operator=(parcelport const&) = delete;
            parcelport& operator=(parcelport&&) = delete;

            ~parcelport() override = default;

            bool do_run()
            {
                gex_TM_t tm = gasnet_environment::get_team();
                gex_EP_t ep = gasnet_environment::get_endpoint();

                // Point the AM handler at our mailbox before any inbound
                // credit message can be dispatched by a poll.
                get_gasnet_mailbox_ptr() = &mailboxes_;

                // (1) Register the credit/wakeup AM handler on our endpoint.
                //     gex_AM_Entry_t.gex_index == 0 means "assign an index";
                //     GEX returns the absolute client index (>= 128) which we
                //     read back and use in all gex_AM_RequestShort2 calls.
                gex_AM_Entry_t htable[1];
                htable[0].gex_index = 0;
                htable[0].gex_fnptr = (gex_AM_Fn_t) &credit_am_handler;
                htable[0].gex_flags = GEX_FLAG_AM_REQUEST | GEX_FLAG_AM_SHORT;
                htable[0].gex_nargs = 2;
                htable[0].gex_cdata = nullptr;
                htable[0].gex_name = "hpx_gasnet_credit";

                int rc = gex_EP_RegisterHandlers(ep, htable, 1);
                if (rc != GASNET_OK)
                {
                    HPX_THROW_EXCEPTION(error::network_error,
                        "gasnet::parcelport::do_run",
                        "gex_EP_RegisterHandlers failed: " +
                            std::string(gasnet_ErrorName(rc)));
                }
                mailboxes_.set_credit_handler(htable[0].gex_index);

                // (2) Collectively publish the bound segment so every peer
                //     has RMA credentials for one-sided puts into it.
                //     This call is collective over the team and therefore
                //     also synchronizes the whole base-exchange handshake.
                gex_EP_t eps[1] = {ep};
                rc = gex_EP_PublishBoundSegment(tm, eps, 1, 0);
                if (rc != GASNET_OK)
                {
                    HPX_THROW_EXCEPTION(error::network_error,
                        "gasnet::parcelport::do_run",
                        "gex_EP_PublishBoundSegment failed: " +
                            std::string(gasnet_ErrorName(rc)));
                }

                // (3) Query each peer's segment base (its owner-address). The
                //     GEX address model uses absolute remote addresses, so we
                //     reconstruct base_of(peer) + offset for every RMA put.
                gex_Rank_t const npes = gex_TM_QuerySize(tm);
                for (gex_Rank_t r = 0; r != npes; ++r)
                {
                    void* owneraddr = nullptr;
                    void* localaddr = nullptr;
                    uintptr_t segsize = 0;

                    gex_Event_t ev = gex_EP_QueryBoundSegmentNB(
                        tm, r, &owneraddr, &localaddr, &segsize, 0);
                    gex_Event_Wait(ev);

                    if (owneraddr == nullptr || segsize == 0)
                    {
                        HPX_THROW_EXCEPTION(error::network_error,
                            "gasnet::parcelport::do_run",
                            "gex_EP_QueryBoundSegmentNB returned no segment "
                            "for peer " +
                                std::to_string(r));
                    }
                    mailboxes_.set_remote_base(static_cast<std::size_t>(r),
                        reinterpret_cast<std::uintptr_t>(owneraddr));
                }

                sender_.run();
                receiver_.run();

                // All GASNet-EX calls must stay on a single thread (we drive
                // the transport without internal locking), so only the first
                // io_service of the pool runs the progress loop (io_pool_size
                // is forced to 1 by the configuration below).
#if ASIO_VERSION >= 103400
                ::asio::post(
                    io_service_pool_.get_io_service(0),
                    hpx::bind(&parcelport::io_service_work, this));
#else
                io_service_pool_.get_io_service(0)
                    .post(hpx::bind(&parcelport::io_service_work, this));
#endif
                return true;
            }

            void do_stop()
            {
                // Wait for the progress thread to drain all queued work. We
                // must not drive send/receive from this thread as that would
                // call into GASNet outside the single progress thread.
                std::size_t max_iter = 1000;
                while (sender_.has_pending() || receiver_.has_pending())
                {
                    if (!threads::get_self_ptr() || max_iter-- == 0)
                        break;
                    hpx::this_thread::suspend(
                        hpx::threads::thread_schedule_state::pending,
                        "gasnet::parcelport::do_stop");
                }

                stopped_.store(true, std::memory_order_release);
            }

            std::string get_locality_name() const override
            {
                return std::to_string(my_pe_);
            }

            std::shared_ptr<sender_connection> create_connection(
                parcelset::locality const& l, error_code&)
            {
                int const dest_rank = l.get<locality>().rank();
                return sender_.create_connection(dest_rank, &mailboxes_);
            }

            parcelset::locality agas_locality(
                util::runtime_configuration const&) const override
            {
                return parcelset::locality(locality(0));
            }

            parcelset::locality create_locality() const override
            {
                return parcelset::locality(locality());
            }

            // All GASNet-EX calls happen on the single progress thread
            // (io_service_work). HPX threads must not call into the GASNet
            // transport, so this is a no-op.
            bool background_work(
                std::size_t, parcelport_background_mode)
            {
                return false;
            }

            constexpr bool can_send_immediate() const noexcept
            {
                return true;
            }

            mailbox const& get_mailboxes() const noexcept
            {
                return mailboxes_;
            }

            mailbox& get_mailboxes() noexcept
            {
                return mailboxes_;
            }

            constexpr std::size_t num_pes() const noexcept
            {
                return num_pes_;
            }

            constexpr std::size_t my_pe() const noexcept
            {
                return my_pe_;
            }

            constexpr std::size_t mtu() const noexcept
            {
                return mtu_;
            }

            bool send_immediate(parcelset::parcelport* pp,
                parcelset::locality const& dest,
                sender::parcel_buffer_type buffer,
                sender::callback_fn_type&& callbackFn)
            {
                (void) pp;
                return sender_.send_immediate(
                    dest, HPX_MOVE(buffer), HPX_MOVE(callbackFn));
            }

        private:
            void io_service_work()
            {
                std::size_t k = 0;
                std::size_t busy_loop = 0;
                constexpr std::size_t max_busy_loop = 1000;

                while (!stopped_.load(std::memory_order_acquire))
                {
                    // Check for incoming data first (dispatch inbound credit
                    // AMs + non-blocking scan + fast receive_()) so that
                    // incoming parcels are not starved by a blocking send on
                    // the other side.
                    bool has_work = receiver_.background_work();
                    has_work = sender_.background_work() || has_work;
                    if (has_work)
                    {
                        k = 0;
                        busy_loop = 0;
                    }
                    else
                    {
                        if (busy_loop < max_busy_loop)
                        {
                            ++busy_loop;
                        }
                        else
                        {
                            // Deliberate hot spin (no OS yield): mirrors the
                            // validated openshmem harness, where the receiver
                            // continuously pumps the transport so an inbound
                            // credit AM / RMA store is reliably observed.
                            ++k;
                        }
                    }
                }
            }

            std::atomic<bool> stopped_;

            std::size_t num_pes_;
            std::size_t my_pe_;
            std::size_t mtu_;
            mailbox mailboxes_;

            sender sender_;
            receiver<parcelport> receiver_;
        };
    }    // namespace policies::gasnet
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

template <>
struct hpx::traits::plugin_config_data<
    hpx::parcelset::policies::gasnet::parcelport>
{
    static constexpr char const* priority() noexcept
    {
        return "100";
    }

    static void init(int* argc, char*** argv, util::command_line_handling& cfg)
    {
        util::gasnet_environment::init(argc, argv, cfg.rtcfg_);
        cfg.num_localities_ =
            static_cast<std::size_t>(util::gasnet_environment::size());
        cfg.node_ =
            static_cast<std::size_t>(util::gasnet_environment::rank());
    }

    static constexpr void init(hpx::resource::partitioner&) noexcept {}

    static void destroy() noexcept
    {
        util::gasnet_environment::finalize();
    }

    static constexpr char const* call() noexcept
    {
        return "mtu = "
               "${HPX_HAVE_PARCELPORT_GASNET_MTU:65536}\n"
               // All GASNet-EX calls must happen on a single thread, so the
               // pool must not be larger than one.
               "io_pool_size = 1\n";
    }
};

HPX_REGISTER_PARCELPORT(
    hpx::parcelset::policies::gasnet::parcelport, gasnet)

#endif