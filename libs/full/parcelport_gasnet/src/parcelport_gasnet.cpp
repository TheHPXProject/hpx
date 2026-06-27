//  Copyright (c) 2023      Christopher Taylor
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2020 Google
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

#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/parcelport_gasnet/locality.hpp>
#include <hpx/parcelport_gasnet/receiver.hpp>
#include <hpx/parcelport_gasnet/sender.hpp>

#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <system_error>
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
        using send_immediate_parcels = std::false_type;
        using is_connectionless = std::false_type;

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

        int acquire_tag(sender* s) noexcept
        {
            return s->acquire_tag();
        }

        void add_connection(
            sender* s, std::shared_ptr<sender_connection> const& ptr)
        {
            s->add(ptr);
        }

        class HPX_EXPORT parcelport : public parcelport_impl<parcelport>
        {
            using base_type = parcelport_impl<parcelport>;

            static parcelset::locality here()
            {
                return parcelset::locality(
                    locality(util::gasnet_environment::enabled() ?
                            util::gasnet_environment::rank() :
                            -1));
            }

            static std::size_t max_connections(
                util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(ini,
                    "hpx.parcel.gasnet.max_connections",
                    HPX_PARCEL_MAX_CONNECTIONS);
            }

            static std::size_t background_threads(
                [[maybe_unused]] util::runtime_configuration const& ini)
            {
                /*
                return hpx::util::get_entry_as<std::size_t>(ini,
                    "hpx.parcel.gasnet.background_threads",
                    HPX_HAVE_PARCELPORT_GASNET_BACKGROUND_THREADS);
                */
                return 1UL;
            }

        public:
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(), notifier)
              , stopped_(false)
              , receiver_(*this)
              , background_threads_(background_threads(ini))
            {
            }

            parcelport(parcelport const&) = delete;
            parcelport(parcelport&&) = delete;
            parcelport& operator=(parcelport const&) = delete;
            parcelport& operator=(parcelport&&) = delete;

            ~parcelport()
            {
                util::gasnet_environment::finalize();
            }

            // Start the handling of connections.
            bool do_run()
            {
                receiver_.run();
                sender_.run();

                for (std::size_t i = 0; i != io_service_pool_.size(); ++i)
                {
#if ASIO_VERSION >= 103400
                    ::asio::post(
                        io_service_pool_.get_io_service(static_cast<int>(i)),
                        hpx::bind(&parcelport::io_service_work, this));
#else
                    io_service_pool_.get_io_service(static_cast<int>(i))
                        .post(hpx::bind(&parcelport::io_service_work, this));
#endif
                }
                return true;
            }

            // Stop the handling of connections.
            void do_stop()
            {
                while (do_background_work(0, parcelport_background_mode::all))
                {
                    if (threads::get_self_ptr())
                        hpx::this_thread::suspend(
                            hpx::threads::thread_schedule_state::pending,
                            "gasnet::parcelport::do_stop");
                }

                bool expected = false;
                if (stopped_.compare_exchange_strong(expected, true))
                {
                    stopped_ = true;

                    int retval;
                    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
                    if ((retval = gasnet_barrier_wait(
                             0, GASNET_BARRIERFLAG_ANONYMOUS)) != GASNET_OK)
                    {
                        // throw exception
                        HPX_THROW_EXCEPTION(error::invalid_status,
                            "hpx::util::gasnet_environment::init",
                            "GASNET failed ",
                            std::string{gasnet_ErrorName(retval)}, " ",
                            std::string{gasnet_ErrorDesc(retval)});
                    }
                }
            }

            /// Return the name of this locality
            std::string get_locality_name() const override
            {
                return util::gasnet_environment::get_processor_name();
            }

            std::shared_ptr<sender_connection> create_connection(
                parcelset::locality const& l, error_code&)
            {
                int dest_rank = l.get<locality>().rank();
                return sender_.create_connection(dest_rank, this);
            }

            parcelset::locality agas_locality(
                util::runtime_configuration const&) const override
            {
                return parcelset::locality(
                    locality(util::gasnet_environment::enabled() ? 0 : -1));
            }

            parcelset::locality create_locality() const override
            {
                return parcelset::locality(locality());
            }

            bool background_work(
                std::size_t num_thread, parcelport_background_mode mode)
            {
                if (stopped_.load(std::memory_order_acquire) ||
                    num_thread >= background_threads_)
                {
                    return false;
                }

                bool has_work = false;
                if (mode & parcelport_background_mode::send)
                {
                    has_work = sender_.background_work();
                }
                if (mode & parcelport_background_mode::receive)
                {
                    has_work = receiver_.background_work() || has_work;
                }
                return has_work;
            }

        private:
            std::atomic<bool> stopped_;

            sender sender_;
            receiver<parcelport> receiver_;

            void io_service_work()
            {
                std::size_t k = 0;
                // We only execute work on the IO service while HPX is starting
                while (hpx::is_starting())
                {
                    bool has_work = sender_.background_work();
                    has_work = receiver_.background_work() || has_work;
                    if (has_work)
                    {
                        k = 0;
                    }
                    else
                    {
                        ++k;
                        util::detail::yield_k(k,
                            "hpx::parcelset::policies::gasnet::parcelport::"
                            "io_service_work");
                    }
                }
            }

            std::size_t background_threads_;

            void early_write_handler(std::error_code const& ec, parcel const& p)
            {
                if (ec)
                {
                    // all errors during early parcel handling are fatal
                    std::exception_ptr exception = hpx::detail::get_exception(
                        hpx::exception(ec), "gasnet::early_write_handler",
                        __FILE__, __LINE__,
                        "error while handling early parcel: " + ec.message() +
                            "(" + std::to_string(ec.value()) + ")" +
                            parcelset::dump_parcel(p));

                    hpx::report_error(exception);
                }
            }
        };
    }    // namespace policies::gasnet
}    // namespace hpx::parcelset

namespace hpx::traits {

    // Inject additional configuration data into the factory registry for this
    // type. This information ends up in the system wide configuration database
    // under the plugin specific section:
    //
    //      [hpx.parcel.gasnet]
    //      ...
    //      priority = 1
    //
    template <>
    struct plugin_config_data<hpx::parcelset::policies::gasnet::parcelport>
    {
        static constexpr char const* priority() noexcept
        {
            return "1";
        }

        static void init(
            int* argc, char*** argv, util::command_line_handling& cfg)
        {
            util::gasnet_environment::init(argc, argv, cfg.rtcfg_);
            cfg.num_localities_ =
                static_cast<std::size_t>(util::gasnet_environment::size());
            cfg.node_ =
                static_cast<std::size_t>(util::gasnet_environment::rank());
        }

        // by default no additional initialization using the resource
        // partitioner is required
        static constexpr void init(hpx::resource::partitioner&) noexcept {}

        static void destroy() noexcept
        {
            util::gasnet_environment::finalize();
        }

        static constexpr char const* call() noexcept
        {
            return "";
        }
    };
}    // namespace hpx::traits

HPX_REGISTER_PARCELPORT(hpx::parcelset::policies::gasnet::parcelport, gasnet)

#endif
