//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/parcelport_mpi/locality.hpp>
#include <hpx/parcelport_mpi/receiver.hpp>
#include <hpx/parcelport_mpi/sender.hpp>

#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace policies::mpi {
        class HPX_EXPORT parcelport;
    }    // namespace policies::mpi

    template <>
    struct connection_handler_traits<policies::mpi::parcelport>
    {
        using connection_type = policies::mpi::sender_connection;
        using send_early_parcel = std::true_type;
        using do_background_work = std::true_type;
        using send_immediate_parcels = std::true_type;
        using is_connectionless = std::true_type;

        static constexpr char const* type() noexcept
        {
            return "mpi";
        }

        static constexpr char const* pool_name() noexcept
        {
            return "parcel-pool-mpi";
        }

        static constexpr char const* pool_name_postfix() noexcept
        {
            return "-mpi";
        }
    };

    namespace policies::mpi {

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
                    locality(util::mpi_environment::enabled() ?
                            util::mpi_environment::rank() :
                            -1));
            }

            static std::size_t max_connections(
                util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(ini,
                    "hpx.parcel.mpi.max_connections",
                    HPX_PARCEL_MAX_CONNECTIONS);
            }

            static std::size_t background_threads(
                util::runtime_configuration const& ini)
            {
                // limit the number of cores accessing MPI to one if
                // multi-threading in MPI is disabled
                if (!multi_threaded_mpi(ini))
                {
                    return 1;
                }

                return hpx::util::get_entry_as<std::size_t>(ini,
                    "hpx.parcel.mpi.background_threads",
                    HPX_HAVE_PARCELPORT_MPI_BACKGROUND_THREADS);
            }

            static bool multi_threaded_mpi(
                util::runtime_configuration const& ini)
            {
                if (hpx::util::get_entry_as<std::size_t>(
                        ini, "hpx.parcel.mpi.multithreaded", 1) != 0)
                {
                    return true;
                }
                return false;
            }

            static bool enable_send_immediate(
                util::runtime_configuration const& ini)
            {
                if (hpx::util::get_entry_as<std::size_t>(
                        ini, "hpx.parcel.mpi.sendimm", 0) != 0)
                {
                    return true;
                }
                return false;
            }

            static bool enable_ack_handshakes(
                util::runtime_configuration const& ini)
            {
                if (hpx::util::get_entry_as<std::size_t>(
                        ini, "hpx.parcel.mpi.ack_handshake", 0) != 0)
                {
                    return true;
                }
                return false;
            }

        public:
            using sender_type = sender;
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(), notifier)
              , stopped_(false)
              , receiver_(*this)
              , background_threads_(background_threads(ini))
              , multi_threaded_mpi_(multi_threaded_mpi(ini))
              , enable_send_immediate_(enable_send_immediate(ini))
              , enable_ack_handshakes_(enable_ack_handshakes(ini))
            {
            }

            parcelport(parcelport const&) = delete;
            parcelport(parcelport&&) = delete;
            parcelport& operator=(parcelport const&) = delete;
            parcelport& operator=(parcelport&&) = delete;

            ~parcelport() override
            {
                util::mpi_environment::finalize();
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
                    {
                        hpx::this_thread::suspend(
                            hpx::threads::thread_schedule_state::pending,
                            "mpi::parcelport::do_stop");
                    }
                }

                bool expected = false;
                if (stopped_.compare_exchange_strong(expected, true))
                {
                    util::mpi_environment::scoped_lock l;

                    int const ret =
                        MPI_Barrier(util::mpi_environment::communicator());
                    util::mpi_environment::check_mpi_error(
                        l, HPX_CURRENT_SOURCE_LOCATION(), ret);
                }
            }

            /// Return the name of this locality
            std::string get_locality_name() const override
            {
                return util::mpi_environment::get_processor_name();
            }

            std::shared_ptr<sender_connection> create_connection(
                parcelset::locality const& l, error_code&)
            {
                int const dest_rank = l.get<locality>().rank();
                return sender_.create_connection(
                    dest_rank, this, enable_ack_handshakes_);
            }

            parcelset::locality agas_locality(
                util::runtime_configuration const&) const override
            {
                return parcelset::locality(
                    locality(util::mpi_environment::enabled() ? 0 : -1));
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

            constexpr bool can_send_immediate() const noexcept
            {
                return enable_send_immediate_;
            }

            bool send_immediate(parcelset::parcelport* pp,
                parcelset::locality const& dest,
                sender::parcel_buffer_type buffer,
                sender::callback_fn_type&& callbackFn)
            {
                return sender_.send_immediate(pp, dest, HPX_MOVE(buffer),
                    HPX_MOVE(callbackFn), enable_ack_handshakes_);
            }

            template <typename F>
            bool reschedule_on_thread(F&& f,
                threads::thread_schedule_state state, char const* funcname)
            {
                // if MPI was initialized in serialized mode the new thread
                // needs to be pinned to thread 0
                if (multi_threaded_mpi_)
                {
                    return this->base_type::reschedule_on_thread(
                        HPX_FORWARD(F, f), state, funcname);
                }

                error_code ec(throwmode::lightweight);
                hpx::threads::thread_init_data data(
                    hpx::threads::make_thread_function_nullary(
                        HPX_FORWARD(F, f)),
                    funcname, threads::thread_priority::bound,
                    threads::thread_schedule_hint(static_cast<std::int16_t>(0)),
                    threads::thread_stacksize::default_, state, true);

                auto const id = hpx::threads::register_thread(data, ec);
                if (!ec)
                    return false;

                if (state == threads::thread_schedule_state::suspended)
                {
                    threads::set_thread_state(id.noref(),
                        std::chrono::milliseconds(100),
                        threads::thread_schedule_state::pending,
                        threads::thread_restart_state::signaled,
                        threads::thread_priority::bound, true, ec);
                }
                return true;
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
                            "hpx::parcelset::policies::mpi::parcelport::"
                            "io_service_work");
                    }
                }
            }

            std::size_t background_threads_;
            bool multi_threaded_mpi_;
            bool enable_send_immediate_;
            bool enable_ack_handshakes_;
        };
    }    // namespace policies::mpi
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

// Inject additional configuration data into the factory registry for this
// type. This information ends up in the system-wide configuration database
// under the plugin specific section:
//
//      [hpx.parcel.mpi]
//      ...
//      priority = 100
//
template <>
struct hpx::traits::plugin_config_data<
    hpx::parcelset::policies::mpi::parcelport>
{
    static constexpr char const* priority() noexcept
    {
        return "100";
    }

    static void init(int* argc, char*** argv, util::command_line_handling& cfg)
    {
        util::mpi_environment::init(argc, argv, cfg.rtcfg_);
        cfg.num_localities_ =
            static_cast<std::size_t>(util::mpi_environment::size());
        cfg.node_ = static_cast<std::size_t>(util::mpi_environment::rank());
    }

    // by default no additional initialization using the resource
    // partitioner is required
    static constexpr void init(hpx::resource::partitioner&) noexcept {}

    static void destroy() noexcept
    {
        util::mpi_environment::finalize();
    }

    static constexpr char const* call() noexcept
    {
        return
#if defined(HPX_HAVE_PARCELPORT_MPI_ENV)
            "env = ${HPX_HAVE_PARCELPORT_MPI_ENV:" HPX_HAVE_PARCELPORT_MPI_ENV
            "}\n"
#else
            "env = ${HPX_HAVE_PARCELPORT_MPI_ENV:"
            "MV2_COMM_WORLD_RANK,PMIX_RANK,PMI_RANK,OMPI_COMM_WORLD_SIZE,"
            "ALPS_APP_PE,PALS_NODEID}\n"
#endif
#if defined(HPX_HAVE_PARCELPORT_MPI_MULTITHREADED)
            "multithreaded = ${HPX_HAVE_PARCELPORT_MPI_MULTITHREADED:1}\n"
#else
            "multithreaded = ${HPX_HAVE_PARCELPORT_MPI_MULTITHREADED:0}\n"
#endif
            "max_connections = "
            "${HPX_HAVE_PARCELPORT_MPI_MAX_CONNECTIONS:8192}\n"

            // number of cores that do background work, default: all
            "background_threads = "
            "${HPX_HAVE_PARCELPORT_MPI_BACKGROUND_THREADS:-1}\n"
            "sendimm = 0\n";
    }
};    // namespace hpx::traits

HPX_REGISTER_PARCELPORT(hpx::parcelset::policies::mpi::parcelport, mpi)

#endif
