//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/static_reinit.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/version.hpp>

#include <hpx/runtime_distributed.hpp>
#include <hpx/runtime_distributed/applier.hpp>
#include <hpx/runtime_distributed/big_boot_barrier.hpp>
#include <hpx/runtime_distributed/find_localities.hpp>
#include <hpx/runtime_distributed/get_num_localities.hpp>
#include <hpx/runtime_distributed/runtime_fwd.hpp>
#include <hpx/runtime_distributed/runtime_support.hpp>
#include <hpx/runtime_distributed/server/runtime_support.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN64) && defined(HPX_DEBUG) &&                                   \
    !defined(HPX_HAVE_FIBER_BASED_COROUTINES)
#include <io.h>
#endif

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {
        naming::gid_type get_next_id(std::size_t count)
        {
            if (nullptr == get_runtime_ptr())
                return naming::invalid_gid;

            return get_runtime_distributed().get_next_id(count);
        }

        ///////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
        void dijkstra_make_black()
        {
            if (auto* rtp = get_runtime_support_ptr(); rtp != nullptr)
            {
                rtp->dijkstra_make_black();
            }
        }
#endif

#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
        bool network_background_callback(
            [[maybe_unused]] runtime_distributed* rt, std::size_t num_thread,
            std::int64_t& background_work_exec_time_send,
            std::int64_t& background_work_exec_time_receive)
        {
            bool result = false;

#if defined(HPX_HAVE_NETWORKING)
            // count background work duration
            {
                threads::detail::background_work_duration_counter
                    bg_send_duration(background_work_exec_time_send);
                threads::detail::background_exec_time_wrapper bg_exec_time(
                    bg_send_duration);

                if (rt->get_parcel_handler().do_background_work(num_thread,
                        false, parcelset::parcelport_background_mode::send))
                {
                    result = true;
                }
            }

            {
                threads::detail::background_work_duration_counter
                    bg_receive_duration(background_work_exec_time_receive);
                threads::detail::background_exec_time_wrapper bg_exec_time(
                    bg_receive_duration);

                if (rt->get_parcel_handler().do_background_work(num_thread,
                        false, parcelset::parcelport_background_mode::receive))
                {
                    result = true;
                }
            }
#endif
            if (0 == num_thread)
            {
                rt->get_agas_client().garbage_collect_non_blocking();
            }

            return result;
        }
#else
        bool network_background_callback(
            [[maybe_unused]] runtime_distributed* rt, std::size_t num_thread)
        {
            bool result = false;

#if defined(HPX_HAVE_NETWORKING)
            if (rt->get_parcel_handler().do_background_work(num_thread, false,
                    parcelset::parcelport_background_mode::all))
            {
                result = true;
            }
#endif
            if (0 == num_thread)
            {
                rt->get_agas_client().garbage_collect_non_blocking();
            }

            return result;
        }
#endif
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    components::server::runtime_support* get_runtime_support_ptr()
    {
        if (auto const* rt = get_runtime_distributed_ptr(); rt != nullptr)
        {
            return static_cast<components::server::runtime_support*>(
                rt->get_runtime_support_lva());
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////
    runtime_distributed::runtime_distributed(util::runtime_configuration& rtcfg,
        int (*pre_main)(runtime_mode), void (*post_main)())
      : runtime(rtcfg)
      , mode_(rtcfg_.mode_)
#if defined(HPX_HAVE_NETWORKING)
      , parcel_handler_(rtcfg_)
#endif
      , agas_client_(rtcfg_)
      , pre_main_(pre_main)
      , post_main_(post_main)
    {
        // set notification policies only after the object was completely
        // initialized
        runtime::set_notification_policies(
            runtime_distributed::get_notification_policy(
                "worker-thread", runtime_local::os_thread_type::worker_thread),
#ifdef HPX_HAVE_IO_POOL
            runtime_distributed::get_notification_policy(
                "io-thread", runtime_local::os_thread_type::io_thread),
#endif
#ifdef HPX_HAVE_TIMER_POOL
            runtime_distributed::get_notification_policy(
                "timer-thread", runtime_local::os_thread_type::timer_thread),
#endif
            threads::detail::network_background_callback_type(
                hpx::bind_front(&detail::network_background_callback, this)));

#if defined(HPX_HAVE_NETWORKING)
        parcel_handler_notifier_ = runtime_distributed::get_notification_policy(
            "parcel-thread", runtime_local::os_thread_type::parcel_thread);
        parcel_handler_.set_notification_policies(
            rtcfg_, thread_manager_.get(), parcel_handler_notifier_);

        applier_.init(parcel_handler_, *thread_manager_);
#else
        applier_.init(*thread_manager_);
#endif
        runtime_support_.reset(new components::server::runtime_support(rtcfg_));

        // This needs to happen first
        runtime::init();

        init_global_data();
        util::reinit_construct();

        LPROGRESS_;

        components::server::get_error_dispatcher().set_error_sink(
            &runtime_distributed::default_errorsink);

        // now, launch AGAS and register all nodes, launch all other components
        initialize_agas();

        applier_.initialize(
            reinterpret_cast<std::uint64_t>(runtime_support_.get()));
    }

    void runtime_distributed::initialize_agas()
    {
#if defined(HPX_HAVE_NETWORKING)
        std::shared_ptr<parcelset::parcelport> const pp =
            parcel_handler_.get_bootstrap_parcelport();

        agas::create_big_boot_barrier(
            pp ? pp.get() : nullptr, parcel_handler_.endpoints(), rtcfg_);

        if (agas_client_.is_bootstrap())
        {
            // store number of cores used by other processes
            std::uint32_t const cores_needed =
                runtime_distributed::assign_cores();
            std::uint32_t const first_used_core =
                runtime_distributed::assign_cores(
                    pp ? pp->get_locality_name() : "<console>", cores_needed);

            rtcfg_.set_first_used_core(first_used_core);
            HPX_ASSERT(pp ? pp->here() == pp->agas_locality(rtcfg_) : true);

            agas_client_.bootstrap(parcel_handler_.endpoints(), rtcfg_);

            init_id_pool_range();

            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if (pp)
                        pp->run(false);
                },
                [&](std::exception_ptr const& e) {
                    std::cerr << hpx::util::format(
                        "the bootstrap parcelport ({}) has failed to "
                        "initialize on locality {}:\n{},\n"
                        "bailing out\n",
                        pp->type(), hpx::get_locality_id(),
                        hpx::get_error_what(e));
                    std::terminate();
                });

            agas::get_big_boot_barrier().wait_bootstrap();
        }
        else
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if (pp)
                        pp->run(false);
                },
                [&](std::exception_ptr const& e) {
                    std::cerr << hpx::util::format(
                        "the bootstrap parcelport ({}) has failed to "
                        "initialize on locality {}:\n{},\n"
                        "bailing out\n",
                        pp->type(), hpx::get_locality_id(),
                        hpx::get_error_what(e));
                    std::terminate();
                });

            agas::get_big_boot_barrier().wait_hosted(
                pp ? pp->get_locality_name() : "<console>",
                agas_client_.get_primary_ns_lva(),
                agas_client_.get_symbol_ns_lva());
        }

        agas_client_.initialize(
            reinterpret_cast<std::uint64_t>(runtime_support_.get()));
        parcel_handler_.initialize();
#else
        if (agas_client_.is_bootstrap())
        {
            parcelset::endpoints_type endpoints;
            endpoints.insert(parcelset::endpoints_type::value_type(
                "local-loopback", parcelset::locality{}));
            agas_client_.bootstrap(endpoints, rtcfg_);
        }
        agas_client_.initialize(std::uint64_t(runtime_support_.get()));
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    runtime_distributed::~runtime_distributed()
    {
        LRT_(debug).format("~runtime_distributed(entering)");

        // reset counter registry
        get_counter_registry().clear();

        runtime_support_->delete_function_lists();

        // stop all services
#if defined(HPX_HAVE_NETWORKING)
        parcel_handler_.stop();    // stops parcel pools as well
#endif
        // unload libraries
        runtime_support_->tidy();

        LRT_(debug).format("~runtime_distributed(finished)");

        LPROGRESS_;
    }

    threads::thread_result_type runtime_distributed::run_helper(
        hpx::function<runtime::hpx_main_function_type> const& func, int& result)
    {
        bool caught_exception = false;
        try
        {
            lbt_ << "(2nd stage) runtime_distributed::run_helper: launching "
                    "pre_main";

            // Change our thread description, as we're about to call pre_main
            threads::set_thread_description(threads::get_self_id(), "pre_main");

            // Finish the bootstrap
            result = 0;
            if (pre_main_ != nullptr)
            {
                result = pre_main_(mode_);
            }

            if (result)
            {
                lbt_ << "runtime_distributed::run_helper: bootstrap "
                        "aborted, bailing out";
                return threads::thread_result_type(
                    threads::thread_schedule_state::terminated,
                    threads::invalid_thread_id);
            }

            lbt_ << "(4th stage) runtime_distributed::run_helper: bootstrap "
                    "complete";
            set_state(hpx::state::running);

#if defined(HPX_HAVE_NETWORKING)
            parcel_handler_.enable_alternative_parcelports();
#endif
            // reset all counters right before running main, if requested
            if (get_config_entry("hpx.print_counter.startup", "0") == "1")
            {
                bool reset = false;
                if (get_config_entry("hpx.print_counter.reset", "0") == "1")
                    reset = true;

                error_code ec(throwmode::lightweight);    // ignore errors
                evaluate_active_counters(reset, "startup", ec);
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

        auto result_value = runtime::run_helper(func, result, false);

        if (post_main_ != nullptr)
        {
            post_main_();
        }

        return result_value;
    }

    int runtime_distributed::start(
        hpx::function<hpx_main_function_type> const& func, bool blocking)
    {
#if defined(_WIN64) && defined(HPX_DEBUG) &&                                   \
    !defined(HPX_HAVE_FIBER_BASED_COROUTINES)
        // needs to be called to avoid problems at system startup
        // see: http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=100319
        _isatty(0);
#endif
        // {{{ early startup code - local

        // initialize instrumentation system
        hpx::tracing::tracing_init(nullptr, 0, nullptr, hpx::get_locality_id(),
            hpx::get_initial_num_localities());

        LRT_(info).format("cmd_line: {}", get_config().get_cmd_line());

        lbt_ << "(1st stage) runtime_distributed::start: booting locality "
             << here();

        // Register this thread with the runtime system to allow calling
        // certain HPX functionality from the main thread. Also calls
        // registered startup callbacks.
        init_tss_helper("main-thread",
            runtime_local::os_thread_type::main_thread, 0, 0, "", "", false);

        // start runtime_support services
        runtime_support_->run();
        lbt_ << "(1st stage) runtime_distributed::start: started "
                "runtime_support component";

#ifdef HPX_HAVE_IO_POOL
        // start the io pool
        io_pool_->run(false);
        lbt_ << "(1st stage) runtime_distributed::start: started the "
                "application I/O service pool";
#endif
        // start the thread manager
        thread_manager_->run();
        lbt_ << "(1st stage) runtime_distributed::start: started "
                "threadmanager";
        // }}}

        // invoke the AGAS v2 notifications
#if defined(HPX_HAVE_NETWORKING)
        agas::get_big_boot_barrier().trigger();
#endif

        // {{{ launch main
        // register the given main function with the thread manager
        lbt_ << "(1st stage) runtime_distributed::start: launching "
                "run_helper HPX thread";

        threads::thread_function_type thread_func =
            threads::make_thread_function(
                hpx::bind(&runtime_distributed::run_helper, this, func,
                    std::ref(result_)));

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
        else
        {
            // wait for at least hpx::state::running
            util::yield_while(
                [this]() {
                    return !exception_ && get_state() < hpx::state::running;
                },
                "runtime_impl::start");
        }

        return 0;    // return zero as we don't know the outcome of hpx_main yet
    }

    int runtime_distributed::start(bool blocking)
    {
        hpx::function<hpx_main_function_type> const empty_main;
        return start(empty_main, blocking);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string locality_prefix(util::runtime_configuration const& cfg)
    {
        std::string const localities = cfg.get_entry("hpx.localities", "1");
        std::size_t const num_localities =
            util::from_string<std::size_t>(localities, 1);
        if (num_localities > 1)
        {
            std::string locality = cfg.get_entry("hpx.locality", "");
            if (!locality.empty())
            {
                locality = "locality#" + locality;
            }
            return locality;
        }
        return "";
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime_distributed::wait_helper(
        std::mutex& mtx, std::condition_variable& cond, bool& running)
    {
        // signal successful initialization
        {
            std::lock_guard<std::mutex> lk(mtx);
            running = true;
            cond.notify_all();
        }

        // prefix thread name with locality number, if needed
        std::string const locality = locality_prefix(get_config());

        // register this thread with any possibly active tracing tool
        std::string const thread_name(locality + "main-thread#wait_helper");
        hpx::tracing::set_thread_name(thread_name.c_str());

        // set thread name as shown in Visual Studio
        util::set_thread_name(thread_name.c_str());

        // wait for termination
        runtime_support_->wait();

        // stop main thread pool
        main_pool_->stop();
    }

    int runtime_distributed::wait()
    {
        LRT_(info).format("runtime_distributed: about to enter wait state");

        // start the wait_helper in a separate thread
        std::mutex mtx;
        std::condition_variable cond;
        bool running = false;

        std::thread t(hpx::bind(&runtime_distributed::wait_helper, this,
            std::ref(mtx), std::ref(cond), std::ref(running)));

        // wait for the thread to run
        {
            std::unique_lock<std::mutex> lk(mtx);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (!running)      // -V776 // -V1044
                cond.wait(lk);    //-V1089
        }

        // use main thread to drive main thread pool
        main_pool_->thread_run(0);

        // block main thread
        t.join();

        LRT_(info).format("runtime_distributed: exiting wait state");
        return result_;
    }

    ///////////////////////////////////////////////////////////////////////////
    // First half of termination process: stop thread manager,
    // schedule a task managed by timer_pool to initiate second part
    void runtime_distributed::stop(bool blocking)
    {
        LRT_(warning).format("runtime_distributed: about to stop services");

        // flush all parcel buffers, stop buffering parcels at this point
        //parcel_handler_.do_background_work(true, parcelport_background_mode::all);

        // execute all on_exit functions whenever the first thread calls this
        this->runtime::stopping();

        // stop runtime_distributed services (threads)
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

            std::thread t(hpx::bind(&runtime_distributed::stop_helper, this,
                blocking, std::ref(cond), std::ref(mtx)));
            cond.wait(l);    //-V1089

            t.join();
        }
        else
        {
            runtime_support_->stopped();    // re-activate shutdown HPX-thread
            thread_manager_->stop(blocking);    // wait for thread manager

            deinit_global_data();

            // this disables all logging from the main thread
            deinit_tss_helper("main-thread", 0);

            LRT_(info).format("runtime_distributed: stopped all services");
        }

        // stop the rest of the system
#if defined(HPX_HAVE_NETWORKING)
        parcel_handler_.stop(blocking);
#endif
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

    int runtime_distributed::finalize(double shutdown_timeout)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        //   tell main locality to start application exit, duplicated requests
        // will be ignored
        hpx::post<components::server::runtime_support::shutdown_all_action>(
            hpx::find_root_locality(), shutdown_timeout);
#else
        HPX_ASSERT(false);
        HPX_UNUSED(shutdown_timeout);
#endif
        return 0;
    }

    // Second step in termination: shut down all services.
    // This gets executed as a task in the timer_pool io_service and not as
    // a HPX thread!
    void runtime_distributed::stop_helper(
        bool blocking, std::condition_variable& cond, std::mutex& mtx)
    {
        // wait for thread manager to exit
        runtime_support_->stopped();        // re-activate shutdown HPX-thread
        thread_manager_->stop(blocking);    // wait for thread manager

        deinit_global_data();

        // this disables all logging from the main thread
        deinit_tss_helper("main-thread", 0);

        LRT_(info).format("runtime_distributed: stopped all services");

        std::lock_guard<std::mutex> l(mtx);
        cond.notify_all();    // we're done now
    }

    int runtime_distributed::suspend()
    {
        return runtime::suspend();
    }

    int runtime_distributed::resume()
    {
        return runtime::resume();
    }

    ///////////////////////////////////////////////////////////////////////////
    bool runtime_distributed::report_error(
        std::size_t num_thread, std::exception_ptr const& e, bool terminate_all)
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

            // initiate stopping the runtime system
            runtime_support_->notify_waiting_main();
            stop(false);

            return report_exception;
        }

        // The components::console_error_sink is only applied at the console,
        // so the default error sink never gets called on the locality, meaning
        // that the user never sees errors that kill the system before the
        // error parcel gets sent out. So, before we try to send the error
        // parcel (which might cause a double fault), print local diagnostics.
        components::server::console_error_sink(e);

        // Report this error to the console.
        naming::gid_type console_id;
        if (agas_client_.get_console_locality(console_id))
        {
            if (agas_client_.get_local_locality() != console_id)
            {
                components::console_error_sink(
                    hpx::id_type(
                        console_id, hpx::id_type::management_type::unmanaged),
                    e);
            }
        }

        if (terminate_all)
        {
            components::stubs::runtime_support::terminate_all(
                naming::get_id_from_locality_id(agas::booststrap_prefix));
        }

        return report_exception;
    }

    bool runtime_distributed::report_error(
        std::exception_ptr const& e, bool terminate_all)
    {
        return report_error(hpx::get_worker_thread_num(), e, terminate_all);
    }

    ///////////////////////////////////////////////////////////////////////////
    int runtime_distributed::run(
        hpx::function<hpx_main_function_type> const& func)
    {
        // start the main thread function
        start(func);

        // now wait for everything to finish
        wait();
        stop();

#if defined(HPX_HAVE_NETWORKING)
        parcel_handler_.stop();    // stops parcelport for sure
#endif

        rethrow_exception();
        return result_;
    }

    ///////////////////////////////////////////////////////////////////////////
    int runtime_distributed::run()
    {
        // start the main thread function
        start();

        // now wait for everything to finish
        int const result = wait();
        stop();

#if defined(HPX_HAVE_NETWORKING)
        parcel_handler_.stop();    // stops parcelport for sure
#endif

        rethrow_exception();
        return result;
    }

    bool runtime_distributed::is_networking_enabled()
    {
#if defined(HPX_HAVE_NETWORKING)
        return get_config().enable_networking();
#else
        return false;
#endif
    }

    performance_counters::registry& runtime_distributed::get_counter_registry()
    {
        return performance_counters::registry::instance();
    }

    performance_counters::registry const&
    runtime_distributed::get_counter_registry() const
    {
        return performance_counters::registry::instance();
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime_distributed::register_query_counters(
        std::shared_ptr<util::query_counters> const& active_counters)
    {
        active_counters_ = active_counters;
    }

    void runtime_distributed::start_active_counters(error_code& ec) const
    {
        if (active_counters_)
            active_counters_->start_counters(ec);
    }

    void runtime_distributed::stop_active_counters(error_code& ec) const
    {
        if (active_counters_)
            active_counters_->stop_counters(ec);
    }

    void runtime_distributed::reset_active_counters(error_code& ec) const
    {
        if (active_counters_)
            active_counters_->reset_counters(ec);
    }

    void runtime_distributed::reinit_active_counters(
        bool reset, error_code& ec) const
    {
        if (active_counters_)
            active_counters_->reinit_counters(reset, ec);
    }

    void runtime_distributed::evaluate_active_counters(
        bool reset, char const* description, error_code& ec) const
    {
        if (active_counters_)
            active_counters_->evaluate_counters(reset, description, true, ec);
    }

    void runtime_distributed::stop_evaluating_counters(bool terminate) const
    {
        if (active_counters_)
            active_counters_->stop_evaluating_counters(terminate);
    }

    agas::addressing_service& runtime_distributed::get_agas_client()
    {
        return agas_client_;
    }

#if defined(HPX_HAVE_NETWORKING)
    parcelset::parcelhandler const& runtime_distributed::get_parcel_handler()
        const
    {
        return parcel_handler_;
    }

    parcelset::parcelhandler& runtime_distributed::get_parcel_handler()
    {
        return parcel_handler_;
    }
#endif

    hpx::threads::threadmanager& runtime_distributed::get_thread_manager()
    {
        return *thread_manager_;
    }

    applier::applier& runtime_distributed::get_applier()
    {
        return applier_;
    }

#if defined(HPX_HAVE_NETWORKING)
    parcelset::endpoints_type const& runtime_distributed::endpoints() const
    {
        return parcel_handler_.endpoints();
    }
#endif

    std::string runtime_distributed::here() const
    {
#if defined(HPX_HAVE_NETWORKING)
        std::ostringstream strm;
        strm << endpoints();
        return strm.str();
#else
        return "console";
#endif
    }

    naming::address_type runtime_distributed::get_runtime_support_lva() const
    {
        return runtime_support_.get();
    }

    naming::gid_type get_next_id(std::size_t count = 1);

    void runtime_distributed::init_id_pool_range()
    {
        naming::gid_type lower, upper;
        naming::get_agas_client().get_id_range(
            HPX_INITIAL_GID_RANGE, lower, upper);
        return id_pool_.set_range(lower, upper);
    }

    util::unique_id_ranges& runtime_distributed::get_id_pool()
    {
        return id_pool_;
    }

    /// \brief Register all performance counter types related to this runtime
    ///        instance
    void runtime_distributed::register_counter_types()
    {
        // clang-format off
        performance_counters::generic_counter_type_data const
            statistic_counter_types[] =
        {    // averaging counter
            {"/statistics/average",
                performance_counters::counter_type::aggregating,
                "returns the averaged value of its base counter over "
                "an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/average",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // stddev counter
            {"/statistics/stddev",
                performance_counters::counter_type::aggregating,
                "returns the standard deviation value of its base counter "
                "over an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/stddev",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // rolling_averaging counter
            {"/statistics/rolling_average",
                performance_counters::counter_type::aggregating,
                "returns the rolling average value of its base counter "
                "over an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/rolling_averaging",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // rolling stddev counter
            {"/statistics/rolling_stddev",
                performance_counters::counter_type::aggregating,
                "returns the rolling standard deviation value of its base "
                "counter over an arbitrary time line; pass required base counter "
                "as the instance name: /statistics{<base_counter_name>}/rolling_stddev",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // median counter
            {"/statistics/median",
                performance_counters::counter_type::aggregating,
                "returns the median value of its base counter over "
                "an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/median",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // max counter
            {"/statistics/max", performance_counters::counter_type::aggregating,
                "returns the maximum value of its base counter over "
                "an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/max",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // min counter
            {"/statistics/min", performance_counters::counter_type::aggregating,
                "returns the minimum value of its base counter over "
                "an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/min",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // rolling max counter
            {"/statistics/rolling_max",
                performance_counters::counter_type::aggregating,
                "returns the rolling maximum value of its base counter "
                "over an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/rolling_max",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // rolling min counter
            {"/statistics/rolling_min",
                performance_counters::counter_type::aggregating,
                "returns the rolling minimum value of its base counter "
                "over an arbitrary time line; pass required base counter as the "
                "instance name: /statistics{<base_counter_name>}/rolling_min",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::statistics_counter_creator,
                &performance_counters::default_counter_discoverer, ""},

            // uptime counters
            {
                "/runtime/uptime",
                performance_counters::counter_type::elapsed_time,
                "returns the up time of the runtime instance for the "
                "referenced locality",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::uptime_counter_creator,
                &performance_counters::locality_counter_discoverer,
                "s"    // unit of measure is seconds
            },

            // component instance counters
            {"/runtime/count/component",
                performance_counters::counter_type::raw,
                "returns the number of component instances currently alive "
                "on this locality (the component type has to be specified as "
                "the counter parameter)",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::detail::
                    component_instance_counter_creator,
                &performance_counters::locality_counter_discoverer, ""},

            // action invocation counters
            {"/runtime/count/action-invocation",
                performance_counters::counter_type::raw,
                "returns the number of (local) invocations of a specific "
                "action on this locality (the action type has to be specified as "
                "the counter parameter)",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::local_action_invocation_counter_creator,
                &performance_counters::
                    local_action_invocation_counter_discoverer,
                ""},

#if defined(HPX_HAVE_NETWORKING)
            {"/runtime/count/remote-action-invocation",
                performance_counters::counter_type::raw,
                "returns the number of (remote) invocations of a specific "
                "action on this locality (the action type has to be specified as "
                "the counter parameter)",
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &performance_counters::remote_action_invocation_counter_creator,
                &performance_counters::
                    remote_action_invocation_counter_discoverer,
                ""}
#endif
        };
        // clang-format on

        performance_counters::install_counter_types(
            statistic_counter_types, std::size(statistic_counter_types));

        performance_counters::generic_counter_type_data const
            arithmetic_counter_types[] = {
                // adding counter
                {"/arithmetics/add",
                    performance_counters::counter_type::aggregating,
                    "returns the sum of the values of the specified base "
                    "counters; pass required base counters as the parameters: "
                    "/arithmetics/"
                    "add@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::arithmetics_counter_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // minus counter
                {"/arithmetics/subtract",
                    performance_counters::counter_type::aggregating,
                    "returns the difference of the values of the specified "
                    "base counters; pass the required base counters as the "
                    "parameters: "
                    "/arithmetics/"
                    "subtract@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::arithmetics_counter_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // multiply counter
                {"/arithmetics/multiply",
                    performance_counters::counter_type::aggregating,
                    "returns the product of the values of the specified "
                    "base counters; pass the required base counters as the "
                    "parameters: "
                    "/arithmetics/"
                    "multiply@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::arithmetics_counter_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // divide counter
                {"/arithmetics/divide",
                    performance_counters::counter_type::aggregating,
                    "returns the result of division of the values of the "
                    "specified base counters; pass the required base counters "
                    "as the parameters: "
                    "/arithmetics/"
                    "divide@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::arithmetics_counter_creator,
                    &performance_counters::default_counter_discoverer, ""},

                // arithmetics mean counter
                {"/arithmetics/mean",
                    performance_counters::counter_type::aggregating,
                    "returns the average value of all values of the "
                    "specified base counters; pass the required base counters "
                    "as the parameters: "
                    "/arithmetics/"
                    "mean@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::
                        arithmetics_counter_extended_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // arithmetics variance counter
                {"/arithmetics/variance",
                    performance_counters::counter_type::aggregating,
                    "returns the standard deviation of all values of the "
                    "specified base counters; pass the required base counters "
                    "as the parameters: "
                    "/arithmetics/"
                    "variance@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::
                        arithmetics_counter_extended_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // arithmetics median counter
                {"/arithmetics/median",
                    performance_counters::counter_type::aggregating,
                    "returns the median of all values of the specified "
                    "base counters; pass the required base counters as the "
                    "parameters: "
                    "/arithmetics/"
                    "median@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::
                        arithmetics_counter_extended_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // arithmetics min counter
                {"/arithmetics/min",
                    performance_counters::counter_type::aggregating,
                    "returns the minimum value of all values of the "
                    "specified base counters; pass the required base counters "
                    "as the parameters: "
                    "/arithmetics/"
                    "min@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::
                        arithmetics_counter_extended_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // arithmetics max counter
                {"/arithmetics/max",
                    performance_counters::counter_type::aggregating,
                    "returns the maximum value of all values of the "
                    "specified base counters; pass the required base counters "
                    "as the parameters: "
                    "/arithmetics/"
                    "max@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::
                        arithmetics_counter_extended_creator,
                    &performance_counters::default_counter_discoverer, ""},
                // arithmetics count counter
                {"/arithmetics/count",
                    performance_counters::counter_type::aggregating,
                    "returns the count value of all values of the "
                    "specified base counters; pass the required base counters "
                    "as the parameters: "
                    "/arithmetics/"
                    "count@<base_counter_name1>,<base_counter_name2>",
                    performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                    &performance_counters::detail::
                        arithmetics_counter_extended_creator,
                    &performance_counters::default_counter_discoverer, ""},
            };
        performance_counters::install_counter_types(
            arithmetic_counter_types, std::size(arithmetic_counter_types));
    }

    ///////////////////////////////////////////////////////////////////////////
    void start_active_counters(error_code& ec)
    {
        if (runtime_distributed const* rtd = get_runtime_distributed_ptr();
            nullptr != rtd)
        {
            rtd->start_active_counters(ec);
        }
        else
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "start_active_counters",
                "the runtime system is not available at this time");
        }
    }

    void stop_active_counters(error_code& ec)
    {
        if (runtime_distributed const* rtd = get_runtime_distributed_ptr();
            nullptr != rtd)
        {
            rtd->stop_active_counters(ec);
        }
        else
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "stop_active_counters",
                "the runtime system is not available at this time");
        }
    }

    void reset_active_counters(error_code& ec)
    {
        if (runtime_distributed const* rtd = get_runtime_distributed_ptr();
            nullptr != rtd)
        {
            rtd->reset_active_counters(ec);
        }
        else
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "reset_active_counters",
                "the runtime system is not available at this time");
        }
    }

    void reinit_active_counters(bool reset, error_code& ec)
    {
        if (runtime_distributed const* rtd = get_runtime_distributed_ptr();
            nullptr != rtd)
        {
            rtd->reinit_active_counters(reset, ec);
        }
        else
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "reinit_active_counters",
                "the runtime system is not available at this time");
        }
    }

    void evaluate_active_counters(
        bool reset, char const* description, error_code& ec)
    {
        if (runtime_distributed const* rtd = get_runtime_distributed_ptr();
            nullptr != rtd)
        {
            rtd->evaluate_active_counters(reset, description, ec);
        }
        else
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "evaluate_active_counters",
                "the runtime system is not available at this time");
        }
    }

    // helper function to stop evaluating counters during shutdown
    void stop_evaluating_counters(bool terminate)
    {
        if (runtime_distributed const* rtd = get_runtime_distributed_ptr();
            nullptr != rtd)
        {
            rtd->stop_evaluating_counters(terminate);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    threads::policies::callback_notifier
    runtime_distributed::get_notification_policy(
        char const* prefix, runtime_local::os_thread_type type)
    {
        typedef bool (runtime_distributed::*report_error_t)(
            std::size_t, std::exception_ptr const&, bool);

        using placeholders::_1;
        using placeholders::_2;
        using placeholders::_3;
        using placeholders::_4;

        notification_policy_type notifier;

        notifier.add_on_start_thread_callback(
            hpx::bind(&runtime_distributed::init_tss_helper, this, prefix, type,
                _1, _2, _3, _4, false));
        notifier.add_on_stop_thread_callback(hpx::bind(
            &runtime_distributed::deinit_tss_helper, this, prefix, _1));
        notifier.set_on_error_callback(hpx::bind(
            static_cast<report_error_t>(&runtime_distributed::report_error),
            this, _1, _2, true));

        return notifier;
    }

    void runtime_distributed::init_tss_helper(char const* context,
        runtime_local::os_thread_type type, std::size_t local_thread_num,
        std::size_t global_thread_num, char const* pool_name,
        char const* postfix, bool service_thread)
    {
        // prefix thread name with locality number, if needed
        std::string const locality = locality_prefix(get_config());

        error_code ec(throwmode::lightweight);
        return init_tss_ex(locality, context, type, local_thread_num,
            global_thread_num, pool_name, postfix, service_thread, ec);
    }

    void runtime_distributed::init_tss_ex(std::string const& locality,
        char const* context, runtime_local::os_thread_type type,
        std::size_t local_thread_num, std::size_t global_thread_num,
        char const* pool_name, char const* postfix, bool service_thread,
        error_code& ec) const
    {
        // set the thread's name, if it's not already set
        HPX_ASSERT(detail::thread_name().empty());

        std::string fullname = std::string(locality);
        if (!locality.empty())
            fullname += "/";
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

        // register this thread with any possibly active Intel tool
        hpx::tracing::set_thread_name(name);

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

                // comment this out for now as on CircleCI this is causing unending grief
                //if (ec)
                //{
                //    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                //        "runtime_distributed::init_tss_ex",
                //        "failed to set thread affinity mask ({}) for service "
                //        "thread: {}",
                //        hpx::threads::to_string(used_processing_units),
                //        detail::thread_name());
                //}
            }
#endif
        }
    }

    void runtime_distributed::deinit_tss_helper(
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

    naming::gid_type runtime_distributed::get_next_id(std::size_t count)
    {
        return id_pool_.get_id(count);
    }

    void runtime_distributed::add_pre_startup_function(startup_function_type f)
    {
        runtime_support_->add_pre_startup_function(HPX_MOVE(f));
    }

    void runtime_distributed::add_startup_function(startup_function_type f)
    {
        runtime_support_->add_startup_function(HPX_MOVE(f));
    }

    void runtime_distributed::add_pre_shutdown_function(
        shutdown_function_type f)
    {
        runtime_support_->add_pre_shutdown_function(HPX_MOVE(f));
    }

    void runtime_distributed::add_shutdown_function(shutdown_function_type f)
    {
        runtime_support_->add_shutdown_function(HPX_MOVE(f));
    }

    hpx::util::io_service_pool* runtime_distributed::get_thread_pool(
        char const* name)
    {
        HPX_ASSERT(name != nullptr);
#ifdef HPX_HAVE_IO_POOL
        if (name && 0 == std::strncmp(name, "io", 2))
            return io_pool_.get();
#endif
#if defined(HPX_HAVE_NETWORKING)
        if (name && 0 == std::strncmp(name, "parcel", 6))
            return parcel_handler_.get_thread_pool(name);
#endif
#ifdef HPX_HAVE_TIMER_POOL
        if (name && 0 == std::strncmp(name, "timer", 5))
            return timer_pool_.get();
#endif
        if (name && 0 == std::strncmp(name, "main", 4))    //-V112
            return main_pool_.get();

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "runtime_distributed::get_thread_pool",
            "unknown thread pool requested: {}", name ? name : "<unknown>");
    }

    /// Register an external OS-thread with HPX
    bool runtime_distributed::register_thread(char const* name,
        std::size_t global_thread_num, bool service_thread, error_code& ec)
    {
        // prefix thread name with locality number, if needed
        std::string const locality = locality_prefix(get_config());

        std::string thread_name(name);
        thread_name += "-thread";

        init_tss_ex(locality, thread_name.c_str(),
            runtime_local::os_thread_type::custom_thread, global_thread_num,
            global_thread_num, "", nullptr, service_thread, ec);

        return !ec ? true : false;
    }

#if defined(HPX_HAVE_NETWORKING)
    void runtime_distributed::register_message_handler(
        char const* message_handler_type, char const* action,
        error_code& ec) const
    {
        return runtime_support_->register_message_handler(
            message_handler_type, action, ec);
    }

    parcelset::policies::message_handler*
    runtime_distributed::create_message_handler(
        char const* message_handler_type, char const* action,
        parcelset::parcelport* pp, std::size_t num_messages,
        std::size_t interval, error_code& ec) const
    {
        return runtime_support_->create_message_handler(
            message_handler_type, action, pp, num_messages, interval, ec);
    }

    serialization::binary_filter* runtime_distributed::create_binary_filter(
        char const* binary_filter_type, bool compress,
        serialization::binary_filter* next_filter, error_code& ec) const
    {
        return runtime_support_->create_binary_filter(
            binary_filter_type, compress, next_filter, ec);
    }
#endif

    std::uint32_t runtime_distributed::get_locality_id(error_code& ec) const
    {
        return agas::get_locality_id(ec);
    }

    std::size_t runtime_distributed::get_num_worker_threads() const
    {
        error_code ec(throwmode::lightweight);
        return static_cast<std::size_t>(
            agas_client_.get_num_overall_threads(ec));
    }

    std::uint32_t runtime_distributed::get_num_localities(
        hpx::launch::sync_policy, error_code& ec) const
    {
        return agas_client_.get_num_localities(ec);
    }

    std::uint32_t runtime_distributed::get_initial_num_localities() const
    {
        return get_config().get_num_localities();
    }

    hpx::future<std::uint32_t> runtime_distributed::get_num_localities() const
    {
        return agas_client_.get_num_localities_async();
    }

    std::string runtime_distributed::get_locality_name() const
    {
#if defined(HPX_HAVE_NETWORKING)
        return get_parcel_handler().get_locality_name();
#else
        return "<unknown>";
#endif
    }

    std::uint32_t runtime_distributed::get_num_localities(
        hpx::launch::sync_policy, components::component_type type,
        error_code& ec) const
    {
        return agas_client_.get_num_localities(type, ec);
    }

    hpx::future<std::uint32_t> runtime_distributed::get_num_localities(
        components::component_type type) const
    {
        return agas_client_.get_num_localities_async(type);
    }

    std::uint32_t runtime_distributed::assign_cores(
        std::string const& locality_basename, std::uint32_t cores_needed)
    {
        std::lock_guard<std::mutex> l(mtx_);

        used_cores_map_type::iterator const it =
            used_cores_map_.find(locality_basename);
        if (it == used_cores_map_.end())
        {
            used_cores_map_.emplace(locality_basename, cores_needed);
            return 0;
        }

        std::uint32_t const current = it->second;
        it->second += cores_needed;

        return current;
    }

    std::uint32_t runtime_distributed::assign_cores()
    {
        // adjust thread assignments to allow for more than one locality per
        // node
        std::size_t const first_core =
            static_cast<std::size_t>(this->get_config().get_first_used_core());
        std::size_t const cores_needed =
            hpx::resource::get_partitioner().assign_cores(first_core);

        return static_cast<std::uint32_t>(cores_needed);
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime_distributed::default_errorsink(std::string const& msg)
    {
        // log the exception information in any case
        LERR_(always).format("default_errorsink: unhandled exception: {}", msg);

        std::cerr << msg << std::endl;
    }

#if defined(HPX_HAVE_NETWORKING)
    ///////////////////////////////////////////////////////////////////////////
    // Create an instance of a binary filter plugin
    serialization::binary_filter* create_binary_filter(
        char const* binary_filter_type, bool compress,
        serialization::binary_filter* next_filter, error_code& ec)
    {
        runtime_distributed* rtd = get_runtime_distributed_ptr();
        if (nullptr != rtd)
            return rtd->create_binary_filter(
                binary_filter_type, compress, next_filter, ec);

        HPX_THROWS_IF(ec, hpx::error::invalid_status, "create_binary_filter",
            "the runtime system is not available at this time");
        return nullptr;
    }
#endif

    runtime_distributed& get_runtime_distributed()
    {
        HPX_ASSERT(get_runtime_distributed_ptr() != nullptr);
        return *get_runtime_distributed_ptr();
    }

    runtime_distributed*& get_runtime_distributed_ptr()
    {
        static runtime_distributed* runtime_distributed_ = nullptr;
        return runtime_distributed_;
    }

    void runtime_distributed::init_global_data()
    {
        runtime_distributed*& runtime_distributed_ =
            get_runtime_distributed_ptr();
        HPX_ASSERT(nullptr == threads::thread_self::get_self());
        runtime_distributed_ = this;
    }

    void runtime_distributed::deinit_global_data()
    {
        runtime_distributed*& runtime_distributed_ =
            get_runtime_distributed_ptr();
        HPX_ASSERT(runtime_distributed_);
        runtime_distributed_ = nullptr;

        runtime::deinit_global_data();
    }

    naming::gid_type const& get_locality()
    {
        return get_runtime_distributed().get_agas_client().get_local_locality();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Helpers
    hpx::id_type find_here(error_code& ec)
    {
        runtime const* rt = get_runtime_ptr();
        if (nullptr == rt)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::find_here",
                "the runtime system is not available at this time");
            return hpx::invalid_id;
        }

        static hpx::id_type here =
            naming::get_id_from_locality_id(rt->get_locality_id(ec));
        return here;
    }

    hpx::id_type find_root_locality(error_code& ec)
    {
        runtime_distributed* rt = hpx::get_runtime_distributed_ptr();
        if (nullptr == rt)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::find_root_locality",
                "the runtime system is not available at this time");
            return hpx::invalid_id;
        }

        naming::gid_type console_locality;
        if (!rt->get_agas_client().get_console_locality(console_locality))
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::find_root_locality",
                "the root locality is not available at this time");
            return hpx::invalid_id;
        }

        if (&ec != &throws)
            ec = make_success_code();

        return {console_locality, hpx::id_type::management_type::unmanaged};
    }

    std::vector<hpx::id_type> find_all_localities(
        components::component_type type, error_code& ec)
    {
        std::vector<hpx::id_type> locality_ids;
        if (nullptr == hpx::applier::get_applier_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::find_all_localities",
                "the runtime system is not available at this time");
            return locality_ids;
        }

        hpx::applier::get_applier().get_localities(locality_ids, type, ec);
        return locality_ids;
    }

    std::vector<hpx::id_type> find_all_localities(error_code& ec)
    {
        std::vector<hpx::id_type> locality_ids;
        if (nullptr == hpx::applier::get_applier_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::find_all_localities",
                "the runtime system is not available at this time");
            return locality_ids;
        }

        hpx::applier::get_applier().get_localities(locality_ids, ec);
        return locality_ids;
    }

    std::vector<hpx::id_type> find_remote_localities(
        components::component_type type, error_code& ec)
    {
        std::vector<hpx::id_type> locality_ids;
        if (nullptr == hpx::applier::get_applier_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::find_remote_localities",
                "the runtime system is not available at this time");
            return locality_ids;
        }

        hpx::applier::get_applier().get_remote_localities(
            locality_ids, type, ec);
        return locality_ids;
    }

    std::vector<hpx::id_type> find_remote_localities(error_code& ec)
    {
        std::vector<hpx::id_type> locality_ids;
        if (nullptr == hpx::applier::get_applier_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::find_remote_localities",
                "the runtime system is not available at this time");
            return locality_ids;
        }

        hpx::applier::get_applier().get_remote_localities(locality_ids,
            to_int(hpx::components::component_enum_type::invalid), ec);

        return locality_ids;
    }

    // find a locality supporting the given component
    hpx::id_type find_locality(components::component_type type, error_code& ec)
    {
        if (nullptr == hpx::applier::get_applier_ptr())
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "hpx::find_locality",
                "the runtime system is not available at this time");
            return hpx::invalid_id;
        }

        std::vector<hpx::id_type> locality_ids;
        hpx::applier::get_applier().get_localities(locality_ids, type, ec);

        if (ec || locality_ids.empty())
            return hpx::invalid_id;

        // chose first locality to host the object
        return locality_ids.front();
    }

    std::uint32_t get_num_localities(hpx::launch::sync_policy,
        components::component_type type, error_code& ec)
    {
        runtime_distributed const* rt = get_runtime_distributed_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_num_localities",
                "the runtime system has not been initialized yet");
        }

        return rt->get_num_localities(hpx::launch::sync, type, ec);
    }

    hpx::future<std::uint32_t> get_num_localities(
        components::component_type type)
    {
        runtime_distributed const* rt = get_runtime_distributed_ptr();
        if (nullptr == rt)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::get_num_localities",
                "the runtime system has not been initialized yet");
        }

        return rt->get_num_localities(type);
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::naming {

    // shortcut for get_runtime().get_agas_client()
    agas::addressing_service& get_agas_client()
    {
        return get_runtime_distributed().get_agas_client();
    }

    // shortcut for get_runtime_ptr()->get_agas_client()
    agas::addressing_service* get_agas_client_ptr()
    {
        auto* rtd = get_runtime_distributed_ptr();
        return rtd ? &rtd->get_agas_client() : nullptr;
    }
}    // namespace hpx::naming

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
namespace hpx::parcelset {

    bool do_background_work(
        std::size_t num_thread, parcelport_background_mode mode)
    {
        return get_runtime_distributed()
            .get_parcel_handler()
            .do_background_work(num_thread, false, mode);
    }

    // shortcut for get_runtime().get_parcel_handler()
    parcelhandler& get_parcel_handler()
    {
        return get_runtime_distributed().get_parcel_handler();
    }

    // shortcut for get_runtime_ptr()->get_parcel_handler()
    parcelhandler* get_parcel_handler_ptr()
    {
        auto* rtd = get_runtime_distributed_ptr();
        return rtd ? &rtd->get_parcel_handler() : nullptr;
    }
}    // namespace hpx::parcelset
#endif
