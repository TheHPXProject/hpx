//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/openshmem_base/openshmem_environment.hpp>
#include <hpx/parcelport_openshmem/locality.hpp>
#include <hpx/parcelport_openshmem/mailbox_array.hpp>
#include <hpx/parcelport_openshmem/receiver.hpp>
#include <hpx/parcelport_openshmem/sender.hpp>
#include <hpx/parcelport_openshmem/sender_connection.hpp>
#include <hpx/parcelset/parcelport_impl.hpp>
#include <hpx/plugin_factories/parcelport_factory.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

#include <shmem.h>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace policies::openshmem {
        class HPX_EXPORT parcelport;
    }    // namespace policies::openshmem

    template <>
    struct connection_handler_traits<policies::openshmem::parcelport>
    {
        using connection_type = policies::openshmem::sender_connection;
        using send_early_parcel = std::true_type;
        using do_background_work = std::true_type;
        using send_immediate_parcels = std::true_type;
        using is_connectionless = std::true_type;

        static constexpr char const* type() noexcept
        {
            return "openshmem";
        }

        static constexpr char const* pool_name() noexcept
        {
            return "parcel-pool-openshmem";
        }

        static constexpr char const* pool_name_postfix() noexcept
        {
            return "-openshmem";
        }
    };

    namespace policies::openshmem {

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
                    ini, "hpx.parcel.openshmem.mtu", HPX_PARCEL_OPENSHMEM_MTU);
            }

        public:
            using sender_type = sender;
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(0), notifier)
              , stopped_(false)
              , num_pes_(static_cast<std::size_t>(shmem_n_pes()))
              , my_pe_(static_cast<std::size_t>(shmem_my_pe()))
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
                sender_.run();
                receiver_.run();

                // All shmem_* calls must stay on a single thread to satisfy
                // SHMEM_THREAD_SERIALIZED, so only the first io_service of
                // the pool runs the progress loop (io_pool_size is forced to
                // 1 by the configuration below).
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
                // call shmem_* outside the single progress thread.
                std::size_t max_iter = 1000;
                while (sender_.has_pending() || receiver_.has_pending())
                {
                    if (!threads::get_self_ptr() || max_iter-- == 0)
                        break;
                    hpx::this_thread::suspend(
                        hpx::threads::thread_schedule_state::pending,
                        "openshmem::parcelport::do_stop");
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

            // All shmem_* calls happen on the single progress thread
            // (io_service_work). HPX threads must not call into the shmem
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

            mailbox_array const& get_mailboxes() const noexcept
            {
                return mailboxes_;
            }

            mailbox_array& get_mailboxes() noexcept
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
                    // Check for incoming data first (non-blocking scan +
                    // fast receive_()) so that incoming parcels are not
                    // starved by a blocking send on the other side.
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
                            // validated shmem_mailbox_credits_with_pages
                            // harness, where the receiver continuously pumps
                            // the transport so a remote atomic_set produced is
                            // reliably pulled into this PE's view. Yielding
                            // starves OSHMEM/UCX progress and the pending
                            // cross-PE store is never observed.
                            ++k;
                        }
                    }
                }
            }

            std::atomic<bool> stopped_;

            std::size_t num_pes_;
            std::size_t my_pe_;
            std::size_t mtu_;
            mailbox_array mailboxes_;

            sender sender_;
            receiver<parcelport> receiver_;
        };
    }    // namespace policies::openshmem
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

template <>
struct hpx::traits::plugin_config_data<
    hpx::parcelset::policies::openshmem::parcelport>
{
    static constexpr char const* priority() noexcept
    {
        return "100";
    }

    static void init(int* argc, char*** argv, util::command_line_handling& cfg)
    {
        util::openshmem_environment::init(argc, argv, cfg.rtcfg_);
        cfg.num_localities_ =
            static_cast<std::size_t>(util::openshmem_environment::size());
        cfg.node_ =
            static_cast<std::size_t>(util::openshmem_environment::rank());
    }

    static constexpr void init(hpx::resource::partitioner&) noexcept {}

    static void destroy() noexcept
    {
        util::openshmem_environment::finalize();
    }

    static constexpr char const* call() noexcept
    {
        return "mtu = "
               "${HPX_HAVE_PARCELPORT_OPENSHMEM_MTU:65536}\n"
               // SHMEM_THREAD_SERIALIZED requires all shmem_* calls to
               // happen on a single thread, so the pool must not be larger
               // than one.
               "io_pool_size = 1\n";
    }
};

HPX_REGISTER_PARCELPORT(
    hpx::parcelset::policies::openshmem::parcelport, openshmem)

#endif
