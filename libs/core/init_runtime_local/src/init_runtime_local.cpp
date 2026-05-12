//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2017 Shoshana Jakobovits
//  Copyright (c) 2010-2011 Phillip LeBlanc, Dylan Stark
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/init_runtime_local/detail/init_logging.hpp>
#include <hpx/init_runtime_local/init_runtime_local.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/command_line_handling_local.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>

#if defined(HPX_NATIVE_MIC) || defined(__bgq__)
#include <cstdlib>
#endif

#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if !defined(HPX_WINDOWS)
#include <signal.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    void (*on_finalize)() = nullptr;

    namespace detail {

        int init_helper(hpx::program_options::variables_map& /*vm*/,
            hpx::function<int(int, char**)> const& f)
        {
            std::string cmdline(
                hpx::get_config_entry("hpx.reconstructed_cmd_line", ""));

            using namespace hpx::program_options;
#if defined(HPX_WINDOWS)
            std::vector<std::string> args = split_winmain(cmdline);
#else
            std::vector<std::string> args = split_unix(cmdline);
#endif

            // Copy all arguments which are not hpx related to a temporary array
            std::vector<char*> argv(args.size() + 1);
            std::size_t argcount = 0;
            for (std::size_t i = 0; i != args.size(); ++i)
            {
                if (0 != args[i].find("--hpx:"))
                {
                    argv[argcount++] = const_cast<char*>(args[i].data());
                }
                else if (6 == args[i].find("positional", 6))
                {
                    std::string::size_type const p = args[i].find_first_of('=');
                    if (p != std::string::npos)
                    {
                        args[i] = args[i].substr(p + 1);
                        argv[argcount++] = const_cast<char*>(args[i].data());
                    }
                }
            }

            // add a single nullptr in the end as some application rely on that
            argv[argcount] = nullptr;

            // Invoke custom startup functions
            return f(static_cast<int>(argcount), argv.data());
        }
    }    // namespace detail

    namespace local {

        namespace detail {
            void dump_config::operator()() const
            {
                std::cout << "Configuration after runtime start:\n";
                std::cout << "----------------------------------\n";
                rt_.get().get_config().dump(0, std::cout);
                std::cout << "----------------------------------\n";
            }
        }    // namespace detail

        // Print stack trace and exit.
#if defined(HPX_WINDOWS)
        extern BOOL WINAPI termination_handler(DWORD ctrl_type);
#else
        extern void termination_handler(int signum);
#endif

        int finalize(error_code& ec)
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

            runtime* rt = get_runtime_ptr();
            if (nullptr == rt)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::finalize",
                    "the runtime system is not active (did you already "
                    "call hpx::stop?)");
                return -1;
            }

            rt->finalize(0);

            // invoke user supplied finalizer
            if (hpx::on_finalize != nullptr)
            {
                (*hpx::on_finalize)();
            }

            return 0;
        }

        int stop(error_code& ec)
        {
            if (threads::get_self_ptr())
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::stop",
                    "this function cannot be called from an HPX thread");
                return -1;
            }

            std::unique_ptr<runtime> const rt(
                get_runtime_ptr());    // take ownership!
            if (!rt)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::stop",
                    "the runtime system is not active (did you already "
                    "call hpx::stop?)");
                return -1;
            }

            int const result = rt->wait();

            rt->stop();
            rt->rethrow_exception();

            return result;
        }

        ///////////////////////////////////////////////////////////////////////////
        int suspend(error_code& ec)
        {
            if (threads::get_self_ptr())
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::suspend",
                    "this function cannot be called from an HPX thread");
                return -1;
            }

            runtime* rt = get_runtime_ptr();
            if (nullptr == rt)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::suspend",
                    "the runtime system is not active (did you already "
                    "call hpx::stop?)");
                return -1;
            }

            return rt->suspend();
        }

        ///////////////////////////////////////////////////////////////////////////
        int resume(error_code& ec)
        {
            if (threads::get_self_ptr())
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::resume",
                    "this function cannot be called from an HPX thread");
                return -1;
            }

            runtime* rt = get_runtime_ptr();
            if (nullptr == rt)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::resume",
                    "the runtime system is not active (did you already "
                    "call hpx::stop?)");
                return -1;
            }

            return rt->resume();
        }

        ///////////////////////////////////////////////////////////////////////////
        namespace detail {

            ///////////////////////////////////////////////////////////////////////
            void activate_global_options(
                local::detail::command_line_handling& cmdline)
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
                    util::trace_depth_lock_detection(
                        cmdline.rtcfg_.trace_depth());
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
                util::detail::init_logging_local(cmdline.rtcfg_);
#else
                util::detail::warn_if_logging_requested(cmdline.rtcfg_);
#endif
            }

            ///////////////////////////////////////////////////////////////////////
            void add_startup_functions(hpx::runtime& rt,
                hpx::program_options::variables_map const& vm,
                startup_function_type startup, shutdown_function_type shutdown)
            {
                if (vm.count("hpx:app-config"))
                {
                    std::string const config(
                        vm["hpx:app-config"].as<std::string>());
                    rt.get_config().load_application_configuration(
                        config.c_str());
                }

                if (!!startup)
                    rt.add_startup_function(HPX_MOVE(startup));

                if (!!shutdown)
                    rt.add_shutdown_function(HPX_MOVE(shutdown));

                if (vm.count("hpx:dump-config-initial"))
                {
                    std::cout << "Configuration after runtime construction:\n";
                    std::cout << "-----------------------------------------\n";
                    rt.get_config().dump(0, std::cout);
                    std::cout << "-----------------------------------------\n";
                }

                if (vm.count("hpx:dump-config"))
                    rt.add_startup_function(dump_config(rt));
            }

            ///////////////////////////////////////////////////////////////////////
            int run(hpx::runtime& rt,
                hpx::function<int(
                    hpx::program_options::variables_map& vm)> const& f,
                hpx::program_options::variables_map& vm,
                startup_function_type startup, shutdown_function_type shutdown)
            {
                LPROGRESS_;

                add_startup_functions(
                    rt, vm, HPX_MOVE(startup), HPX_MOVE(shutdown));

                // Run this runtime instance using the given function f.
                if (!f.empty())
                    return rt.run(hpx::bind_front(f, vm));

                // Run this runtime instance without hpx_main
                return rt.run();
            }

            int start(hpx::runtime& rt,
                hpx::function<int(
                    hpx::program_options::variables_map& vm)> const& f,
                hpx::program_options::variables_map& vm,
                startup_function_type startup, shutdown_function_type shutdown)
            {
                LPROGRESS_;

                add_startup_functions(
                    rt, vm, HPX_MOVE(startup), HPX_MOVE(shutdown));

                if (!f.empty())
                {
                    // Run this runtime instance using the given function f.
                    return rt.start(hpx::bind_front(f, vm));
                }

                // Run this runtime instance without hpx_main
                return rt.start();
            }

            int run_or_start(bool blocking, std::unique_ptr<hpx::runtime> rt,
                local::detail::command_line_handling& cfg,
                startup_function_type startup, shutdown_function_type shutdown)
            {
                if (blocking)
                {
                    return run(*rt, cfg.hpx_main_f_, cfg.vm_, HPX_MOVE(startup),
                        HPX_MOVE(shutdown));
                }

                // non-blocking version
                int const result = start(*rt, cfg.hpx_main_f_, cfg.vm_,
                    HPX_MOVE(startup), HPX_MOVE(shutdown));

                // pointer to runtime is stored in TLS
                [[maybe_unused]] hpx::runtime const* p = rt.release();

                return result;
            }

            ////////////////////////////////////////////////////////////////////////
            void init_environment(
                [[maybe_unused]] hpx::util::runtime_configuration const& cfg)
            {
                HPX_UNUSED(hpx::filesystem::initial_path());

                hpx::assertion::set_assertion_handler(
                    &hpx::detail::assertion_handler);
                hpx::util::set_test_failure_handler(
                    &hpx::detail::test_failure_handler);
                hpx::set_custom_exception_info_handler(
                    &hpx::detail::custom_exception_info);
                hpx::serialization::detail::set_save_custom_exception_handler(
                    &hpx::runtime_local::detail::save_custom_exception);
                hpx::serialization::detail::set_load_custom_exception_handler(
                    &hpx::runtime_local::detail::load_custom_exception);
                hpx::set_pre_exception_handler(
                    &hpx::detail::pre_exception_handler);
                hpx::set_thread_termination_handler(
                    [](std::exception_ptr const& e) { report_error(e); });
                hpx::lcos::detail::set_run_on_completed_error_handler(
                    [](std::exception_ptr const& e) {
                        hpx::detail::report_exception_and_terminate(e);
                    });
#if defined(HPX_HAVE_VERIFY_LOCKS)
                hpx::util::set_registered_locks_error_handler(
                    &hpx::detail::registered_locks_error_handler);
                hpx::util::set_register_locks_predicate(
                    &hpx::detail::register_locks_predicate);
#endif
#if !defined(HPX_HAVE_DISABLED_SIGNAL_EXCEPTION_HANDLERS)
                set_error_handlers(cfg);
#endif
                hpx::threads::detail::set_get_default_pool(
                    &hpx::detail::get_default_pool);
                hpx::threads::detail::set_get_default_timer_service(
                    &hpx::detail::get_default_timer_service);
                hpx::threads::detail::set_get_locality_id(&get_locality_id);
                hpx::parallel::execution::detail::set_get_pu_mask(
                    &hpx::detail::get_pu_mask);
                hpx::parallel::execution::detail::set_get_os_thread_count(
                    []() { return hpx::get_os_thread_count(); });

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
                    std::cerr << "hpx::init: can't initialize runtime system "
                                 "more than once! Exiting...\n";
                    return -1;
                }
                return 0;
            }

            ///////////////////////////////////////////////////////////////////////
            int run_or_start(
                hpx::function<int(
                    hpx::program_options::variables_map& vm)> const& f,
                int argc, char** argv, init_params const& params, bool blocking)
            {
                int result;
                try
                {
                    result = ensure_no_runtime_is_up();
                    if (result != 0)
                    {
                        return result;
                    }

                    hpx::local::detail::command_line_handling cmdline{
                        hpx::util::runtime_configuration(
                            argv[0], hpx::runtime_mode::local),
                        params.cfg, f};

                    // scope exception handling to resource partitioner initialization
                    // any exception thrown during run_or_start below are handled
                    // separately
                    try
                    {
                        result = cmdline.call(params.desc_cmdline, argc, argv);

                        init_environment(cmdline.rtcfg_);

                        hpx::threads::policies::detail::affinity_data
                            affinity_data{};
                        affinity_data.init(
                            hpx::util::get_entry_as<std::size_t>(
                                cmdline.rtcfg_, "hpx.os_threads", 0),
                            hpx::util::get_entry_as<std::size_t>(
                                cmdline.rtcfg_, "hpx.cores", 0),
                            hpx::util::get_entry_as<std::size_t>(
                                cmdline.rtcfg_, "hpx.pu_offset", 0),
                            hpx::util::get_entry_as<std::size_t>(
                                cmdline.rtcfg_, "hpx.pu_step", 0),
                            0, cmdline.rtcfg_.get_entry("hpx.affinity", ""),
                            cmdline.rtcfg_.get_entry("hpx.bind", ""),
                            hpx::util::get_entry_as<bool>(
                                cmdline.rtcfg_, "hpx.use_process_mask", false));

                        hpx::resource::partitioner rp =
                            hpx::resource::detail::make_partitioner(
                                params.rp_mode, cmdline.rtcfg_, affinity_data);

                        activate_global_options(cmdline);

                        // check whether HPX should be exited at this point
                        // (parse_result is returning a result > 0, if the program options
                        // contain --hpx:help or --hpx:version, on error result is < 0)
                        if (result != 0)
                        {
                            if (result > 0)
                                result = 0;
                            return result;
                        }

                        rp.assign_cores(hpx::util::get_entry_as<std::size_t>(
                            cmdline.rtcfg_, "hpx.first_used_core", 0));

                        // If thread_pools initialization in user main
                        if (params.rp_callback)
                        {
                            params.rp_callback(rp, cmdline.vm_);
                        }

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
                    LPROGRESS_ << "creating local runtime";
                    rt.reset(new hpx::runtime(cmdline.rtcfg_, true));

                    // Store application defined command line options
                    rt->set_app_options(params.desc_cmdline);

                    result = run_or_start(blocking, HPX_MOVE(rt), cmdline,
                        HPX_MOVE(params.startup), HPX_MOVE(params.shutdown));
                }
                catch (hpx::detail::command_line_error const& e)
                {
                    std::cerr << "hpx::local::init: std::exception caught: "
                              << e.what() << "\n";
                    return -1;
                }
                return result;
            }

            hpx::program_options::options_description const& default_desc(
                char const* desc)
            {
                static hpx::program_options::options_description default_desc_(
                    std::string("Usage: ") + desc + " [options]");
                return default_desc_;
            }
        }    // namespace detail
    }    // namespace local
}    // namespace hpx
