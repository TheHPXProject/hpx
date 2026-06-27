//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2017 Shoshana Jakobovits
//  Copyright (c) 2010-2011 Phillip LeBlanc, Dylan Stark
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/init_runtime_local.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/hpx_user_main_config.hpp>
#include <hpx/init.hpp>
#include <hpx/init_runtime/detail/main_winsocket.hpp>
#include <hpx/init_runtime/finalize.hpp>
#include <hpx/init_runtime/suspend.hpp>

#ifdef HPX_HAVE_MODULE_MPI_BASE
#include <hpx/modules/mpi_base.hpp>
#endif
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/init_runtime/pre_main.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming.hpp>
#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelports/init_all_parcelports.hpp>
#endif
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#endif
#if defined(HPX_HAVE_LOGGING)
#include <hpx/init_runtime/detail/init_logging.hpp>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if !defined(HPX_WINDOWS)
#include <signal.h>
#endif

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx_startup {

    std::vector<std::string> (*user_main_config_function)(
        std::vector<std::string> const&) = nullptr;
}    // namespace hpx_startup

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    // forward declarations only
    void console_print(std::string const&);

    int init_impl(
        hpx::function<int(hpx::program_options::variables_map&)> const& f,
        int argc, char** argv, init_params const& params,
        char const* hpx_prefix, [[maybe_unused]] char** env)
    {
        if (argc == 0 || argv == nullptr)
        {
            argc = hpx::local::detail::dummy_argc;
            argv = hpx::local::detail::dummy_argv;
        }

#if defined(HPX_WINDOWS)
        detail::init_winsocket();
#if defined(HPX_HAVE_APEX)
        // artificially force the apex shared library to be loaded by the
        // application
        apex::version();
#endif
#endif
#if defined(HPX_HAVE_NETWORKING)
        // force linking parcelports
        hpx::parcelset::init_all_parcelports();
#endif
        util::set_hpx_prefix(hpx_prefix);
#if defined(__FreeBSD__)
        freebsd_environ = env;
#endif
        // set a handler for std::at_quick_exit, and std::atexit
        [[maybe_unused]] auto const ret_at_exit = std::atexit(detail::on_exit);
        HPX_ASSERT(ret_at_exit == 0);

#if defined(HPX_HAVE_CXX11_STD_QUICK_EXIT)
        [[maybe_unused]] auto const ret_at_quick_exit =
            std::at_quick_exit(detail::on_exit);
        HPX_ASSERT(ret_at_quick_exit == 0);
#endif
        return detail::run_or_start(f, argc, argv, params, true);
    }

    bool start_impl(
        hpx::function<int(hpx::program_options::variables_map&)> const& f,
        int argc, char** argv, init_params const& params,
        char const* hpx_prefix, [[maybe_unused]] char** env)
    {
        if (argc == 0 || argv == nullptr)
        {
            argc = local::detail::dummy_argc;
            argv = local::detail::dummy_argv;
        }

#if defined(HPX_WINDOWS)
        detail::init_winsocket();
#if defined(HPX_HAVE_APEX)
        // artificially force the apex shared library to be loaded by the
        // application
        apex::version();
#endif
#endif
#if defined(HPX_HAVE_NETWORKING)
        // force linking parcelports
        hpx::parcelset::init_all_parcelports();
#endif
        util::set_hpx_prefix(hpx_prefix);
#if defined(__FreeBSD__)
        freebsd_environ = env;
#endif
        // set a handler for std::abort, std::at_quick_exit, and std::atexit
        [[maybe_unused]] auto const prev_signal =
            std::signal(SIGABRT, detail::on_abort);
        HPX_ASSERT(prev_signal != SIG_ERR);

        [[maybe_unused]] auto const ret_atexit = std::atexit(detail::on_exit);
        HPX_ASSERT(ret_atexit == 0);

#if defined(HPX_HAVE_CXX11_STD_QUICK_EXIT)
        [[maybe_unused]] auto const ret_at_quick_exit =
            std::at_quick_exit(detail::on_exit);
        HPX_ASSERT(ret_at_quick_exit == 0);
#endif
        return 0 == detail::run_or_start(f, argc, argv, params, false);
    }
}    // namespace hpx::detail

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
namespace hpx::detail {

    // forward declarations only
    void list_symbolic_name(std::string const&, hpx::id_type const&);
    void list_component_type(std::string const&, components::component_type);
}    // namespace hpx::detail

HPX_PLAIN_ACTION_ID(hpx::detail::console_print, console_print_action,
    hpx::actions::console_print_action_id)
HPX_PLAIN_ACTION_ID(hpx::detail::list_component_type,
    list_component_type_action, hpx::actions::list_component_type_action_id)

using bound_list_component_type_action =
    hpx::detail::bound_action<list_component_type_action,
        hpx::util::index_pack<0, 1, 2>, hpx::id_type,
        hpx::detail::placeholder<1>, hpx::detail::placeholder<2>>;

HPX_UTIL_REGISTER_FUNCTION_DECLARATION(
    void(std::string const&, hpx::components::component_type),
    bound_list_component_type_action, list_component_type_function)

HPX_UTIL_REGISTER_FUNCTION(
    void(std::string const&, hpx::components::component_type),
    bound_list_component_type_action, list_component_type_function)
#endif

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    // print string on the console
    void console_print(std::string const& name)
    {
        std::cout << name << std::endl;
    }

    inline void print(std::string const& name, error_code& ec = throws)
    {
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        hpx::id_type console(agas::get_console_locality(ec));
        if (ec)
            return;

        hpx::async<console_print_action>(console, name).get(ec);
        if (ec)
            return;
#else
        console_print(name);
#endif
        if (&ec != &throws)
            ec = make_success_code();
    }

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
    ///////////////////////////////////////////////////////////////////////////
    // redirect the printing of the given counter name to the console
    bool list_counter(
        performance_counters::counter_info const& info, error_code& ec)
    {
        print(info.fullname_, ec);
        return true;
    }

    // List the names of all registered performance counters.
    void list_counter_names_header(bool skeleton)
    {
        // print header
        print("List of available counter instances");
        if (skeleton)
            print("(replace '*' below with the appropriate sequence number)");
        print(std::string(78, '-'));
    }

    void list_counter_names_minimal()
    {
        // list all counter names
        list_counter_names_header(true);
        performance_counters::discover_counter_types(&list_counter,
            performance_counters::discover_counters_mode::minimal);
    }

    void list_counter_names_full()
    {
        // list all counter names
        list_counter_names_header(false);
        performance_counters::discover_counter_types(
            &list_counter, performance_counters::discover_counters_mode::full);
    }

    ///////////////////////////////////////////////////////////////////////////
    // redirect the printing of the full counter info to the console
    bool list_counter_info(
        performance_counters::counter_info const& info, error_code& ec)
    {
        // compose the information to be printed for each of the counters
        std::ostringstream strm;

        strm << std::string(78, '-') << '\n';
        strm << "fullname: " << info.fullname_ << '\n';
        strm << "helptext: " << info.helptext_ << '\n';
        strm << "type:     "
             << performance_counters::get_counter_type_name(info.type_) << '\n';

        strm << "version:  ";    // 0xMMmmrrrr
        hpx::util::format_to(strm, "{}.{}.{}\n", info.version_ / 0x1000000,
            info.version_ / 0x10000 % 0x100, info.version_ % 0x10000);
        strm << std::string(78, '-') << '\n';

        print(strm.str(), ec);

        if (&ec != &throws)
            ec = make_success_code();
        return true;
    }

    // List the names of all registered performance counters.
    void list_counter_infos_header(bool skeleton)
    {
        // print header
        print("Information about available counter instances");
        if (skeleton)
            print("(replace '*' below with the appropriate sequence number)");
        print("--------------------------------------------------------");
    }

    void list_counter_infos_minimal()
    {
        // list all counter information
        list_counter_infos_header(true);
        performance_counters::discover_counter_types(&list_counter_info,
            performance_counters::discover_counters_mode::minimal);
    }

    void list_counter_infos_full()
    {
        // list all counter information
        list_counter_infos_header(false);
        performance_counters::discover_counter_types(&list_counter_info,
            performance_counters::discover_counters_mode::full);
    }

    ///////////////////////////////////////////////////////////////////////////
    void list_symbolic_name(std::string const& name, hpx::id_type const& id)
    {
        std::string const str = hpx::util::format("{}, {}, {}", name, id,
            (id.get_management_type() == id_type::management_type::managed ?
                    "management_type::managed" :
                    "management_type::unmanaged"));
        print(str);
    }

    void list_symbolic_names()
    {
        print(std::string("List of all registered symbolic names:"));
        print(std::string("--------------------------------------"));

        std::map<std::string, hpx::id_type> const entries =
            agas::find_symbols(hpx::launch::sync);

        for (auto const& e : entries)
        {
            list_symbolic_name(e.first, e.second);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void list_component_type(
        std::string const& name, components::component_type ctype)
    {
        print(hpx::util::format(
            "{1:-40}, {2}", name, components::get_component_type_name(ctype)));
    }

    void list_component_types()
    {
        print(std::string("List of all registered component types:"));
        print(std::string("---------------------------------------"));

        using hpx::placeholders::_1;
        using hpx::placeholders::_2;

        hpx::id_type console(agas::get_console_locality());
        naming::get_agas_client().iterate_types(
            hpx::bind<list_component_type_action>(console, _1, _2));
    }

    ///////////////////////////////////////////////////////////////////////////
    void start_counters(std::shared_ptr<util::query_counters> const& qc)
    {
        try
        {
            HPX_ASSERT(qc);
            qc->start();
        }
        catch (...)
        {
            std::cerr << hpx::diagnostic_information(std::current_exception())
                      << std::flush;
            hpx::terminate();
        }
    }
#endif
}    // namespace hpx::detail

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux) ||               \
        defined(__APPLE__))
namespace hpx_start {
    // Importing weak symbol from libhpx_wrap.a which may be shadowed by one present in
    // hpx_main.hpp.
    HPX_SYMBOL_EXPORT __attribute__((weak)) bool include_libhpx_wrap = false;
}    // namespace hpx_start

#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    // Print stack trace and exit.
#if defined(HPX_WINDOWS)
    extern BOOL WINAPI termination_handler(DWORD ctrl_type);
#else
    extern void termination_handler(int signum);
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        void activate_global_options(
            util::command_line_handling& cmdline, int argc, char** argv)
        {
#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__)
            threads::coroutines::detail::posix::use_guard_pages =
                cmdline.rtcfg_.use_stack_guard_pages();
#endif
#ifdef HPX_HAVE_VERIFY_LOCKS
            if (cmdline.rtcfg_.enable_lock_detection())
            {
                util::enable_lock_detection();
                util::trace_depth_lock_detection(cmdline.rtcfg_.trace_depth());
            }
            else
            {
                util::disable_lock_detection();
            }
#endif
#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
            threads::policies::set_minimal_deadlock_detection_enabled(
                cmdline.rtcfg_.enable_minimal_deadlock_detection());
#endif
#ifdef HPX_HAVE_SPINLOCK_DEADLOCK_DETECTION
            util::detail::set_spinlock_break_on_deadlock_enabled(
                cmdline.rtcfg_.enable_spinlock_deadlock_detection());
            util::detail::set_spinlock_deadlock_detection_limit(
                cmdline.rtcfg_.get_spinlock_deadlock_detection_limit());
#endif

#if defined(HPX_HAVE_LOGGING)
            util::detail::init_logging_full(cmdline.rtcfg_);
#else
            util::detail::warn_if_logging_requested(cmdline.rtcfg_);
#endif

#if defined(HPX_HAVE_NETWORKING)
            if (cmdline.num_localities_ != 1 || cmdline.node_ != 0 ||
                cmdline.rtcfg_.enable_networking())
            {
                parcelset::parcelhandler::init(&argc, &argv, cmdline);
            }
#endif
            HPX_UNUSED(argc);
            HPX_UNUSED(argv);
        }

        ///////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        void handle_list_and_print_options(hpx::runtime& rt,
            hpx::program_options::variables_map& vm,
            bool print_counters_locally)
        {
            auto* rtd = dynamic_cast<hpx::runtime_distributed*>(&rt);
            if (rtd == nullptr)
            {
                throw detail::command_line_error(
                    "Unexpected: runtime system was not initialized.");
            }

            if (vm.count("hpx:list-counters"))
            {
                // Print the names of all registered performance counters.
                std::string option(vm["hpx:list-counters"].as<std::string>());
                if (0 == std::string("minimal").find(option))
                    rt.add_startup_function(&list_counter_names_minimal);
                else if (0 == std::string("full").find(option))
                    rt.add_startup_function(&list_counter_names_full);
                else
                {
                    std::string msg("Invalid command line option value"
                                    "for --hpx:list-counters: ");
                    msg += option;
                    msg += ", allowed values are 'minimal' and 'full'";
                    throw detail::command_line_error(msg.c_str());
                }
            }
            if (vm.count("hpx:list-counter-infos"))
            {
                // Print info about all registered performance counters.
                std::string option(
                    vm["hpx:list-counter-infos"].as<std::string>());
                if (0 == std::string("minimal").find(option))
                    rt.add_startup_function(&list_counter_infos_minimal);
                else if (0 == std::string("full").find(option))
                    rt.add_startup_function(&list_counter_infos_full);
                else
                {
                    std::string msg("Invalid command line option value"
                                    "for --hpx:list-counter-infos: ");
                    msg += option;
                    msg += ", allowed values are 'minimal' and 'full'";
                    throw detail::command_line_error(msg.c_str());
                }
            }
            if (vm.count("hpx:list-symbolic-names"))
            {
                // Print all registered symbolic names.
                rt.add_startup_function(&list_symbolic_names);
            }
            if (vm.count("hpx:list-component-types"))
            {
                // Print all registered component types.
                rt.add_startup_function(&list_component_types);
            }

            if (vm.count("hpx:print-counter") ||
                vm.count("hpx:print-counter-reset"))
            {
                std::size_t interval = 0;
                if (vm.count("hpx:print-counter-interval"))
                {
                    interval =
                        vm["hpx:print-counter-interval"].as<std::size_t>();
                }

                std::vector<std::string> counters;
                if (vm.count("hpx:print-counter"))
                {
                    counters =
                        vm["hpx:print-counter"].as<std::vector<std::string>>();
                }

                std::vector<std::string> reset_counters;
                if (vm.count("hpx:print-counter-reset"))
                {
                    reset_counters = vm["hpx:print-counter-reset"]
                                         .as<std::vector<std::string>>();
                }

                std::vector<std::string> counter_shortnames;
                std::string counter_format("normal");
                if (vm.count("hpx:print-counter-format"))
                {
                    counter_format =
                        vm["hpx:print-counter-format"].as<std::string>();
                    if (counter_format == "csv-short")
                    {
                        for (auto& counter : counters)
                        {
                            std::vector<std::string> entry;
                            hpx::string_util::split(entry, counter,
                                hpx::string_util::is_any_of(","),
                                hpx::string_util::token_compress_mode::on);

                            if (entry.size() != 2)
                            {
                                throw detail::command_line_error(
                                    "Invalid format for command line "
                                    "option "
                                    "--hpx:print-counter-format=csv-short");
                            }

                            counter_shortnames.push_back(entry[0]);
                            counter = entry[1];
                        }
                    }
                }

                bool csv_header = true;
                if (vm.count("hpx:no-csv-header"))
                    csv_header = false;

                std::string destination("cout");
                if (vm.count("hpx:print-counter-destination"))
                    destination =
                        vm["hpx:print-counter-destination"].as<std::string>();

                bool counter_types = false;
                if (vm.count("hpx:print-counter-types"))
                    counter_types = true;

                // schedule the query function at startup, which will schedule
                // itself to run after the given interval
                std::shared_ptr<util::query_counters> qc =
                    std::make_shared<util::query_counters>(std::ref(counters),
                        std::ref(reset_counters), interval, destination,
                        counter_format, counter_shortnames, csv_header,
                        print_counters_locally, counter_types);

                // schedule to print counters at shutdown, if requested
                if (get_config_entry("hpx.print_counter.shutdown", "0") == "1")
                {
                    // schedule to run at shutdown
                    rt.add_pre_shutdown_function(hpx::bind_front(
                        &util::query_counters::evaluate, qc, true));
                }

                // schedule to start all counters

                rt.add_startup_function(hpx::bind_front(&start_counters, qc));

                // register the query_counters object with the runtime system
                rtd->register_query_counters(qc);
            }
            else if (vm.count("hpx:print-counter-interval"))
            {
                throw detail::command_line_error(
                    "Invalid command line option "
                    "--hpx:print-counter-interval, valid in conjunction "
                    "with --hpx:print-counter only");
            }
            else if (vm.count("hpx:print-counter-destination"))
            {
                throw detail::command_line_error(
                    "Invalid command line option "
                    "--hpx:print-counter-destination, valid in conjunction "
                    "with --hpx:print-counter only");
            }
            else if (vm.count("hpx:print-counter-format"))
            {
                throw detail::command_line_error(
                    "Invalid command line option "
                    "--hpx:print-counter-format, valid in conjunction with "
                    "--hpx:print-counter only");
            }
            else if (vm.count("hpx:print-counter-at"))
            {
                throw detail::command_line_error(
                    "Invalid command line option "
                    "--hpx:print-counter-at, valid in conjunction with "
                    "--hpx:print-counter only");
            }
            else if (vm.count("hpx:reset-counters"))
            {
                throw detail::command_line_error(
                    "Invalid command line option "
                    "--hpx:reset-counters, valid in conjunction with "
                    "--hpx:print-counter only");
            }
        }
#endif

        void add_startup_functions(hpx::runtime& rt,
            hpx::program_options::variables_map& vm, runtime_mode mode,
            startup_function_type startup, shutdown_function_type shutdown)
        {
            if (vm.count("hpx:app-config"))
            {
                std::string const config(
                    vm["hpx:app-config"].as<std::string>());
                rt.get_config().load_application_configuration(config.c_str());
            }

            if (!!startup)
                rt.add_startup_function(HPX_MOVE(startup));

            if (!!shutdown)
                rt.add_shutdown_function(HPX_MOVE(shutdown));

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            // Add startup function related to listing counter names or counter
            // infos (on console only).
            bool const print_counters_locally =
                vm.count("hpx:print-counters-locally") != 0;
            if (mode == runtime_mode::console || print_counters_locally)
                handle_list_and_print_options(rt, vm, print_counters_locally);
#else
            HPX_UNUSED(mode);
#endif

            // Dump the configuration before all components have been loaded.
            if (vm.count("hpx:dump-config-initial"))
            {
                std::cout << "Configuration after runtime construction:\n";
                std::cout << "-----------------------------------------\n";
                rt.get_config().dump(0, std::cout);
                std::cout << "-----------------------------------------\n";
            }

            // Dump the configuration after all components have been loaded.
            if (vm.count("hpx:dump-config"))
                rt.add_startup_function(hpx::local::detail::dump_config(rt));
        }

        ///////////////////////////////////////////////////////////////////////
        int run(hpx::runtime& rt,
            hpx::function<int(hpx::program_options::variables_map& vm)> const&
                f,
            hpx::program_options::variables_map& vm, runtime_mode mode,
            startup_function_type startup, shutdown_function_type shutdown)
        {
            LPROGRESS_;

            add_startup_functions(
                rt, vm, mode, HPX_MOVE(startup), HPX_MOVE(shutdown));

            // Run this runtime instance using the given function f.
            if (!f.empty())
                return rt.run(hpx::bind_front(f, vm));

            // Run this runtime instance without a hpx_main
            return rt.run();
        }

        int start(hpx::runtime& rt,
            hpx::function<int(hpx::program_options::variables_map& vm)> const&
                f,
            hpx::program_options::variables_map& vm, runtime_mode mode,
            startup_function_type startup, shutdown_function_type shutdown)
        {
            LPROGRESS_;

            add_startup_functions(
                rt, vm, mode, HPX_MOVE(startup), HPX_MOVE(shutdown));

            if (!f.empty())
            {
                // Run this runtime instance using the given function f.
                return rt.start(hpx::bind_front(f, vm));
            }

            // Run this runtime instance without a hpx_main
            return rt.start();
        }

        int run_or_start(bool blocking, std::unique_ptr<hpx::runtime> rt,
            util::command_line_handling& cfg, startup_function_type startup,
            shutdown_function_type shutdown)
        {
            if (blocking)
            {
                return run(*rt, cfg.hpx_main_f_, cfg.vm_, cfg.rtcfg_.mode_,
                    HPX_MOVE(startup), HPX_MOVE(shutdown));
            }

            // non-blocking version
            start(*rt, cfg.hpx_main_f_, cfg.vm_, cfg.rtcfg_.mode_,
                HPX_MOVE(startup), HPX_MOVE(shutdown));

            // pointer to runtime is stored in TLS
            [[maybe_unused]] hpx::runtime const* p = rt.release();
            HPX_ASSERT(p != nullptr);

            return 0;
        }

        ////////////////////////////////////////////////////////////////////////
        void init_environment(
            [[maybe_unused]] hpx::util::runtime_configuration const& cfg)
        {
            HPX_UNUSED(hpx::filesystem::initial_path());

            hpx::assertion::set_assertion_handler(&detail::assertion_handler);
            hpx::util::set_test_failure_handler(&detail::test_failure_handler);
            hpx::tracing::set_enable_parent_task_handler(
                &detail::enable_parent_task_handler);
            hpx::set_custom_exception_info_handler(
                &detail::custom_exception_info);
            hpx::serialization::detail::set_save_custom_exception_handler(
                &runtime_local::detail::save_custom_exception);
            hpx::serialization::detail::set_load_custom_exception_handler(
                &runtime_local::detail::load_custom_exception);
            hpx::set_pre_exception_handler(&detail::pre_exception_handler);
            hpx::set_thread_termination_handler(
                [](std::exception_ptr const& e) { report_error(e); });
            hpx::lcos::detail::set_run_on_completed_error_handler(
                [](std::exception_ptr const& e) {
                    report_exception_and_terminate(e);
                });
#if defined(HPX_HAVE_VERIFY_LOCKS)
            hpx::util::set_registered_locks_error_handler(
                &detail::registered_locks_error_handler);
            hpx::util::set_register_locks_predicate(
                &detail::register_locks_predicate);
#endif
#if !defined(HPX_HAVE_DISABLED_SIGNAL_EXCEPTION_HANDLERS)
            set_error_handlers(cfg);
#endif
            hpx::threads::detail::set_get_default_pool(
                &detail::get_default_pool);
            hpx::threads::detail::set_get_default_timer_service(
                &hpx::detail::get_default_timer_service);
            hpx::threads::detail::set_get_locality_id(&get_locality_id);
            hpx::parallel::execution::detail::set_get_pu_mask(
                &hpx::detail::get_pu_mask);
            hpx::parallel::execution::detail::set_get_os_thread_count(
                []() { return hpx::get_os_thread_count(); });
            hpx::parallel::detail::set_exception_list_termination_handler(
                &hpx::terminate);
            hpx::parallel::util::detail::
                set_parallel_exception_termination_handler(&hpx::terminate);

            // instantiate the interface function initialization objects
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#if defined(HPX_HAVE_NETWORKING)
            parcelset::locality_init();
#endif
            agas::agas_init();
            agas::runtime_components_init();
            components::counter_init();
#endif

#if defined(HPX_NATIVE_MIC) || defined(__bgq__) || defined(__bgqion__)
            unsetenv("LANG");
            unsetenv("LC_CTYPE");
            unsetenv("LC_NUMERIC");
            unsetenv("LC_TIME");
            unsetenv("LC_COLLATE");
            unsetenv("LC_MONETARY");
            unsetenv("LC_MESSAGES");
            unsetenv("LC_PAPER");
            unsetenv("LC_NAME");
            unsetenv("LC_ADDRESS");
            unsetenv("LC_TELEPHONE");
            unsetenv("LC_MEASUREMENT");
            unsetenv("LC_IDENTIFICATION");
            unsetenv("LC_ALL");
#endif
        }

        // make sure the runtime system is not active yet
        int ensure_no_runtime_is_up()
        {
            // make sure the runtime system is not active yet
            if (get_runtime_ptr() != nullptr)
            {
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux) ||               \
        defined(__APPLE__))
                // make sure the runtime system is not initialized
                // after its activation from int main()
                if (hpx_start::include_libhpx_wrap)
                {
                    std::cerr
                        << "hpx is already initialized from main.\n"
                           "Note: Delete hpx_main.hpp to initialize hpx system "
                           "using hpx::init. Exiting...\n";
                    return -1;
                }
#endif
                std::cerr << "hpx::init: can't initialize runtime system "
                             "more than once! Exiting...\n";
                return -1;
            }
            return 0;
        }

        ///////////////////////////////////////////////////////////////////////
        int run_or_start(
            hpx::function<int(hpx::program_options::variables_map& vm)> const&
                f,
            int argc, char** argv, init_params const& params, bool blocking)
        {
            int result;
            try
            {
                // make sure the runtime system is not active yet
                result = ensure_no_runtime_is_up();
                if (result != 0)
                {
                    return result;
                }

#if defined(HPX_HAVE_NETWORKING)
                hpx::util::command_line_handling cmdline{
                    hpx::util::runtime_configuration(argv[0], params.mode,
                        hpx::parcelset::load_runtime_configuration()),
                    hpx_startup::user_main_config(params.cfg), f};
#else
                hpx::util::command_line_handling cmdline{
                    hpx::util::runtime_configuration(argv[0], params.mode, {}),
                    hpx_startup::user_main_config(params.cfg), f};
#endif

                std::vector<
                    std::shared_ptr<components::component_registry_base>>
                    component_registries;

                // scope exception handling to resource partitioner initialization
                // any exception thrown during run_or_start below are handled
                // separately
                try
                {
                    result = cmdline.call(
                        params.desc_cmdline, argc, argv, component_registries);

                    init_environment(cmdline.rtcfg_);

                    hpx::threads::policies::detail::affinity_data
                        affinity_data{};
                    affinity_data.init(hpx::util::get_entry_as<std::size_t>(
                                           cmdline.rtcfg_, "hpx.os_threads", 0),
                        hpx::util::get_entry_as<std::size_t>(
                            cmdline.rtcfg_, "hpx.cores", 0),
                        hpx::util::get_entry_as<std::size_t>(
                            cmdline.rtcfg_, "hpx.pu_offset", 0),
                        hpx::util::get_entry_as<std::size_t>(
                            cmdline.rtcfg_, "hpx.pu_step", 0),
                        static_cast<std::size_t>(
                            cmdline.rtcfg_.get_first_used_core()),
                        cmdline.rtcfg_.get_entry("hpx.affinity", ""),
                        cmdline.rtcfg_.get_entry("hpx.bind", ""),
                        hpx::util::get_entry_as<bool>(
                            cmdline.rtcfg_, "hpx.use_process_mask", false));

                    hpx::resource::partitioner rp =
                        hpx::resource::detail::make_partitioner(
                            params.rp_mode, cmdline.rtcfg_, affinity_data);

                    activate_global_options(cmdline, argc, argv);

                    // check whether HPX should be exited at this point
                    // (parse_result is returning a result > 0, if the program options
                    // contain --hpx:help or --hpx:version, on error result is < 0)
                    if (result != 0)
                    {
                        result = (std::min) (result, 0);
                        return result;
                    }

                    // If thread_pools initialization in user main
                    if (params.rp_callback)
                    {
                        params.rp_callback(rp, cmdline.vm_);
                    }

#if defined(HPX_HAVE_NETWORKING)
                    if (cmdline.num_localities_ != 1 || cmdline.node_ != 0 ||
                        cmdline.rtcfg_.enable_networking())
                    {
                        parcelset::parcelhandler::init(rp);
                    }
#endif
                    // Setup all internal parameters of the resource_partitioner
                    rp.configure_pools();
                }
                catch (hpx::exception const& e)
                {
                    std::cerr << "hpx::init: hpx::exception caught: "
                              << hpx::get_error_what(e) << "\n";
                    return -1;
                }

                // Initialize and start the HPX runtime.
                LPROGRESS_ << "run_local: create runtime";

                // Build and configure this runtime instance.
                std::unique_ptr<hpx::runtime> rt;

                // Command line handling should have updated this by now.
                HPX_ASSERT(cmdline.rtcfg_.mode_ != runtime_mode::default_);
                if (cmdline.rtcfg_.mode_ == runtime_mode::local)
                {
                    LPROGRESS_ << "creating local runtime";
                    rt.reset(new hpx::runtime(cmdline.rtcfg_, true));
                }
                else
                {
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
                    for (auto const& registry : component_registries)
                    {
                        hpx::register_startup_function([registry]() {
                            registry->register_component_type();
                        });
                    }

                    LPROGRESS_ << "creating distributed runtime";
                    rt.reset(new hpx::runtime_distributed(cmdline.rtcfg_,
                        &hpx::detail::pre_main, &hpx::detail::post_main));
#else
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "run_or_start",
                        "Attempted to start the runtime in the mode \"{1}\", "
                        "but HPX was compiled with "
                        "HPX_WITH_DISTRIBUTED_RUNTIME=OFF, and \"{1}\" "
                        "requires HPX_WITH_DISTRIBUTED_RUNTIME=ON. "
                        "Recompile HPX with HPX_WITH_DISTRIBUTED_RUNTIME=ON or "
                        "change the runtime mode.",
                        get_runtime_mode_name(cmdline.rtcfg_.mode_));
#endif
                }

                // Store application defined command line options
                rt->set_app_options(params.desc_cmdline);

                result = run_or_start(blocking, HPX_MOVE(rt), cmdline,
                    params.startup, params.shutdown);
            }
            catch (detail::command_line_error const& e)
            {
                std::cerr << "hpx::init: std::exception caught: " << e.what()
                          << "\n";
                return -1;
            }
            return result;
        }

        ////////////////////////////////////////////////////////////////////////
        template <typename T>
        inline T get_option(std::string const& config, T default_ = T())
        {
            if (!config.empty())
            {
                try
                {
                    return hpx::util::from_string<T>(
                        get_runtime().get_config().get_entry(config, default_));
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
                catch (hpx::util::bad_lexical_cast const&)
                {
                    // do nothing
                }
            }
            return default_;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    int finalize(double shutdown_timeout, double localwait, error_code& ec)
    {
        if (!threads::get_self_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::finalize",
                "this function can be called from an HPX thread only");
            return -1;
        }

        if (!is_running())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::finalize",
                "the runtime system is not active (did you already "
                "call finalize?)");
            return -1;
        }

        if (&ec != &throws)
            ec = make_success_code();

        if (std::abs(localwait + 1.0) < 1e-16)
            localwait = detail::get_option("hpx.finalize_wait_time", -1.0);

        {
            hpx::chrono::high_resolution_timer const t;
            double const start_time = t.elapsed();
            double current;
            do
            {
                current = t.elapsed();
            } while (current - start_time < localwait * 1e-6);
        }

        if (std::abs(shutdown_timeout + 1.0) < 1e-16)
            shutdown_timeout = detail::get_option("hpx.shutdown_timeout", -1.0);

        runtime* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::finalize",
                "the runtime system is not active (did you already "
                "call hpx::stop?)");
            return -1;
        }

        rt->finalize(shutdown_timeout);

        // invoke user supplied finalizer
        if (hpx::on_finalize != nullptr)
        {
            (*hpx::on_finalize)();
        }
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    int disconnect(double shutdown_timeout, double localwait, error_code& ec)
    {
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        if (!threads::get_self_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::disconnect",
                "this function can be called from an HPX thread only");
            return -1;
        }

        if (!is_running())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::disconnect",
                "the runtime system is not active (did you already "
                "call finalize?)");
            return -1;
        }

        if (&ec != &throws)
            ec = make_success_code();

        if (std::abs(localwait + 1.0) < 1e-16)
            localwait = detail::get_option("hpx.finalize_wait_time", -1.0);

        {
            hpx::chrono::high_resolution_timer const t;
            double const start_time = t.elapsed();
            double current;
            do
            {
                current = t.elapsed();
            } while (current - start_time < localwait * 1e-6);
        }

        if (std::abs(shutdown_timeout + 1.0) < 1e-16)
            shutdown_timeout = detail::get_option("hpx.shutdown_timeout", -1.0);

        auto* p = static_cast<components::server::runtime_support*>(
            get_runtime_distributed().get_runtime_support_lva());

        if (nullptr == p)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::disconnect",
                "the runtime system is not active (did you already "
                "call finalize?)");
            return -1;
        }

        p->call_shutdown_functions(true);
        p->call_shutdown_functions(false);

        p->stop(shutdown_timeout, hpx::invalid_id, true);
#else
        HPX_UNUSED(shutdown_timeout);
        HPX_UNUSED(localwait);
        HPX_UNUSED(ec);
#endif

        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    void terminate()
    {
        if (!threads::get_self_ptr())
        {
            // hpx::terminate shouldn't be called from a non-HPX thread
            std::terminate();
        }

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        auto* p = static_cast<components::server::runtime_support*>(
            get_runtime_distributed().get_runtime_support_lva());

        if (nullptr == p)
        {
            // the runtime system is not running, just terminate
            std::terminate();
        }

        p->terminate_all();
#else
        std::terminate();
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    int stop(error_code& ec)
    {
        return hpx::local::stop(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    int suspend(error_code& ec)
    {
        return hpx::local::suspend(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    int resume(error_code& ec)
    {
        return hpx::local::resume(ec);
    }
}    // namespace hpx
