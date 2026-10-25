//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/runtime_local/os_thread_type.hpp>
#include <hpx/runtime_local/runtime_local_fwd.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>
#include <hpx/runtime_local/startup_function.hpp>
#include <hpx/runtime_local/state.hpp>
#include <hpx/runtime_local/thread_mapper.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // There is no need to protect these global from thread concurrent
        // access as they are access during early startup only.
        extern std::list<startup_function_type>& global_pre_startup_functions();
        extern std::list<startup_function_type>& global_startup_functions();
        extern std::list<shutdown_function_type>&
        global_pre_shutdown_functions();
        extern std::list<shutdown_function_type>& global_shutdown_functions();
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT runtime
    {
    public:
        /// Generate a new notification policy instance for the given thread
        /// name prefix
        using notification_policy_type = threads::policies::callback_notifier;
        virtual notification_policy_type get_notification_policy(
            char const* prefix, runtime_local::os_thread_type type);

        state get_state() const;
        void set_state(state s);

        /// The \a hpx_main_function_type is the default function type usable
        /// as the main HPX thread function.
        using hpx_main_function_type = int();

        using hpx_errorsink_function_type = void(
            std::uint32_t, std::string const&);

        /// Construct a new HPX runtime instance
        explicit runtime(
            hpx::util::runtime_configuration rtcfg, bool initialize);

    protected:
        explicit runtime(hpx::util::runtime_configuration rtcfg);

        void set_notification_policies(notification_policy_type&& notifier,
#ifdef HPX_HAVE_IO_POOL
            notification_policy_type&& io_pool_notifier,
#endif
#ifdef HPX_HAVE_TIMER_POOL
            notification_policy_type&& timer_pool_notifier,
#endif
            threads::detail::network_background_callback_type const&
                network_background_callback);

        /// Common initialization for different constructors
        void init();

    public:
        /// \brief The destructor makes sure all HPX runtime services are
        ///        properly shut down before exiting.
        virtual ~runtime();

        /// \brief Manage list of functions to call on exit
        void on_exit(hpx::function<void()> const& f);

        /// \brief Manage runtime 'stopped' state
        void starting();

        /// \brief Call all registered on_exit functions
        void stopping();

        /// This accessor returns whether the runtime instance has been stopped
        bool stopped() const;

        /// \brief access configuration information
        hpx::util::runtime_configuration& get_config();

        hpx::util::runtime_configuration const& get_config() const;

        std::size_t get_instance_number() const;

        /// \brief Return the system uptime measure on the thread executing this call
        static std::uint64_t get_system_uptime();

        /// \brief Return a reference to the internal PAPI thread manager
        util::thread_mapper& get_thread_mapper() const;

        threads::topology const& get_topology() const;

        /// \brief Run the HPX runtime system, use the given function for the
        ///        main \a thread and block waiting for all threads to
        ///        finish
        ///
        /// \param func       [in] This is the main function of an HPX
        ///                   application. It will be scheduled for execution
        ///                   by the thread manager as soon as the runtime has
        ///                   been initialized. This function is expected to
        ///                   expose an interface as defined by the typedef
        ///                   \a hpx_main_function_type. This parameter is
        ///                   optional and defaults to none main thread
        ///                   function, in which case all threads have to be
        ///                   scheduled explicitly.
        ///
        /// \note             The parameter \a func is optional. If no function
        ///                   is supplied, the runtime system will simply wait
        ///                   for the shutdown action without explicitly
        ///                   executing any main thread.
        ///
        /// \returns          This function will return the value as returned
        ///                   as the result of the invocation of the function
        ///                   object given by the parameter \p func.
        virtual int run(hpx::function<hpx_main_function_type> const& func);

        /// \brief Run the HPX runtime system, initially use the given number
        ///        of (OS) threads in the thread-manager and block waiting for
        ///        all threads to finish.
        ///
        /// \returns          This function will always return 0 (zero).
        virtual int run();

        /// Rethrow any stored exception (to be called after stop())
        virtual void rethrow_exception();

        /// \brief Start the runtime system
        ///
        /// \param func       [in] This is the main function of an HPX
        ///                   application. It will be scheduled for execution
        ///                   by the thread manager as soon as the runtime has
        ///                   been initialized. This function is expected to
        ///                   expose an interface as defined by the typedef
        ///                   \a hpx_main_function_type.
        /// \param blocking   [in] This allows to control whether this
        ///                   call blocks until the runtime system has been
        ///                   stopped. If this parameter is \a true the
        ///                   function \a runtime#start will call
        ///                   \a runtime#wait internally.
        ///
        /// \returns          If a blocking is a true, this function will
        ///                   return the value as returned as the result of the
        ///                   invocation of the function object given by the
        ///                   parameter \p func. Otherwise, it will return zero.
        virtual int start(hpx::function<hpx_main_function_type> const& func,
            bool blocking = false);

        /// \brief Start the runtime system
        ///
        /// \param blocking   [in] This allows to control whether this
        ///                   call blocks until the runtime system has been
        ///                   stopped. If this parameter is \a true the
        ///                   function \a runtime#start will call
        ///                   \a runtime#wait internally .
        ///
        /// \returns          If a blocking is a true, this function will
        ///                   return the value as returned as the result of the
        ///                   invocation of the function object given by the
        ///                   parameter \p func. Otherwise, it will return zero.
        virtual int start(bool blocking = false);

        /// \brief Wait for the shutdown action to be executed
        ///
        /// \returns          This function will return the value as returned
        ///                   as the result of the invocation of the function
        ///                   object given by the parameter \p func.
        virtual int wait();

        /// \brief Initiate termination of the runtime system
        ///
        /// \param blocking   [in] This allows to control whether this
        ///                   call blocks until the runtime system has been
        ///                   fully stopped. If this parameter is \a false then
        ///                   this call will initiate the stop action but will
        ///                   return immediately. Use a second call to stop
        ///                   with this parameter set to \a true to wait for
        ///                   all internal work to be completed.
        virtual void stop(bool blocking = true);

        /// \brief Suspend the runtime system
        virtual int suspend();

        ///    \brief Resume the runtime system
        virtual int resume();

        virtual int finalize(double /*shutdown_timeout*/);

        ///  \brief Return true if networking is enabled.
        virtual bool is_networking_enabled();

        /// \brief Allow access to the thread manager instance used by the HPX
        ///        runtime.
        virtual hpx::threads::threadmanager& get_thread_manager();

        /// \brief Returns a string of the locality endpoints (usable in debug output)
        virtual std::string here() const;

        /// \brief Report a non-recoverable error to the runtime system
        ///
        /// \param num_thread [in] The number of the operating system thread
        ///                   the error has been detected in.
        /// \param e          [in] This is an instance encapsulating an
        ///                   exception which lead to this function call.
        /// \param terminate_all [in] signal whether all localities should be
        ///                      terminated
        virtual bool report_error(std::size_t num_thread,
            std::exception_ptr const& e, bool terminate_all = true);

        /// \brief Report a non-recoverable error to the runtime system
        ///
        /// \param e          [in] This is an instance encapsulating an
        ///                   exception which lead to this function call.
        /// \param terminate_all [in] signal whether all localities should be
        ///                      terminated
        ///
        /// \note This function will retrieve the number of the current
        ///       shepherd thread and forward to the report_error function
        ///       above.
        virtual bool report_error(
            std::exception_ptr const& e, bool terminate_all = true);

        /// Add a function to be executed inside a HPX thread before hpx_main
        /// but guaranteed to be executed before any startup function registered
        /// with \a add_startup_function.
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread before hpx_main is executed. This is very useful
        ///             to set up the runtime environment of the application
        ///             (install performance counters, etc.)
        ///
        /// \note       The difference to a startup function is that all
        ///             pre-startup functions will be (system-wide) executed
        ///             before any startup function.
        virtual void add_pre_startup_function(startup_function_type f);

        /// Add a function to be executed inside a HPX thread before hpx_main
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread before hpx_main is executed. This is very useful
        ///             to set up the runtime environment of the application
        ///             (install performance counters, etc.)
        virtual void add_startup_function(startup_function_type f);

        /// Add a function to be executed inside a HPX thread during
        /// hpx::finalize, but guaranteed before any of the shutdown functions
        /// is executed.
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread while hpx::finalize is executed. This is very
        ///             useful to tear down the runtime environment of the
        ///             application (uninstall performance counters, etc.)
        ///
        /// \note       The difference to a shutdown function is that all
        ///             pre-shutdown functions will be (system-wide) executed
        ///             before any shutdown function.
        virtual void add_pre_shutdown_function(shutdown_function_type f);

        /// Add a function to be executed inside a HPX thread during hpx::finalize
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread while hpx::finalize is executed. This is very
        ///             useful to tear down the runtime environment of the
        ///             application (uninstall performance counters, etc.)
        virtual void add_shutdown_function(shutdown_function_type f);

        /// Access one of the internal thread pools (io_service instances)
        /// HPX is using to perform specific tasks. The three possible values
        /// for the argument \p name are "main_pool", "io_pool", "parcel_pool",
        /// and "timer_pool". For any other argument value the function will
        /// return zero.
        virtual hpx::util::io_service_pool* get_thread_pool(char const* name);

        /// \brief Register an external OS-thread with HPX
        ///
        /// This function should be called from any OS-thread which is external
        /// to HPX (not created by HPX), but which needs to access HPX
        /// functionality, such as setting a value on a promise or similar.
        ///
        /// \param name             [in] The name to use for thread
        ///                         registration.
        /// \param num              [in] The sequence number to use for thread
        ///                         registration. The default for this parameter
        ///                         is zero.
        /// \param service_thread   [in] The thread should be registered as a
        ///                         service thread. The default for this
        ///                         parameter is 'true'. Any service threads
        ///                         will be pinned to cores not currently used
        ///                         by any of the HPX worker threads.
        /// \param ec               [in,out] this represents the error status on
        ///                         exit, if this is pre-initialized to \a
        ///                         hpx#throws the function will throw on error
        ///                         instead.
        ///
        /// \note The function will compose a thread name from \c name and
        ///       \c num which is used to register the thread. It
        ///       is the user's responsibility to ensure that each (composed)
        ///       thread name is unique. HPX internally uses the following names
        ///       for the threads it creates, do not reuse those:
        ///
        ///         'main', 'io', 'timer', 'parcel', 'worker'
        ///
        /// \note This function should be called for each thread exactly once.
        ///       It will fail if it is called more than once.
        ///
        /// \returns This function will return whether the requested operation
        ///          succeeded or not.
        ///
        virtual bool register_thread(char const* name, std::size_t num = 0,
            bool service_thread = true, error_code& ec = throws);

        /// \brief Unregister an external OS-thread with HPX
        ///
        /// This function will unregister any external OS-thread from HPX.
        ///
        /// \note This function should be called for each thread exactly once. It
        ///       will fail if it is called more than once. It will fail as well
        ///       if the thread has not been registered before (see
        ///       \a register_thread).
        ///
        /// \returns This function will return whether the requested operation
        ///          succeeded or not.
        ///
        virtual bool unregister_thread();

        /// Access data for a given OS thread that was previously registered by
        /// \a register_thread.
        virtual runtime_local::os_thread_data get_os_thread_data(
            std::string const& label) const;

        /// Enumerate all OS threads that have registered with the runtime.
        virtual bool enumerate_os_threads(
            hpx::function<bool(runtime_local::os_thread_data const&)> const& f)
            const;

        notification_policy_type::on_startstop_type on_start_func() const;
        notification_policy_type::on_startstop_type on_stop_func() const;
        notification_policy_type::on_error_type on_error_func() const;

        notification_policy_type::on_startstop_type on_start_func(
            notification_policy_type::on_startstop_type&&);
        notification_policy_type::on_startstop_type on_stop_func(
            notification_policy_type::on_startstop_type&&);
        notification_policy_type::on_error_type on_error_func(
            notification_policy_type::on_error_type&&);

        virtual std::uint32_t get_locality_id(error_code& ec) const;

        virtual std::size_t get_num_worker_threads() const;

        virtual std::uint32_t get_num_localities(
            hpx::launch::sync_policy, error_code& ec) const;

        virtual std::uint32_t get_initial_num_localities() const;

        virtual hpx::future<std::uint32_t> get_num_localities() const;

        virtual std::string get_locality_name() const;

        virtual std::uint32_t assign_cores(std::string const&, std::uint32_t);

        virtual std::uint32_t assign_cores();

        hpx::program_options::options_description const& get_app_options() const
        {
            return app_options_;
        }
        void set_app_options(
            hpx::program_options::options_description const& app_options)
        {
            app_options_.add(app_options);
        }

    protected:
        void init_global_data();
        static void deinit_global_data();

        threads::thread_result_type run_helper(
            hpx::function<runtime::hpx_main_function_type> const& func,
            int& result, bool call_startup_functions,
            void (*handle_print_bind)(std::size_t) = nullptr);

        void wait_helper(
            std::mutex& mtx, std::condition_variable& cond, bool& running);

        // list of functions to call on exit
        using on_exit_type = std::vector<hpx::function<void()>>;
        on_exit_type on_exit_functions_;
        mutable std::mutex mtx_;

        hpx::util::runtime_configuration rtcfg_;

        long instance_number_;
        static std::atomic<int> instance_number_counter_;

        // certain components (such as PAPI) require all threads to be
        // registered with the library
        std::unique_ptr<util::thread_mapper> thread_support_;

        // topology and affinity data
        threads::topology& topology_;

        std::atomic<state> state_;

        // support tying in external functions to be called for thread events
        notification_policy_type::on_startstop_type on_start_func_;
        notification_policy_type::on_startstop_type on_stop_func_;
        notification_policy_type::on_error_type on_error_func_;

        int result_;

        std::exception_ptr exception_;

        notification_policy_type main_pool_notifier_;
        std::unique_ptr<util::io_service_pool> main_pool_;
#ifdef HPX_HAVE_IO_POOL
        notification_policy_type io_pool_notifier_;
        std::unique_ptr<util::io_service_pool> io_pool_;
#endif
#ifdef HPX_HAVE_TIMER_POOL
        notification_policy_type timer_pool_notifier_;
        std::unique_ptr<util::io_service_pool> timer_pool_;
#endif
        notification_policy_type notifier_;
        std::unique_ptr<hpx::threads::threadmanager> thread_manager_;

    private:
        /// \brief Helper function to stop the runtime.
        ///
        /// \param blocking   [in] This allows to control whether this
        ///                   call blocks until the runtime system has been
        ///                   fully stopped. If this parameter is \a false then
        ///                   this call will initiate the stop action but will
        ///                   return immediately. Use a second call to stop
        ///                   with this parameter set to \a true to wait for
        ///                   all internal work to be completed.
        /// \param cond
        /// \param mtx
        void stop_helper(bool blocking, std::condition_variable& cond,
            std::mutex& mtx) const;

        void deinit_tss_helper(char const* context, std::size_t num) const;

        void init_tss_ex(char const* context,
            runtime_local::os_thread_type type, std::size_t local_thread_num,
            std::size_t global_thread_num, char const* pool_name,
            char const* postfix, bool service_thread, error_code& ec) const;

        void init_tss_helper(char const* context,
            runtime_local::os_thread_type type, std::size_t local_thread_num,
            std::size_t global_thread_num, char const* pool_name,
            char const* postfix, bool service_thread) const;

        void notify_finalize();
        void wait_finalize();

        void call_startup_functions(bool pre_startup);

        std::list<startup_function_type> pre_startup_functions_;
        std::list<startup_function_type> startup_functions_;
        std::list<shutdown_function_type> pre_shutdown_functions_;
        std::list<shutdown_function_type> shutdown_functions_;

        bool stop_called_;
        bool stop_done_;
        std::condition_variable wait_condition_;

        hpx::program_options::options_description app_options_;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_error_handlers(
        hpx::util::runtime_configuration const& cfg);

    namespace util {

        ///////////////////////////////////////////////////////////////////////////
        // retrieve the command line arguments for the current locality
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool retrieve_commandline_arguments(
            hpx::program_options::options_description const& app_options,
            hpx::program_options::variables_map& vm);

        ///////////////////////////////////////////////////////////////////////////
        // retrieve the command line arguments for the current locality
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool retrieve_commandline_arguments(
            std::string const& app_name,
            hpx::program_options::variables_map& vm);
    }    // namespace util

    namespace threads {

        /// \brief Returns the stack size name.
        ///
        /// Get the readable string representing the given stack size constant.
        ///
        /// \param size this represents the stack size
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_stack_size_name(
            std::ptrdiff_t size);

        /// \brief Returns the default stack size.
        ///
        /// Get the default stack size in bytes.
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ptrdiff_t
        get_default_stack_size();

        /// \brief Returns the stack size corresponding to the given stack size
        ///        enumeration.
        ///
        /// Get the stack size corresponding to the given stack size enumeration.
        ///
        /// \param size this represents the stack size
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ptrdiff_t get_stack_size(
            thread_stacksize size);
    }    // namespace threads
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
