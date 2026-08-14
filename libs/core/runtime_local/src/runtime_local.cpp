//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/config/thread_name.hpp>
#include <hpx/assert.hpp>

#include <hpx/modules/command_line_handling_local.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/static_reinit.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/runtime_local/config_entry.hpp>
#include <hpx/runtime_local/custom_exception_info.hpp>
#include <hpx/runtime_local/debugging.hpp>
#include <hpx/runtime_local/os_thread_type.hpp>
#include <hpx/runtime_local/runtime_local.hpp>
#include <hpx/runtime_local/runtime_local_fwd.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>
#include <hpx/runtime_local/startup_function.hpp>
#include <hpx/runtime_local/state.hpp>
#include <hpx/runtime_local/termination_detection.hpp>
#include <hpx/runtime_local/thread_hooks.hpp>
#include <hpx/runtime_local/thread_mapper.hpp>
#include <hpx/version.hpp>

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#if defined(HPX_HAVE_LOGGING)
namespace hpx::detail {

    void try_log_runtime_threads()
    {
        // This may be used in non-valid runtime states, let it fail silently
        try
        {
            auto rt = hpx::get_runtime_ptr();
            if (rt == nullptr)
                return;

            [[maybe_unused]] auto ret =
                rt->get_thread_manager().enumerate_threads(
                    [](hpx::threads::thread_id_type const& id) -> bool {
                        hpx::threads::thread_data* td = get_thread_id_data(id);
                        auto sched = td->get_scheduler_base();
                        LTM_(debug).format(
                            "Logging all runtime threads: pool({}), "
                            "scheduler({}),"
                            "thread({}), description({}), state({})",
                            sched->get_parent_pool(), sched, id,
                            td->get_description(), td->get_state().state());
                        return true;
                    });
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
        }
    }
}    // namespace hpx::detail
#endif

///////////////////////////////////////////////////////////////////////////////
// Make sure the system gets properly shut down while handling Ctrl-C and other
// system signals
#if defined(HPX_WINDOWS)

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    void handle_termination(char const* reason)
    {
        if (hpx::threads::coroutines::attach_debugger_on_sigv)
        {
            util::attach_debugger();
        }

        if (hpx::threads::coroutines::diagnostics_on_terminate)
        {
            int const verbosity = hpx::threads::coroutines::exception_verbosity;

            if (verbosity >= 2)
            {
                std::cerr << full_build_string() << "\n" << std::flush;
            }

#if defined(HPX_HAVE_STACKTRACES)
            if (verbosity >= 1)
            {
                std::size_t const trace_depth =
                    util::from_string<std::size_t>(get_config_entry(
                        "hpx.trace_depth", HPX_HAVE_THREAD_BACKTRACE_DEPTH));
                std::cerr << "{stack-trace}: " << hpx::util::trace(trace_depth)
                          << "\n"
                          << std::flush;
            }
#endif

#if defined(HPX_HAVE_LOGGING)
            LRT_(debug).format("Terminating due to system signal({})", reason);
            hpx::detail::try_log_runtime_threads();
#endif

            std::cerr << "{what}: " << (reason ? reason : "Unknown reason")
                      << "\n"
                      << std::flush;
        }
    }

    HPX_CORE_EXPORT BOOL WINAPI termination_handler(DWORD ctrl_type)
    {
        switch (ctrl_type)
        {
        case CTRL_C_EVENT:
            handle_termination("Ctrl-C");
            break;

        case CTRL_BREAK_EVENT:
            handle_termination("Ctrl-Break");
            break;

        case CTRL_CLOSE_EVENT:
            handle_termination("Ctrl-Close");
            break;

        case CTRL_LOGOFF_EVENT:
            handle_termination("Logoff");
            break;

        case CTRL_SHUTDOWN_EVENT:
            handle_termination("Shutdown");
            break;

        default:
            break;
        }
        return FALSE;
    }
}    // namespace hpx

#else

#include <signal.h>
#include <stdlib.h>
#include <string.h>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    [[noreturn]] HPX_CORE_EXPORT void termination_handler(int signum)
    {
        if (signum != SIGINT &&
            hpx::threads::coroutines::attach_debugger_on_sigv)
        {
            util::attach_debugger();
        }

        if (hpx::threads::coroutines::diagnostics_on_terminate)
        {
            int const verbosity = hpx::threads::coroutines::exception_verbosity;
            char* reason = strsignal(signum);

            if (verbosity >= 2)
            {
                std::cerr << full_build_string() << "\n" << std::flush;
            }

#if defined(HPX_HAVE_STACKTRACES)
            if (verbosity >= 1)
            {
                std::size_t const trace_depth =
                    util::from_string<std::size_t>(get_config_entry(
                        "hpx.trace_depth", HPX_HAVE_THREAD_BACKTRACE_DEPTH));
                std::cerr << "{stack-trace}: " << hpx::util::trace(trace_depth)
                          << "\n"
                          << std::flush;
            }
#endif

#if defined(HPX_HAVE_LOGGING)
            LRT_(debug).format("Terminating due to system signal({})", signum);
            hpx::detail::try_log_runtime_threads();
#endif

            std::cerr << "{what}: " << (reason ? reason : "Unknown reason")
                      << "\n"
                      << std::flush;
        }
        std::abort();
    }
}    // namespace hpx

#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    [[noreturn]] HPX_CORE_EXPORT void HPX_CDECL new_handler()
    {
        HPX_THROW_BAD_ALLOC("new_handler");
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        namespace {

            // Sometimes the HPX library gets simply unloaded as a result of
            // some extreme error handling. Avoid hangs in the end by setting a
            // flag.
            bool exit_called = false;
        }    // namespace

        void on_exit() noexcept
        {
            exit_called = true;
        }

        void on_abort(int) noexcept
        {
            exit_called = true;
            std::abort();
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    void set_error_handlers(hpx::util::runtime_configuration const& cfg)
    {
        // initialize global variables
        hpx::threads::coroutines::attach_debugger_on_sigv =
            hpx::util::get_entry_as<std::string>(
                cfg, "hpx.attach_debugger", "") == "exception";
        hpx::threads::coroutines::diagnostics_on_terminate =
            hpx::util::get_entry_as<std::string>(
                cfg, "hpx.diagnostics_on_terminate", "1") == "1";
        hpx::threads::coroutines::exception_verbosity =
            hpx::util::get_entry_as<int>(cfg, "hpx.exception_verbosity", 2);
        hpx::threads::coroutines::exception_verbosity = 0;
#if defined(HPX_HAVE_STACKTRACES) && defined(HPX_HAVE_THREAD_BACKTRACE_DEPTH)
        hpx::threads::coroutines::exception_verbosity =
            hpx::util::get_entry_as<int>(
                cfg, "hpx.trace_depth", HPX_HAVE_THREAD_BACKTRACE_DEPTH);
#endif

#if defined(HPX_WINDOWS)
        if (hpx::util::get_entry_as<int>(cfg, "hpx.handle_signals", 1))
        {
            [[maybe_unused]] auto const prev_signal =
                std::signal(SIGABRT, detail::on_abort);
            HPX_ASSERT(prev_signal != SIG_ERR);
        }

        // Set console control handler to allow server to be stopped.
        SetConsoleCtrlHandler(hpx::termination_handler, TRUE);
#else
        if (hpx::util::get_entry_as<int>(cfg, "hpx.handle_signals", 1))
        {
            [[maybe_unused]] auto const prev_signal =
                std::signal(SIGABRT, detail::on_abort);
            HPX_ASSERT(prev_signal != SIG_ERR);

            struct sigaction new_action;
            new_action.sa_handler = hpx::termination_handler;
            sigemptyset(&new_action.sa_mask);
            new_action.sa_flags = 0;

            sigaction(SIGINT, &new_action, nullptr);    // Interrupted
            sigaction(SIGBUS, &new_action, nullptr);    // Bus error
            sigaction(
                SIGFPE, &new_action, nullptr);    // Floating point exception
            sigaction(SIGILL, &new_action, nullptr);     // Illegal instruction
            sigaction(SIGPIPE, &new_action, nullptr);    // Bad pipe
            sigaction(SIGSEGV, &new_action, nullptr);    // Segmentation fault
            sigaction(SIGSYS, &new_action, nullptr);     // Bad syscall

            hpx::threads::coroutines::register_signal_handler = true;
        }
        else
        {
            hpx::threads::coroutines::register_signal_handler = false;
        }
#endif

        if (hpx::util::get_entry_as<int>(cfg, "hpx.handle_failed_new", 1))
        {
            std::set_new_handler(hpx::new_handler);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace strings {

        inline constexpr char const* const runtime_state_names[] = {
            "state::invalid",         // -1
            "state::initialized",     // 0
            "state::pre_startup",     // 1
            "state::startup",         // 2
            "state::pre_main",        // 3
            "state::starting",        // 4
            "state::running",         // 5
            "state::suspended",       // 6
            "state::pre_sleep",       // 7
            "state::sleeping",        // 8
            "state::pre_shutdown",    // 9
            "state::shutdown",        // 10
            "state::stopping",        // 11
            "state::terminating",     // 12
            "state::stopped"          // 13
        };
    }

    char const* get_runtime_state_name(state s) noexcept
    {
        if (s < state::invalid || s >= state::last_valid_runtime_state)
        {
            return "invalid (value out of bounds)";
        }
        return strings::runtime_state_names[static_cast<int>(s) + 1];
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace {

        threads::policies::callback_notifier::on_startstop_type
            global_on_start_func;
        threads::policies::callback_notifier::on_startstop_type
            global_on_stop_func;
        threads::policies::callback_notifier::on_error_type
            global_on_error_func;
    }    // namespace

    ///////////////////////////////////////////////////////////////////////////
    runtime::runtime(hpx::util::runtime_configuration rtcfg, bool initialize)
      : rtcfg_(HPX_MOVE(rtcfg))
      , instance_number_(++instance_number_counter_)
      , thread_support_(std::make_unique<util::thread_mapper>())
      , topology_(resource::get_partitioner().get_topology())
      , state_(state::invalid)
      , on_start_func_(global_on_start_func)
      , on_stop_func_(global_on_stop_func)
      , on_error_func_(global_on_error_func)
      , result_(0)
      , main_pool_(std::make_unique<util::io_service_pool>(
            main_pool_notifier_, "main_pool"))
#ifdef HPX_HAVE_IO_POOL
      , io_pool_(std::make_unique<util::io_service_pool>(
            io_pool_notifier_, "io_pool"))
#endif
#ifdef HPX_HAVE_TIMER_POOL
      , timer_pool_(std::make_unique<util::io_service_pool>(
            timer_pool_notifier_, "timer_pool"))
#endif
      , stop_called_(false)
      , stop_done_(false)
    {
        LPROGRESS_;

        // set notification policies only after the object was completely
        // initialized
        runtime::set_notification_policies(
            runtime::get_notification_policy(
                "worker-thread", runtime_local::os_thread_type::worker_thread),
#ifdef HPX_HAVE_IO_POOL
            runtime::get_notification_policy(
                "io-thread", runtime_local::os_thread_type::io_thread),
#endif
#ifdef HPX_HAVE_TIMER_POOL
            runtime::get_notification_policy(
                "timer-thread", runtime_local::os_thread_type::timer_thread),
#endif
            threads::detail::network_background_callback_type{});

        init_global_data();
        util::reinit_construct();

        if (initialize)
        {
            runtime::init();
        }
    }

    // this constructor is called by the distributed runtime only
    runtime::runtime(hpx::util::runtime_configuration rtcfg)
      : rtcfg_(HPX_MOVE(rtcfg))
      , instance_number_(++instance_number_counter_)
      , thread_support_(std::make_unique<util::thread_mapper>())
      , topology_(resource::get_partitioner().get_topology())
      , state_(state::invalid)
      , on_start_func_(global_on_start_func)
      , on_stop_func_(global_on_stop_func)
      , on_error_func_(global_on_error_func)
      , result_(0)
      , main_pool_(std::make_unique<util::io_service_pool>(
            main_pool_notifier_, "main_pool"))
#ifdef HPX_HAVE_IO_POOL
      , io_pool_(std::make_unique<util::io_service_pool>(
            io_pool_notifier_, "io_pool"))
#endif
#ifdef HPX_HAVE_TIMER_POOL
      , timer_pool_(std::make_unique<util::io_service_pool>(
            timer_pool_notifier_, "timer_pool"))
#endif
      , stop_called_(false)
      , stop_done_(false)
    {
        init_global_data();
        util::reinit_construct();

        LPROGRESS_;
    }

    void runtime::set_notification_policies(notification_policy_type&& notifier,
#ifdef HPX_HAVE_IO_POOL
        notification_policy_type&& io_pool_notifier,
#endif
#ifdef HPX_HAVE_TIMER_POOL
        notification_policy_type&& timer_pool_notifier,
#endif
        threads::detail::network_background_callback_type const&
            network_background_callback)
    {
        notifier_ = HPX_MOVE(notifier);

        main_pool_->init(1);
#ifdef HPX_HAVE_IO_POOL
        io_pool_notifier_ = HPX_MOVE(io_pool_notifier);
        io_pool_->init(rtcfg_.get_thread_pool_size("io_pool"));
#endif
#ifdef HPX_HAVE_TIMER_POOL
        timer_pool_notifier_ = HPX_MOVE(timer_pool_notifier);
        timer_pool_->init(rtcfg_.get_thread_pool_size("timer_pool"));
#endif

        thread_manager_.reset(new hpx::threads::threadmanager(rtcfg_,
#ifdef HPX_HAVE_TIMER_POOL
            *timer_pool_,
#endif
            notifier_, network_background_callback));
    }

    void runtime::init()
    {
        LPROGRESS_;

        try
        {
            // now create all threadmanager pools
            thread_manager_->create_pools();

            // this initializes the used_processing_units_ mask
            thread_manager_->init();

            // copy over all startup functions registered so far
            for (startup_function_type& f :
                detail::global_pre_startup_functions())
            {
                add_pre_startup_function(HPX_MOVE(f));
            }
            detail::global_pre_startup_functions().clear();

            for (startup_function_type& f : detail::global_startup_functions())
            {
                add_startup_function(HPX_MOVE(f));
            }
            detail::global_startup_functions().clear();

            for (shutdown_function_type& f :
                detail::global_pre_shutdown_functions())
            {
                add_pre_shutdown_function(HPX_MOVE(f));
            }
            detail::global_pre_shutdown_functions().clear();

            for (shutdown_function_type& f :
                detail::global_shutdown_functions())
            {
                add_shutdown_function(HPX_MOVE(f));
            }
            detail::global_shutdown_functions().clear();
        }
        catch (std::exception const& e)
        {
            // errors at this point need to be reported directly
            detail::report_exception_and_terminate(e);
        }
        catch (...)
        {
            // errors at this point need to be reported directly
            detail::report_exception_and_terminate(std::current_exception());
        }

        // set state to initialized
        set_state(state::initialized);
    }

    runtime::~runtime()
    {
        LRT_(debug).format("~runtime_local(entering)");

        // stop all services
        thread_manager_->stop();    // stops timer_pool_ as well
#ifdef HPX_HAVE_IO_POOL
        io_pool_->stop();
#endif

        // Wait for all threads to complete before destroying allocators.
        // This ensures that collective operations and other background work
        // finish before reinit_destruct() destroys component heaps.
        // Fixes issue #6776: crash in one_size_heap_list::alloc on macOS Debug
        thread_manager_->wait();

        LRT_(debug).format("~runtime_local(finished)");

        LPROGRESS_;

        // allow to reuse instance number if this was the only instance
        if (0 == instance_number_counter_)
            --instance_number_counter_;

        util::reinit_destruct();
        resource::detail::delete_partitioner();
    }

    void runtime::on_exit(hpx::function<void()> const& f)
    {
        std::lock_guard<std::mutex> l(mtx_);
        on_exit_functions_.push_back(f);
    }

    void runtime::starting()
    {
        state_.store(hpx::state::pre_main);
    }

    void runtime::stopping()
    {
        state_.store(hpx::state::stopped);

        std::lock_guard<std::mutex> l(mtx_);
        for (auto const& f : on_exit_functions_)
            f();
    }

    bool runtime::stopped() const
    {
        return state_.load() == hpx::state::stopped;
    }

    hpx::util::runtime_configuration& runtime::get_config()
    {
        return rtcfg_;
    }

    hpx::util::runtime_configuration const& runtime::get_config() const
    {
        return rtcfg_;
    }

    std::size_t runtime::get_instance_number() const
    {
        return static_cast<std::size_t>(instance_number_);
    }

    state runtime::get_state() const
    {
        return state_.load();
    }

    threads::topology const& runtime::get_topology() const
    {
        return topology_;
    }

    void runtime::set_state(state s)
    {
        LPROGRESS_ << get_runtime_state_name(s);
        state_.store(s);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::atomic<int> runtime::instance_number_counter_(-1);

    ///////////////////////////////////////////////////////////////////////////
    namespace {
        std::uint64_t& runtime_uptime()
        {
            static std::uint64_t uptime = 0;
            return uptime;
        }
    }    // namespace

    void runtime::init_global_data()
    {
        runtime*& runtime_ = get_runtime_ptr();
        HPX_ASSERT(!runtime_);
        HPX_ASSERT(nullptr == threads::thread_self::get_self());

        runtime_ = this;
        runtime_uptime() = hpx::chrono::high_resolution_clock::now();
    }

    void runtime::deinit_global_data()
    {
        runtime*& runtime_ = get_runtime_ptr();
        runtime_uptime() = 0;
        runtime_ = nullptr;
    }

    std::uint64_t runtime::get_system_uptime()
    {
        auto const diff = static_cast<std::int64_t>(
            hpx::chrono::high_resolution_clock::now() - runtime_uptime());
        return diff < 0LL ? 0ULL : static_cast<std::uint64_t>(diff);
    }

    threads::policies::callback_notifier::on_startstop_type
    runtime::on_start_func() const
    {
        return on_start_func_;
    }

    threads::policies::callback_notifier::on_startstop_type
    runtime::on_stop_func() const
    {
        return on_stop_func_;
    }

    threads::policies::callback_notifier::on_error_type runtime::on_error_func()
        const
    {
        return on_error_func_;
    }

    threads::policies::callback_notifier::on_startstop_type
    runtime::on_start_func(
        threads::policies::callback_notifier::on_startstop_type&& f)
    {
        threads::policies::callback_notifier::on_startstop_type newf =
            HPX_MOVE(f);
        std::swap(on_start_func_, newf);
        return newf;
    }

    threads::policies::callback_notifier::on_startstop_type
    runtime::on_stop_func(
        threads::policies::callback_notifier::on_startstop_type&& f)
    {
        threads::policies::callback_notifier::on_startstop_type newf =
            HPX_MOVE(f);
        std::swap(on_stop_func_, newf);
        return newf;
    }

    threads::policies::callback_notifier::on_error_type runtime::on_error_func(
        threads::policies::callback_notifier::on_error_type&& f)
    {
        threads::policies::callback_notifier::on_error_type newf = HPX_MOVE(f);
        std::swap(on_error_func_, newf);
        return newf;
    }

    std::uint32_t runtime::get_locality_id(error_code& /* ec */) const
    {
        return 0;
    }

    std::size_t runtime::get_num_worker_threads() const
    {
        HPX_ASSERT(thread_manager_);
        return thread_manager_->get_os_thread_count();
    }

    std::uint32_t runtime::get_num_localities(
        hpx::launch::sync_policy, error_code& /* ec */) const
    {
        return 1;
    }

    std::uint32_t runtime::get_initial_num_localities() const
    {
        return 1;
    }

    hpx::future<std::uint32_t> runtime::get_num_localities() const
    {
        return make_ready_future(static_cast<std::uint32_t>(1));
    }

    std::string runtime::get_locality_name() const
    {
        return "console";
    }

    std::uint32_t runtime::assign_cores(std::string const&, std::uint32_t)
    {
        return 0;
    }

    std::uint32_t runtime::assign_cores()
    {
        return static_cast<std::uint32_t>(
            hpx::resource::get_partitioner().assign_cores(0));
    }

    ///////////////////////////////////////////////////////////////////////////
    threads::policies::callback_notifier::on_startstop_type
    get_thread_on_start_func()
    {
        if (runtime const* rt = get_runtime_ptr(); nullptr != rt)
        {
            return rt->on_start_func();
        }
        return global_on_start_func;
    }

    threads::policies::callback_notifier::on_startstop_type
    get_thread_on_stop_func()
    {
        if (runtime const* rt = get_runtime_ptr(); nullptr != rt)
        {
            return rt->on_stop_func();
        }
        return global_on_stop_func;
    }

    threads::policies::callback_notifier::on_error_type
    get_thread_on_error_func()
    {
        if (runtime const* rt = get_runtime_ptr(); nullptr != rt)
        {
            return rt->on_error_func();
        }
        return global_on_error_func;
    }

    threads::policies::callback_notifier::on_startstop_type
    register_thread_on_start_func(
        threads::policies::callback_notifier::on_startstop_type&& f)
    {
        if (runtime* rt = get_runtime_ptr(); nullptr != rt)
        {
            return rt->on_start_func(HPX_MOVE(f));
        }

        threads::policies::callback_notifier::on_startstop_type newf =
            HPX_MOVE(f);
        std::swap(global_on_start_func, newf);
        return newf;
    }

    threads::policies::callback_notifier::on_startstop_type
    register_thread_on_stop_func(
        threads::policies::callback_notifier::on_startstop_type&& f)
    {
        if (runtime* rt = get_runtime_ptr(); nullptr != rt)
        {
            return rt->on_stop_func(HPX_MOVE(f));
        }

        threads::policies::callback_notifier::on_startstop_type newf =
            HPX_MOVE(f);
        std::swap(global_on_stop_func, newf);
        return newf;
    }

    threads::policies::callback_notifier::on_error_type
    register_thread_on_error_func(
        threads::policies::callback_notifier::on_error_type&& f)
    {
        if (runtime* rt = get_runtime_ptr(); nullptr != rt)
        {
            return rt->on_error_func(HPX_MOVE(f));
        }

        threads::policies::callback_notifier::on_error_type newf = HPX_MOVE(f);
        std::swap(global_on_error_func, newf);
        return newf;
    }

    ///////////////////////////////////////////////////////////////////////////
    runtime& get_runtime()
    {
        HPX_ASSERT(get_runtime_ptr() != nullptr);
        return *get_runtime_ptr();
    }

    runtime*& get_runtime_ptr()
    {
        static runtime* runtime_ = nullptr;
        return runtime_;
    }

    std::string get_thread_name()
    {
        std::string& thread_name = detail::thread_name();
        if (thread_name.empty())
        {
            return "<unknown>";
        }
        return thread_name;
    }

    // Register the current kernel thread with HPX, this should be done once for
    // each external OS-thread intended to invoke HPX functionality. Calling
    // this function more than once will silently fail (will return false).
    bool register_thread(runtime* rt, char const* name, error_code& ec)
    {
        HPX_ASSERT(rt);
        return rt->register_thread(name, 0, true, ec);
    }

    // Unregister the thread from HPX, this should be done once in
    // the end before the external thread exists.
    void unregister_thread(runtime* rt)
    {
        HPX_ASSERT(rt);
        rt->unregister_thread();
    }

    // Access data for a given OS thread that was previously registered by
    // \a register_thread. This function must be called from a thread that was
    // previously registered with the runtime.
    runtime_local::os_thread_data get_os_thread_data(std::string const& label)
    {
        return get_runtime().get_os_thread_data(label);
    }

    /// Enumerate all OS threads that have registered with the runtime.
    bool enumerate_os_threads(
        hpx::function<bool(os_thread_data const&)> const& f)
    {
        return get_runtime().enumerate_os_threads(f);
    }

    ///////////////////////////////////////////////////////////////////////////
    void report_error(std::size_t num_thread, std::exception_ptr const& e)
    {
        // Early and late exceptions
        if (!threads::threadmanager_is(hpx::state::running))
        {
            if (hpx::runtime* rt = hpx::get_runtime_ptr(); rt != nullptr)
            {
                rt->report_error(num_thread, e);
            }
            else
            {
                detail::report_exception_and_terminate(e);
            }
            return;
        }

        get_runtime().get_thread_manager().report_error(num_thread, e);
    }

    void report_error(std::exception_ptr const& e)
    {
        // Early and late exceptions
        if (!threads::threadmanager_is(hpx::state::running))
        {
            if (hpx::runtime* rt = hpx::get_runtime_ptr(); rt != nullptr)
            {
                rt->report_error(static_cast<std::size_t>(-1), e);
            }
            else
            {
                detail::report_exception_and_terminate(e);
            }
            return;
        }

        std::size_t const num_thread = hpx::get_worker_thread_num();
        get_runtime().get_thread_manager().report_error(num_thread, e);
    }

    bool register_on_exit(hpx::function<void()> const& f)
    {
        if (runtime* rt = get_runtime_ptr(); rt != nullptr)
        {
            rt->on_exit(f);
            return true;
        }
        return false;
    }

    std::size_t get_runtime_instance_number()
    {
        runtime const* rt = get_runtime_ptr();
        return (rt == nullptr) ? 0 : rt->get_instance_number();
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string get_config_entry(
        std::string const& key, std::string const& dflt)
    {
        if (runtime const* rt = get_runtime_ptr(); rt != nullptr)
        {
            return rt->get_config().get_entry(key, dflt);
        }
        return dflt;
    }

    std::string get_config_entry(std::string const& key, std::size_t dflt)
    {
        if (runtime const* rt = get_runtime_ptr(); rt != nullptr)
        {
            return rt->get_config().get_entry(key, dflt);
        }
        return std::to_string(dflt);
    }

    // set entries
    void set_config_entry(std::string const& key, std::string const& value)
    {
        if (runtime* rt = get_runtime_ptr(); rt != nullptr)
        {
            rt->get_config().add_entry(key, value);
        }
    }

    void set_config_entry(std::string const& key, std::size_t value)
    {
        set_config_entry(key, std::to_string(value));
    }

    void set_config_entry_callback(std::string const& key,
        hpx::function<void(std::string const&, std::string const&)> const&
            callback)
    {
        if (runtime* rt = get_runtime_ptr(); rt != nullptr)
        {
            rt->get_config().add_notification_callback(key, callback);
        }
    }

    namespace util {

        ///////////////////////////////////////////////////////////////////////////
        // retrieve the command line arguments for the current locality
        bool retrieve_commandline_arguments(
            hpx::program_options::options_description const& app_options,
            hpx::program_options::variables_map& vm)
        {
            // The command line for this application instance is available from
            // this configuration section:
            //
            //     [hpx]
            //     cmd_line=....
            //
            std::string cmdline;

            hpx::util::section const& cfg = hpx::get_runtime().get_config();
            if (cfg.has_entry("hpx.cmd_line"))
                cmdline = cfg.get_entry("hpx.cmd_line");

            return hpx::local::detail::parse_commandline(cfg, app_options,
                cmdline, vm, util::commandline_error_mode::allow_unregistered);
        }

        ///////////////////////////////////////////////////////////////////////////
        // retrieve the command line arguments for the current locality
        bool retrieve_commandline_arguments(
            std::string const& appname, hpx::program_options::variables_map& vm)
        {
            using hpx::program_options::options_description;

            options_description const desc_commandline(
                "Usage: " + appname + " [options]");

            return retrieve_commandline_arguments(desc_commandline, vm);
        }
    }    // namespace util

    ///////////////////////////////////////////////////////////////////////////
    std::size_t get_os_thread_count()
    {
        runtime* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_os_thread_count()",
                "the runtime system has not been initialized yet");
        }
        return rt->get_config().get_os_thread_count();
    }

    bool is_scheduler_numa_sensitive()
    {
        if (get_runtime_ptr() != nullptr)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::is_scheduler_numa_sensitive",
                "the runtime system has not been initialized yet");
        }
        return static_cast<std::size_t>(-1) != get_worker_thread_num();
    }

    ///////////////////////////////////////////////////////////////////////////
    bool is_running()
    {
        if (runtime const* rt = get_runtime_ptr(); rt != nullptr)
        {
            return rt->get_state() == hpx::state::running;
        }
        return false;
    }

    bool is_stopped()
    {
        if (!detail::exit_called)
        {
            if (runtime const* rt = get_runtime_ptr(); rt != nullptr)
            {
                return rt->get_state() == hpx::state::stopped;
            }
        }
        return true;    // assume stopped
    }

    bool is_stopped_or_shutting_down()
    {
        if (runtime const* rt = get_runtime_ptr();
            !detail::exit_called && nullptr != rt)
        {
            state const st = rt->get_state();
            return st >= hpx::state::shutdown;
        }
        return true;    // assume stopped
    }

    bool tolerate_node_faults()
    {
#ifdef HPX_HAVE_FAULT_TOLERANCE
        return true;
#else
        return false;
#endif
    }

    bool is_starting()
    {
        runtime const* rt = get_runtime_ptr();
        return nullptr != rt ? rt->get_state() <= hpx::state::startup : true;
    }

    bool is_pre_startup()
    {
        runtime const* rt = get_runtime_ptr();
        return nullptr != rt ? rt->get_state() < hpx::state::startup : true;
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    std::string expand(std::string const& in)
    {
        return get_runtime().get_config().expand(in);
    }

    void expand(std::string& in)
    {
        get_runtime().get_config().expand(
            in, static_cast<std::string::size_type>(-1));
    }
}    // namespace hpx::util

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads {

    threadmanager& get_thread_manager()
    {
        return get_runtime().get_thread_manager();
    }

    // shortcut for runtime_configuration::get_default_stack_size
    std::ptrdiff_t get_default_stack_size()
    {
        return get_runtime().get_config().get_default_stack_size();
    }

    // shortcut for runtime_configuration::get_stack_size
    std::ptrdiff_t get_stack_size(threads::thread_stacksize stacksize)
    {
        if (stacksize == threads::thread_stacksize::current)
            return threads::get_self_stacksize();

        return get_runtime().get_config().get_stack_size(stacksize);
    }

    void reset_thread_distribution()
    {
        get_runtime().get_thread_manager().reset_thread_distribution();
    }

    void set_scheduler_mode(threads::policies::scheduler_mode m)
    {
        get_runtime().get_thread_manager().set_scheduler_mode(
            m, hpx::resource::get_partitioner().get_used_pus_mask());
    }

    void add_scheduler_mode(threads::policies::scheduler_mode m)
    {
        get_runtime().get_thread_manager().add_scheduler_mode(
            m, hpx::resource::get_partitioner().get_used_pus_mask());
    }

    void add_remove_scheduler_mode(
        threads::policies::scheduler_mode to_add_mode,
        threads::policies::scheduler_mode to_remove_mode)
    {
        get_runtime().get_thread_manager().add_remove_scheduler_mode(
            to_add_mode, to_remove_mode,
            hpx::resource::get_partitioner().get_used_pus_mask());
    }

    void remove_scheduler_mode(threads::policies::scheduler_mode m)
    {
        get_runtime().get_thread_manager().remove_scheduler_mode(
            m, hpx::resource::get_partitioner().get_used_pus_mask());
    }

    void set_scheduler_mode(threads::policies::scheduler_mode m,
        hpx::threads::mask_cref_type pu_mask)
    {
        get_runtime().get_thread_manager().set_scheduler_mode(m, pu_mask);
    }

    void add_scheduler_mode(threads::policies::scheduler_mode m,
        hpx::threads::mask_cref_type pu_mask)
    {
        get_runtime().get_thread_manager().add_scheduler_mode(m, pu_mask);
    }

    void add_remove_scheduler_mode(
        threads::policies::scheduler_mode to_add_mode,
        threads::policies::scheduler_mode to_remove_mode,
        hpx::threads::mask_cref_type pu_mask)
    {
        get_runtime().get_thread_manager().add_remove_scheduler_mode(
            to_add_mode, to_remove_mode, pu_mask);
    }

    void remove_scheduler_mode(threads::policies::scheduler_mode m,
        hpx::threads::mask_cref_type pu_mask)
    {
        get_runtime().get_thread_manager().remove_scheduler_mode(m, pu_mask);
    }
}    // namespace hpx::threads

///////////////////////////////////////////////////////////////////////////////
namespace hpx {
    std::uint64_t get_system_uptime()
    {
        return runtime::get_system_uptime();
    }

    hpx::util::runtime_configuration const& get_config()
    {
        return get_runtime().get_config();
    }

    hpx::util::io_service_pool* get_thread_pool(
        char const* name, char const* name_suffix)
    {
        std::string full_name(name);
        full_name += name_suffix;
        return get_runtime().get_thread_pool(full_name.c_str());
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Return true if networking is enabled.
    bool is_networking_enabled()
    {
        runtime* rt = get_runtime_ptr();
        if (nullptr != rt)
        {
            return rt->is_networking_enabled();
        }
        return true;    // be on the safe side, enable networking
    }
}    // namespace hpx

#if defined(_WIN64) && defined(HPX_DEBUG) &&                                   \
    !defined(HPX_HAVE_FIBER_BASED_COROUTINES)
#include <io.h>
#endif

namespace hpx {
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // There is no need to protect these global from thread concurrent
        // access as they are access during early startup only.
        std::list<startup_function_type>& global_pre_startup_functions()
        {
            static std::list<startup_function_type>
                global_pre_startup_functions_;
            return global_pre_startup_functions_;
        }

        std::list<startup_function_type>& global_startup_functions()
        {
            static std::list<startup_function_type> global_startup_functions_;
            return global_startup_functions_;
        }

        std::list<shutdown_function_type>& global_pre_shutdown_functions()
        {
            static std::list<shutdown_function_type>
                global_pre_shutdown_functions_;
            return global_pre_shutdown_functions_;
        }

        std::list<shutdown_function_type>& global_shutdown_functions()
        {
            static std::list<shutdown_function_type> global_shutdown_functions_;
            return global_shutdown_functions_;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    void register_pre_startup_function(startup_function_type f)
    {
        runtime* rt = get_runtime_ptr();
        if (nullptr != rt)
        {
            if (rt->get_state() > hpx::state::pre_startup)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "register_pre_startup_function",
                    "Too late to register a new pre-startup function.");
            }
            rt->add_pre_startup_function(HPX_MOVE(f));
        }
        else
        {
            detail::global_pre_startup_functions().push_back(HPX_MOVE(f));
        }
    }

    void register_startup_function(startup_function_type f)
    {
        if (runtime* rt = get_runtime_ptr(); nullptr != rt)
        {
            if (rt->get_state() > hpx::state::startup)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "register_startup_function",
                    "Too late to register a new startup function.");
            }
            rt->add_startup_function(HPX_MOVE(f));
        }
        else
        {
            detail::global_startup_functions().push_back(HPX_MOVE(f));
        }
    }

    void register_pre_shutdown_function(shutdown_function_type f)
    {
        if (runtime* rt = get_runtime_ptr(); nullptr != rt)
        {
            if (rt->get_state() > hpx::state::pre_shutdown)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "register_pre_shutdown_function",
                    "Too late to register a new pre-shutdown function.");
            }
            rt->add_pre_shutdown_function(HPX_MOVE(f));
        }
        else
        {
            detail::global_pre_shutdown_functions().push_back(HPX_MOVE(f));
        }
    }

    void register_shutdown_function(shutdown_function_type f)
    {
        if (runtime* rt = get_runtime_ptr(); nullptr != rt)
        {
            if (rt->get_state() > hpx::state::shutdown)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "register_shutdown_function",
                    "Too late to register a new shutdown function.");
            }
            rt->add_shutdown_function(HPX_MOVE(f));
        }
        else
        {
            detail::global_shutdown_functions().push_back(HPX_MOVE(f));
        }
    }

    void runtime::call_startup_functions(bool pre_startup)
    {
        if (pre_startup)
        {
            set_state(hpx::state::pre_startup);
            for (startup_function_type& f : pre_startup_functions_)
            {
                f();
            }
        }
        else
        {
            set_state(hpx::state::startup);
            for (startup_function_type& f : startup_functions_)
            {
                f();
            }
        }
    }

    namespace detail {

        void handle_print_bind(std::size_t num_threads)
        {
            threads::topology const& top = threads::create_topology();
            auto const& rp = hpx::resource::get_partitioner();
            auto const& tm = get_runtime().get_thread_manager();

            {
                // make sure all output is kept together
                std::ostringstream strm;

                strm << std::string(79, '*') << '\n';
                strm << "locality: " << hpx::get_locality_id() << '\n';
                for (std::size_t i = 0; i != num_threads; ++i)
                {
                    // print the mask for the current PU
                    threads::mask_cref_type pu_mask = rp.get_pu_mask(i);

                    if (!threads::any(pu_mask))
                    {
                        strm << std::setw(4) << i    //-V112
                             << ": thread binding disabled\n";
                    }
                    else
                    {
                        std::string pool_name = tm.get_pool(i).get_pool_name();
                        top.print_affinity_mask(strm, i, pu_mask, pool_name);
                    }

                    // Make sure the mask does not contradict the CPU bindings
                    // returned by the system (see #973: Would like option to
                    // report HWLOC bindings).
                    error_code ec(throwmode::lightweight);
                    std::thread& blob = tm.get_os_thread_handle(i);
                    threads::mask_type const boundcpu =
                        top.get_cpubind_mask(blob, ec);

                    // The masks reported by HPX must be the same as the ones
                    // reported from HWLOC.
                    if (!ec && threads::any(boundcpu) &&
                        !threads::equal(boundcpu, pu_mask, num_threads))
                    {
                        std::string const boundcpu_str =
                            threads::to_string(boundcpu);
                        std::string const pu_mask_str =
                            threads::to_string(pu_mask);

                        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                            "handle_print_bind",
                            "unexpected mismatch between locality {1}: "
                            "binding reported from HWLOC({2}) and HPX({3}) "
                            "on thread {4}",
                            hpx::get_locality_id(), boundcpu_str, pu_mask_str,
                            i);
                    }
                }

                std::cout << strm.str() << std::flush;
            }
        }
    }    // namespace detail

    threads::thread_result_type runtime::run_helper(
        hpx::function<runtime::hpx_main_function_type> const& func, int& result,
        bool call_startup, void (*handle_print_bind)(std::size_t))
    {
        bool caught_exception = false;
        try
        {
            // no need to do late command line handling if this is called from
            // the distributed runtime
            if (handle_print_bind != nullptr)
            {
                result = hpx::local::detail::handle_late_commandline_options(
                    get_config(), get_app_options(), handle_print_bind);
                if (result)
                {
                    HPX_UNUSED(lbt_ << "runtime_local::run_helper: bootstrap "
                                       "aborted, bailing out");

                    set_state(hpx::state::running);
                    finalize(-1.0);

                    return {threads::thread_schedule_state::terminated,
                        threads::invalid_thread_id};
                }
            }

            if (call_startup)
            {
                call_startup_functions(true);
                HPX_UNUSED(lbt_
                    << "(3rd stage, local) runtime::run_helper: ran "
                       "pre-startup functions");

                call_startup_functions(false);
                HPX_UNUSED(lbt_
                    << "(4th stage, local) runtime::run_helper: ran startup "
                       "functions");
            }

            HPX_UNUSED(lbt_ << "(4th stage, local) runtime::run_helper: "
                               "bootstrap complete");
            set_state(hpx::state::running);

            // Now, execute the user supplied thread function (hpx_main)
            if (!!func)
            {
                HPX_UNUSED(lbt_
                    << "(last stage, local) runtime::run_helper: about to "
                       "invoke hpx_main");

                // Change our thread description, as we're about to call hpx_main
                threads::set_thread_description(
                    threads::get_self_id(), "hpx_main");

                // Call hpx_main
                result = func();
            }
        }
        catch (...)
        {
            // make sure exceptions thrown in hpx_main don't escape
            // unnoticed
            {
                std::lock_guard<std::mutex> l(mtx_);
                exception_ = std::current_exception();
            }
            result = -1;
            caught_exception = true;
        }

        if (caught_exception)
        {
            HPX_ASSERT(exception_);
            report_error(exception_, false);
            finalize(-1.0);    // make sure the application exits
        }

        return {threads::thread_schedule_state::terminated,
            threads::invalid_thread_id};
    }

    int runtime::start(
        hpx::function<hpx_main_function_type> const& func, bool blocking)
    {
#if defined(_WIN64) && defined(HPX_DEBUG) &&                                   \
    !defined(HPX_HAVE_FIBER_BASED_COROUTINES)
        // needs to be called to avoid problems at system startup
        // see: http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=100319
        _isatty(0);
#endif

        // initialize instrumentation system
        hpx::tracing::tracing_init(nullptr, 0, nullptr);

        LRT_(info).format("cmd_line: {}", get_config().get_cmd_line());

        HPX_UNUSED(
            lbt_ << "(1st stage) runtime::start: booting locality " << here());

        // Register this thread with the runtime system to allow calling
        // certain HPX functionality from the main thread. Also calls
        // registered startup callbacks.
        init_tss_helper("main-thread",
            runtime_local::os_thread_type::main_thread, 0, 0, "", "", false);

#ifdef HPX_HAVE_IO_POOL
        // start the io pool
        io_pool_->run(false);
        HPX_UNUSED(
            lbt_ << "(1st stage) runtime::start: started the application "
                    "I/O service pool");
#endif
        // start the thread manager
        if (!thread_manager_->run())
        {
            std::cerr << "runtime::start: failed to start threadmanager\n";
            return -1;
        }

        HPX_UNUSED(lbt_ << "(1st stage) runtime::start: started threadmanager");

        // {{{ launch main
        // register the given main function with the thread manager
        HPX_UNUSED(lbt_ << "(1st stage) runtime::start: launching run_helper "
                           "HPX thread");

        threads::thread_function_type thread_func =
            threads::make_thread_function(hpx::bind(&runtime::run_helper, this,
                func, std::ref(result_), true, &detail::handle_print_bind));

        threads::thread_init_data data(HPX_MOVE(thread_func), "run_helper",
            threads::thread_priority::normal, threads::thread_schedule_hint(0),
            threads::thread_stacksize::large);

        this->runtime::starting();
        threads::thread_id_ref_type id = threads::invalid_thread_id;
        thread_manager_->register_thread(data, id);
        // }}}

        // block if required
        if (blocking)
        {
            return wait();    // wait for the shutdown_action to be executed
        }

        // wait for at least hpx::state::running
        util::yield_while(
            [this]() { return get_state() < hpx::state::running; },
            "runtime::start");

        return 0;    // return zero as we don't know the outcome of hpx_main yet
    }

    int runtime::start(bool blocking)
    {
        hpx::function<hpx_main_function_type> const empty_main;
        return start(empty_main, blocking);
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime::notify_finalize()
    {
        std::unique_lock<std::mutex> l(mtx_);
        if (!stop_called_)
        {
            stop_called_ = true;
            stop_done_ = true;
            wait_condition_.notify_all();
        }
    }

    void runtime::wait_finalize()
    {
        std::unique_lock<std::mutex> l(mtx_);
        while (!stop_done_)
        {
            LRT_(info).format("runtime: about to enter wait state");
            wait_condition_.wait(l);    //-V1089
            LRT_(info).format("runtime: exiting wait state");
        }
    }

    void runtime::wait_helper(
        std::mutex& mtx, std::condition_variable& cond, bool& running)
    {
        // signal successful initialization
        {
            std::lock_guard<std::mutex> lk(mtx);
            running = true;
            cond.notify_all();
        }

        std::string const thread_name("main-thread#wait_helper");
        // set thread name as shown in Visual Studio
        util::set_thread_name(thread_name.c_str());

        wait_finalize();

        // stop main thread pool
        main_pool_->stop();
    }

    int runtime::wait()
    {
        LRT_(info).format("runtime_local: about to enter wait state");

        // start the wait_helper in a separate thread
        std::mutex mtx;
        std::condition_variable cond;
        bool running = false;

        std::thread t(hpx::bind(&runtime::wait_helper, this, std::ref(mtx),
            std::ref(cond), std::ref(running)));

        // wait for the thread to run
        {
            std::unique_lock<std::mutex> lk(mtx);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (!running)      //-V776 //-V1044
                cond.wait(lk);    //-V1089
        }

        // use main thread to drive main thread pool
        main_pool_->thread_run(0);

        // block main thread
        t.join();

        thread_manager_->wait();

        LRT_(info).format("runtime_local: exiting wait state");
        return result_;
    }

    ///////////////////////////////////////////////////////////////////////////
    // First half of termination process: stop thread manager,
    // schedule a task managed by timer_pool to initiate second part
    void runtime::stop(bool blocking)
    {
        LRT_(warning).format("runtime_local: about to stop services");

        // execute all on_exit functions whenever the first thread calls this
        this->runtime::stopping();

        // stop runtime_local services (threads)
        thread_manager_->stop(false);    // just initiate shutdown

        hpx::tracing::tracing_finalize();

        if (threads::get_self_ptr())
        {
            // schedule task on separate thread to execute stop_helper() below
            // this is necessary as this function (stop()) might have been called
            // from a HPX thread, so it would deadlock by waiting for the thread
            // manager
            std::mutex mtx;
            std::condition_variable cond;
            std::unique_lock<std::mutex> l(mtx);

            std::thread t(hpx::bind(&runtime::stop_helper, this, blocking,
                std::ref(cond), std::ref(mtx)));
            cond.wait(l);    //-V1089

            t.join();
        }
        else
        {
            thread_manager_->stop(blocking);    // wait for thread manager

            deinit_global_data();

            // this disables all logging from the main thread
            deinit_tss_helper("main-thread", 0);

            LRT_(info).format("runtime_local: stopped all services");
        }

#ifdef HPX_HAVE_TIMER_POOL
        LTM_(info).format("stop: stopping timer pool");
        timer_pool_->stop();
        if (blocking)
        {
            timer_pool_->join();
            timer_pool_->clear();
        }
#endif
#ifdef HPX_HAVE_IO_POOL
        LTM_(info).format("stop: stopping io pool");
        io_pool_->stop();
        if (blocking)
        {
            io_pool_->join();
            io_pool_->clear();
        }
#endif
    }

    // Second step in termination: shut down all services. This gets executed as
    // a task in the timer_pool io_service and not as a HPX thread!
    void runtime::stop_helper(
        bool blocking, std::condition_variable& cond, std::mutex& mtx) const
    {
        // wait for thread manager to exit
        thread_manager_->stop(blocking);    // wait for thread manager

        deinit_global_data();

        // this disables all logging from the main thread
        deinit_tss_helper("main-thread", 0);

        LRT_(info).format("runtime_local: stopped all services");

        std::lock_guard<std::mutex> l(mtx);
        cond.notify_all();    // we're done now
    }

    int runtime::suspend()
    {
        LRT_(info).format("runtime_local: about to suspend runtime");

        if (state_.load() == hpx::state::sleeping)
        {
            return 0;
        }

        if (state_.load() != hpx::state::running)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status, "runtime::suspend",
                "Can only suspend runtime from running state");
        }

        thread_manager_->suspend();

#ifdef HPX_HAVE_TIMER_POOL
        timer_pool_->wait();
#endif
#ifdef HPX_HAVE_IO_POOL
        io_pool_->wait();
#endif

        set_state(hpx::state::sleeping);

        return 0;
    }

    int runtime::resume()
    {
        LRT_(info).format("runtime_local: about to resume runtime");

        if (state_.load() == hpx::state::running)
        {
            return 0;
        }

        if (state_.load() != hpx::state::sleeping)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status, "runtime::resume",
                "Can only resume runtime from suspended state");
        }

        thread_manager_->resume();

        set_state(hpx::state::running);

        return 0;
    }

    int runtime::finalize(double /*shutdown_timeout*/)
    {
        notify_finalize();
        return 0;
    }

    bool runtime::is_networking_enabled()
    {
        return false;
    }

    hpx::threads::threadmanager& runtime::get_thread_manager()
    {
        return *thread_manager_;
    }

    std::string runtime::here() const
    {
        return "127.0.0.1";
    }

    ///////////////////////////////////////////////////////////////////////////
    bool runtime::report_error(std::size_t num_thread,
        std::exception_ptr const& e, bool /*terminate_all*/)
    {
        // call thread-specific user-supplied on_error handler
        bool report_exception = true;
        if (on_error_func_)
        {
            report_exception = on_error_func_(num_thread, e);
        }

        // Early and late exceptions, errors outside HPX-threads
        if (!threads::get_self_ptr() ||
            !threads::threadmanager_is(hpx::state::running))
        {
            // report the error to the local console
            if (report_exception)
            {
                detail::report_exception_and_continue(e);
            }

            // store the exception to be able to rethrow it later
            {
                std::lock_guard<std::mutex> l(mtx_);
                exception_ = e;
            }

            notify_finalize();
            stop(false);

            return report_exception;
        }

        return report_exception;
    }

    bool runtime::report_error(std::exception_ptr const& e, bool terminate_all)
    {
        return report_error(hpx::get_worker_thread_num(), e, terminate_all);
    }

    void runtime::rethrow_exception()
    {
        if (state_.load() > hpx::state::running)
        {
            std::lock_guard<std::mutex> l(mtx_);
            if (exception_)
            {
                std::exception_ptr const e = exception_;
                exception_ = std::exception_ptr();
                std::rethrow_exception(e);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    int runtime::run(hpx::function<hpx_main_function_type> const& func)
    {
        // start the main thread function
        start(func);

        // now wait for everything to finish
        wait();
        stop();

        rethrow_exception();
        return result_;
    }

    ///////////////////////////////////////////////////////////////////////////
    int runtime::run()
    {
        // start the main thread function
        start();

        // now wait for everything to finish
        int const result = wait();
        stop();

        rethrow_exception();
        return result;
    }

    util::thread_mapper& runtime::get_thread_mapper() const
    {
        return *thread_support_;
    }

    ///////////////////////////////////////////////////////////////////////////
    threads::policies::callback_notifier runtime::get_notification_policy(
        char const* prefix, runtime_local::os_thread_type type)
    {
        using report_error_t =
            bool (runtime::*)(std::size_t, std::exception_ptr const&, bool);

        using placeholders::_1;
        using placeholders::_2;
        using placeholders::_3;
        using placeholders::_4;

        notification_policy_type notifier;

        notifier.add_on_start_thread_callback(
            hpx::bind(&runtime::init_tss_helper, this, prefix, type, _1, _2, _3,
                _4, false));
        notifier.add_on_stop_thread_callback(
            hpx::bind(&runtime::deinit_tss_helper, this, prefix, _1));
        notifier.set_on_error_callback(
            hpx::bind(static_cast<report_error_t>(&runtime::report_error), this,
                _1, _2, true));

        return notifier;
    }

    void runtime::init_tss_helper(char const* context,
        runtime_local::os_thread_type type, std::size_t local_thread_num,
        std::size_t global_thread_num, char const* pool_name,
        char const* postfix, bool service_thread) const
    {
        error_code ec(throwmode::lightweight);
        return init_tss_ex(context, type, local_thread_num, global_thread_num,
            pool_name, postfix, service_thread, ec);
    }

    void runtime::init_tss_ex(char const* context,
        runtime_local::os_thread_type type, std::size_t local_thread_num,
        std::size_t global_thread_num, char const* pool_name,
        char const* postfix, bool service_thread, error_code& ec) const
    {
        // set the thread's name, if it's not already set
        HPX_ASSERT(detail::thread_name().empty());

        std::string fullname;
        fullname += context;
        if (postfix && *postfix)
            fullname += postfix;

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
        fullname += "#" + std::to_string(global_thread_num);
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

        detail::thread_name() = HPX_MOVE(fullname);

        char const* name = detail::thread_name().c_str();

        // initialize thread mapping for external libraries (i.e. PAPI)
        thread_support_->register_thread(name, type);

        // set thread name as shown in Visual Studio
        util::set_thread_name(name);

        if (std::strstr(name, "worker") != nullptr)
            hpx::tracing::register_thread(name);

        // call thread-specific user-supplied on_start handler
        if (on_start_func_)
        {
            on_start_func_(
                local_thread_num, global_thread_num, pool_name, context);
        }

        // if this is a service thread, set its service affinity
        if (service_thread)
        {
            // FIXME: We don't set the affinity of the service threads on BG/Q,
            // as this is causing a hang (needs to be investigated)
#if !defined(__bgq__)
            threads::mask_cref_type used_processing_units =
                thread_manager_->get_used_processing_units();

            // --hpx:bind=none  should disable all affinity definitions
            if (threads::any(used_processing_units))
            {
                this->topology_.set_thread_affinity_mask(
                    this->topology_.get_service_affinity_mask(
                        used_processing_units),
                    ec);

                // comment this out for now as on CircleCI this is causing
                // unending grief
                //if (ec)
                //{
                //    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                //        "runtime::init_tss_ex",
                //        "failed to set thread affinity mask ({}) for service "
                //        "thread: {}",
                //        hpx::threads::to_string(used_processing_units),
                //        detail::thread_name());
                //}
            }
#endif
        }
    }

    void runtime::deinit_tss_helper(
        char const* context, std::size_t global_thread_num) const
    {
        threads::reset_continuation_recursion_count();

        // call thread-specific user-supplied on_stop handler
        if (on_stop_func_)
        {
            on_stop_func_(global_thread_num, global_thread_num, "", context);
        }

        // reset PAPI support
        thread_support_->unregister_thread();

        // reset thread local storage
        detail::thread_name().clear();
    }

    void runtime::add_pre_startup_function(startup_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<std::mutex> l(mtx_);
            pre_startup_functions_.push_back(HPX_MOVE(f));
        }
    }

    void runtime::add_startup_function(startup_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<std::mutex> l(mtx_);
            startup_functions_.push_back(HPX_MOVE(f));
        }
    }

    void runtime::add_pre_shutdown_function(shutdown_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<std::mutex> l(mtx_);
            pre_shutdown_functions_.push_back(HPX_MOVE(f));
        }
    }

    void runtime::add_shutdown_function(shutdown_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<std::mutex> l(mtx_);
            shutdown_functions_.push_back(HPX_MOVE(f));
        }
    }

    hpx::util::io_service_pool* runtime::get_thread_pool(char const* name)
    {
        HPX_ASSERT(name != nullptr);
#ifdef HPX_HAVE_IO_POOL
        if (0 == std::strncmp(name, "io", 2))
            return io_pool_.get();
#endif
#ifdef HPX_HAVE_TIMER_POOL
        if (0 == std::strncmp(name, "timer", 5))
            return timer_pool_.get();
#endif
        if (0 == std::strncmp(name, "main", 4))    //-V112
            return main_pool_.get();

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "runtime::get_thread_pool", "unknown thread pool requested: {}",
            name);
    }

    /// Register an external OS-thread with HPX
    bool runtime::register_thread(char const* name,
        std::size_t global_thread_num, bool service_thread, error_code& ec)
    {
        std::string thread_name(name);
        thread_name += "-thread";

        init_tss_ex(thread_name.c_str(),
            runtime_local::os_thread_type::custom_thread, global_thread_num,
            global_thread_num, "", nullptr, service_thread, ec);

        return !ec ? true : false;
    }

    /// Unregister an external OS-thread with HPX
    bool runtime::unregister_thread()
    {
        deinit_tss_helper(
            detail::thread_name().c_str(), hpx::get_worker_thread_num());
        return true;
    }

    // Access data for a given OS thread that was previously registered by
    // \a register_thread. This function must be called from a thread that was
    // previously registered with the runtime.
    runtime_local::os_thread_data runtime::get_os_thread_data(
        std::string const& label) const
    {
        return thread_support_->get_os_thread_data(label);
    }

    /// Enumerate all OS threads that have registered with the runtime.
    bool runtime::enumerate_os_threads(
        hpx::function<bool(os_thread_data const&)> const& f) const
    {
        return thread_support_->enumerate_os_threads(f);
    }

    ///////////////////////////////////////////////////////////////////////////
    threads::policies::callback_notifier get_notification_policy(
        char const* prefix)
    {
        return get_runtime().get_notification_policy(
            prefix, runtime_local::os_thread_type::worker_thread);
    }

    std::uint32_t get_locality_id(error_code& ec)
    {
        runtime const* rt = get_runtime_ptr();
        if (nullptr == rt || rt->get_state() == state::invalid)
        {
            // same as naming::invalid_locality_id
            return ~static_cast<std::uint32_t>(0);
        }

        return rt->get_locality_id(ec);
    }

    std::size_t get_num_worker_threads()
    {
        runtime const* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_num_worker_threads",
                "the runtime system has not been initialized yet");
        }

        return rt->get_num_worker_threads();
    }

    /// \brief Return the number of localities which are currently registered
    ///        for the running application.
    std::uint32_t get_num_localities(hpx::launch::sync_policy, error_code& ec)
    {
        runtime const* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_num_localities",
                "the runtime system has not been initialized yet");
        }

        return rt->get_num_localities(hpx::launch::sync, ec);
    }

    std::uint32_t get_initial_num_localities()
    {
        runtime const* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_initial_num_localities",
                "the runtime system has not been initialized yet");
        }

        return rt->get_initial_num_localities();
    }

    hpx::future<std::uint32_t> get_num_localities()
    {
        runtime const* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_num_localities",
                "the runtime system has not been initialized yet");
        }

        return rt->get_num_localities();
    }

    namespace threads {

        char const* get_stack_size_name(std::ptrdiff_t size)
        {
            auto size_enum = thread_stacksize::unknown;

            hpx::util::runtime_configuration const& rtcfg = hpx::get_config();
            if (rtcfg.get_stack_size(thread_stacksize::small_) == size)
                size_enum = thread_stacksize::small_;
            else if (rtcfg.get_stack_size(thread_stacksize::medium) == size)
                size_enum = thread_stacksize::medium;
            else if (rtcfg.get_stack_size(thread_stacksize::large) == size)
                size_enum = thread_stacksize::large;
            else if (rtcfg.get_stack_size(thread_stacksize::huge) == size)
                size_enum = thread_stacksize::huge;
            else if (rtcfg.get_stack_size(thread_stacksize::nostack) == size)
                size_enum = thread_stacksize::nostack;

            return get_stack_size_enum_name(size_enum);
        }
    }    // namespace threads

    ///////////////////////////////////////////////////////////////////////////
    // Termination detection API implementation
    namespace local {
        void termination_detection()
        {
            runtime* rt = get_runtime_ptr();
            if (rt == nullptr)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::local::termination_detection",
                    "the runtime system is not active");
            }

            hpx::threads::threadmanager& tm = rt->get_thread_manager();

            // Wait for all HPX threads to complete
            tm.wait();

            // Clean up any terminated threads
            tm.cleanup_terminated(true);
        }

        bool termination_detection(hpx::chrono::steady_duration const& timeout)
        {
            runtime* rt = get_runtime_ptr();
            if (rt == nullptr)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::local::termination_detection",
                    "the runtime system is not active");
            }

            hpx::threads::threadmanager& tm = rt->get_thread_manager();

            // Return immediately if timeout is zero or negative
            if (timeout.value().count() <= 0)
            {
                return false;
            }

            // Wait with timeout using threadmanager's wait_for
            bool completed = tm.wait_for(timeout);

            // Only cleanup if all threads completed
            if (completed)
            {
                tm.cleanup_terminated(true);
            }

            return completed;
        }

        bool termination_detection(
            hpx::chrono::steady_time_point const& deadline)
        {
            // Convert deadline to duration from now
            auto now = hpx::chrono::steady_clock::now();

            if (deadline.value() <= now)
            {
                // Deadline already passed
                return false;
            }

            auto duration = deadline.value() - now;
            return termination_detection(
                hpx::chrono::steady_duration(duration));
        }

        bool termination_detection(hpx::stop_token stop_token,
            hpx::chrono::steady_duration const& timeout)
        {
            runtime* rt = get_runtime_ptr();
            if (rt == nullptr)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::local::termination_detection",
                    "the runtime system is not active");
            }

            hpx::threads::threadmanager& tm = rt->get_thread_manager();

            // Return immediately if timeout is zero or negative
            if (timeout.value().count() <= 0)
            {
                return false;
            }

            bool completed = tm.wait_for(stop_token, timeout);

            if (completed)
            {
                tm.cleanup_terminated(true);
            }

            return completed;
        }
    }    // namespace local
}    // namespace hpx
