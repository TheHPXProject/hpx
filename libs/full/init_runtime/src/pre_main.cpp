//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)

#include <hpx/modules/agas.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/init_runtime/pre_main.hpp>

#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::detail {

    static void garbage_collect_non_blocking()
    {
        return ::hpx::agas::garbage_collect_non_blocking();
    }

    static void garbage_collect()
    {
        return ::hpx::agas::garbage_collect();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Install performance counter startup functions for core subsystems.
    static void register_counter_types()
    {
        auto& agas_client = naming::get_agas_client();
        performance_counters::register_agas_counter_types(agas_client);
        agas_client.register_server_instances();
        lbt_ << "(2nd stage) pre_main: registered AGAS client-side "
                "performance counter types";

        get_runtime_distributed().register_counter_types();
        lbt_ << "(2nd stage) pre_main: registered runtime performance "
                "counter types";

        performance_counters::register_threadmanager_counter_types(
            threads::get_thread_manager());
        lbt_ << "(2nd stage) pre_main: registered thread-manager performance "
                "counter types";

#if defined(HPX_HAVE_NETWORKING)
        performance_counters::register_parcelhandler_counter_types(
            applier::get_applier().get_parcel_handler());
        lbt_ << "(2nd stage) pre_main: registered parcelset performance "
                "counter types";
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    static void register_message_handlers()
    {
        runtime_distributed& rtd = get_runtime_distributed();
        for (auto const& t :
            parcelset::detail::get_message_handler_registrations())
        {
            error_code ec(throwmode::lightweight);
            rtd.register_message_handler(hpx::get<0>(t), hpx::get<1>(t), ec);
        }
        lbt_ << "(3rd stage) pre_main: registered message handlers";
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Implements second and third stage bootstrapping.
    int pre_main(runtime_mode mode)
    {
        // Register pre-shutdown and shutdown functions to flush pending
        // reference counting operations.
        register_pre_shutdown_function(&garbage_collect_non_blocking);
        register_shutdown_function(&garbage_collect);

        using components::stubs::runtime_support;

        agas::addressing_service& agas_client = naming::get_agas_client();
        runtime& rt = get_runtime();

        int exit_code = 0;
        if (runtime_mode::connect == mode)
        {
            lbt_ << "(2nd stage) pre_main: locality is in connect mode, "
                    "skipping 2nd and 3rd stage startup synchronization";
            lbt_ << "(2nd stage) pre_main: addressing services enabled";

            // Load components, so that we can use the barrier LCO.
            exit_code = runtime_support::load_components(find_here());
            lbt_ << "(2nd stage) pre_main: loaded components"
                 << (exit_code ? ", application exit has been requested" : "");

            // Work on registration requests for message handler plugins
#if defined(HPX_HAVE_NETWORKING)
            register_message_handlers();
#endif
            // Register all counter types before the startup functions are being
            // executed.
            register_counter_types();

            rt.set_state(hpx::state::pre_startup);
            runtime_support::call_startup_functions(find_here(), true);
            lbt_ << "(3rd stage) pre_main: ran pre-startup functions";

            rt.set_state(hpx::state::startup);
            runtime_support::call_startup_functions(find_here(), false);
            lbt_ << "(4th stage) pre_main: ran startup functions";
        }
        else
        {
            lbt_ << "(2nd stage) pre_main: addressing services enabled";

            // Load components, so that we can use the barrier LCO.
            exit_code = runtime_support::load_components(find_here());
            lbt_ << "(2nd stage) pre_main: loaded components"
                 << (exit_code ? ", application exit has been requested" : "");

            // Second and third stage barrier creation.
            if (agas_client.is_bootstrap())
            {
                naming::gid_type console_;
                if (HPX_UNLIKELY(!agas_client.get_console_locality(console_)))
                {
                    HPX_THROW_EXCEPTION(hpx::error::network_error, "pre_main",
                        "no console locality registered");
                }

                lbt_ << "(2nd stage) pre_main: creating 2nd and 3rd stage boot "
                        "barriers";
            }
            else    // Hosted.
            {
                lbt_ << "(2nd stage) pre_main: finding 2nd and 3rd stage boot "
                        "barriers";
            }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
            // create predefined communicator, but only if locality is not
            // connecting late
            if (hpx::get_config_entry("hpx.runtime_mode",
                    get_runtime_mode_name(runtime_mode::console)) !=
                get_runtime_mode_name(runtime_mode::connect))
            {
                hpx::collectives::detail::create_global_communicator();
            }

            // create our global barrier...
            hpx::distributed::barrier::get_global_barrier() =
                hpx::distributed::barrier::create_global_barrier();

            // Second stage bootstrap synchronizes component loading across all
            // localities, ensuring that the component namespace tables are fully
            // populated before user code is executed.
            distributed::barrier::synchronize();
            lbt_ << "(2nd stage) pre_main: passed 2nd stage boot barrier";

            // Work on registration requests for message handler plugins
#if defined(HPX_HAVE_NETWORKING)
            register_message_handlers();
#endif
            // Register all counter types before the startup functions are being
            // executed.
            register_counter_types();

            // Second stage bootstrap synchronizes performance counter loading
            // across all localities.
            distributed::barrier::synchronize();
            lbt_ << "(3rd stage) pre_main: passed 3rd stage boot barrier";

            runtime_support::call_startup_functions(find_here(), true);
            lbt_ << "(3rd stage) pre_main: ran pre-startup functions";

            // Third stage separates pre-startup and startup function phase.
            distributed::barrier::synchronize();
            lbt_ << "(4th stage) pre_main: passed 4th stage boot barrier";

            runtime_support::call_startup_functions(find_here(), false);
            lbt_ << "(4th stage) pre_main: ran startup functions";

            // Forth stage bootstrap synchronizes startup functions across all
            // localities. This is done after component loading to guarantee that
            // all user code, including startup functions, are only run after the
            // component tables are populated.
            distributed::barrier::synchronize();
            lbt_ << "(5th stage) pre_main: passed 5th stage boot barrier";
#endif
        }

        // Enable logging. Even if we terminate at this point we will see all
        // pending log messages so far.
        components::activate_logging();
        lbt_ << "(last stage) pre_main: activated logging";

        // Any error in post-command line handling or any explicit --exit command
        // line option will cause the application to terminate at this point.
        if (exit_code)
        {
            // If load_components returns false, shutdown the system. This
            // essentially only happens if the command line contained --exit.
            runtime_support::shutdown_all(
                naming::get_id_from_locality_id(agas::booststrap_prefix), -1.0);
            return exit_code;
        }

        // Connect back to given latch if specified
        std::string connect_back_to(
            get_config_entry("hpx.on_startup.wait_on_latch", ""));
        if (!connect_back_to.empty())
        {
            lbt_ << "(6th stage) runtime::run_helper: about to "
                    "synchronize with latch: "
                 << connect_back_to;

            // inform launching process that this locality is up and running
            hpx::distributed::latch l;
            l.connect_to(connect_back_to);
            l.arrive_and_wait();

            lbt_ << "(6th stage) runtime::run_helper: "
                    "synchronized with latch: "
                 << connect_back_to;
        }

        return 0;
    }

    void post_main()
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        // destroy predefined communicators
        hpx::collectives::detail::reset_global_communicator();
        hpx::collectives::detail::reset_local_communicator();
        hpx::collectives::detail::reset_world_channel_communicator();

        // simply destroy global barrier
        hpx::distributed::barrier::get_global_barrier().detach();
#endif
    }
}    // namespace hpx::detail

#endif
