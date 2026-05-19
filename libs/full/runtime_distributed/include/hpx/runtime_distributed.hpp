//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/performance_counters/query_counters.hpp>
#include <hpx/performance_counters/registry.hpp>

#include <hpx/runtime_distributed/applier.hpp>
#include <hpx/runtime_distributed/find_localities.hpp>
#include <hpx/runtime_distributed/runtime_fwd.hpp>
#include <hpx/runtime_distributed/server/runtime_support.hpp>

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx {

    /// The \a runtime class encapsulates the HPX runtime system in a simple to
    /// use way. It makes sure all required parts of the HPX runtime system are
    /// properly initialized.
    class HPX_EXPORT runtime_distributed : public runtime
    {
    public:
        /// Construct a new HPX runtime instance
        ///
        /// \param rtcfg        Runtime configuration for this instance
        /// \param pre_main     Function to be called before running the
        ///                     main action of this instance
        /// \param post_main    Function to be called after running the
        ///                     main action of this instance
        explicit runtime_distributed(util::runtime_configuration& rtcfg,
            int (*pre_main)(runtime_mode) = nullptr,
            void (*post_main)() = nullptr);

        /// \brief The destructor makes sure all HPX runtime services are
        ///        properly shut down before exiting.
        ~runtime_distributed();

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
        ///                   parameter \p func. Otherwise it will return zero.
        int start(hpx::function<hpx_main_function_type> const& func,
            bool blocking = false) override;

        ///  \brief Start the runtime system
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
        ///                   parameter \p func. Otherwise it will return zero.
        int start(bool blocking = false) override;

        /// \brief Wait for the shutdown action to be executed
        ///
        /// \returns          This function will return the value as returned
        ///                   as the result of the invocation of the function
        ///                   object given by the parameter \p func.
        int wait() override;

        /// \brief Initiate termination of the runtime system
        ///
        /// \param blocking   [in] This allows to control whether this
        ///                   call blocks until the runtime system has been
        ///                   fully stopped. If this parameter is \a false then
        ///                   this call will initiate the stop action but will
        ///                   return immediately. Use a second call to stop
        ///                   with this parameter set to \a true to wait for
        ///                   all internal work to be completed.
        void stop(bool blocking = true) override;

        int finalize(double shutdown_timeout) override;

        /// \brief Stop the runtime system, wait for termination
        ///
        /// \param blocking   [in] This allows to control whether this
        ///                   call blocks until the runtime system has been
        ///                   fully stopped. If this parameter is \a false then
        ///                   this call will initiate the stop action but will
        ///                   return immediately. Use a second call to stop
        ///                   with this parameter set to \a true to wait for
        ///                   all internal work to be completed.
        /// \param cond       Condition used to update all thread when done
        /// \param mtx        Mutex used by this function to sync all threads

        void stop_helper(
            bool blocking, std::condition_variable& cond, std::mutex& mtx);

        /// \brief Suspend the runtime system
        ///
        int suspend() override;

        /// \brief Resume the runtime system
        ///
        int resume() override;

        /// \brief Report a non-recoverable error to the runtime system
        ///
        /// \param num_thread [in] The number of the operating system thread
        ///                   the error has been detected in.
        /// \param e          [in] This is an instance encapsulating an
        ///                   exception which lead to this function call.
        /// \param terminate_all [in] Kill all localities attached to the
        ///                   currently running application (default: true)
        bool report_error(std::size_t num_thread, std::exception_ptr const& e,
            bool terminate_all = true) override;

        /// \brief Report a non-recoverable error to the runtime system
        ///
        /// \param e          [in] This is an instance encapsulating an
        ///                   exception which lead to this function call.
        /// \param terminate_all [in] Kill all localities attached to the
        ///                   currently running application (default: true)
        ///
        /// \note This function will retrieve the number of the current
        ///       shepherd thread and forward to the report_error function
        ///       above.
        bool report_error(
            std::exception_ptr const& e, bool terminate_all = true) override;

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
        int run(hpx::function<hpx_main_function_type> const& func) override;

        /// \brief Run the HPX runtime system, initially use the given number
        ///        of (OS) threads in the thread-manager and block waiting for
        ///        all threads to finish.
        ///
        /// \returns          This function will always return 0 (zero).
        int run() override;

        bool is_networking_enabled() override;

        ///////////////////////////////////////////////////////////////////////
        template <typename F>
        components::server::console_error_dispatcher::sink_type set_error_sink(
            F&& sink)
        {
            return components::server::get_error_dispatcher().set_error_sink(
                HPX_FORWARD(F, sink));
        }

        /// \brief Allow access to the registry counter registry instance used
        ///        by the HPX runtime.
        performance_counters::registry& get_counter_registry();

        /// \brief Allow access to the registry counter registry instance used
        ///        by the HPX runtime.
        performance_counters::registry const& get_counter_registry() const;

        /// \brief Install all performance counters related to this runtime
        ///        instance
        static void register_counter_types();

        ///////////////////////////////////////////////////////////////////////
        // management API for active performance counters
        void register_query_counters(
            std::shared_ptr<util::query_counters> const& active_counters);

        void start_active_counters(error_code& ec = throws) const;
        void stop_active_counters(error_code& ec = throws) const;
        void reset_active_counters(error_code& ec = throws) const;
        void reinit_active_counters(
            bool reset = true, error_code& ec = throws) const;
        void evaluate_active_counters(bool reset = false,
            char const* description = nullptr, error_code& ec = throws) const;

        // stop periodic evaluation of counters during shutdown
        void stop_evaluating_counters(bool terminate = false) const;

        ///////////////////////////////////////////////////////////////////////
        /// \brief Allow access to the AGAS client instance used by the HPX
        ///        runtime.
        agas::addressing_service& get_agas_client();

#if defined(HPX_HAVE_NETWORKING)
        /// \brief Allow access to the parcel handler instance used by the HPX
        ///        runtime.
        parcelset::parcelhandler const& get_parcel_handler() const;

        parcelset::parcelhandler& get_parcel_handler();
#endif

        /// \brief Allow access to the thread manager instance used by the HPX
        ///        runtime.
        hpx::threads::threadmanager& get_thread_manager() override;

        /// \brief Allow access to the applier instance used by the HPX
        ///        runtime.
        applier::applier& get_applier();

#if defined(HPX_HAVE_NETWORKING)
        /// \brief Allow access to the locality endpoints this runtime instance is
        /// associated with.
        ///
        /// This accessor returns a reference to the locality endpoints this runtime
        /// instance is associated with.
        parcelset::endpoints_type const& endpoints() const;
#endif

        /// \brief Returns a string of the locality endpoints (usable in debug output)
        std::string here() const override;

        naming::address_type get_runtime_support_lva() const;

        naming::gid_type get_next_id(std::size_t count = 1);

        void init_id_pool_range();
        util::unique_id_ranges& get_id_pool();

#if defined(HPX_HAVE_NETWORKING)
        void register_message_handler(char const* message_handler_type,
            char const* action, error_code& ec = throws) const;
        parcelset::policies::message_handler* create_message_handler(
            char const* message_handler_type, char const* action,
            parcelset::parcelport* pp, std::size_t num_messages,
            std::size_t interval, error_code& ec = throws) const;
        serialization::binary_filter* create_binary_filter(
            char const* binary_filter_type, bool compress,
            serialization::binary_filter* next_filter,
            error_code& ec = throws) const;
#endif

        /// Initialize AGAS operation
        void initialize_agas();

        /// Add a function to be executed inside a HPX thread before hpx_main
        /// but guaranteed to be executed before any startup function registered
        /// with \a add_startup_function.
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread before hpx_main is executed. This is very useful
        ///             to setup the runtime environment of the application
        ///             (install performance counters, etc.)
        ///
        /// \note       The difference to a startup function is that all
        ///             pre-startup functions will be (system-wide) executed
        ///             before any startup function.
        void add_pre_startup_function(startup_function_type f) override;

        /// Add a function to be executed inside a HPX thread before hpx_main
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread before hpx_main is executed. This is very useful
        ///             to setup the runtime environment of the application
        ///             (install performance counters, etc.)
        void add_startup_function(startup_function_type f) override;

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
        void add_pre_shutdown_function(shutdown_function_type f) override;

        /// Add a function to be executed inside a HPX thread during hpx::finalize
        ///
        /// \param  f   The function 'f' will be called from inside a HPX
        ///             thread while hpx::finalize is executed. This is very
        ///             useful to tear down the runtime environment of the
        ///             application (uninstall performance counters, etc.)
        void add_shutdown_function(shutdown_function_type f) override;

        /// Access one of the internal thread pools (io_service instances)
        /// HPX is using to perform specific tasks. The three possible values
        /// for the argument \p name are "main_pool", "io_pool", "parcel_pool",
        /// and "timer_pool". For any other argument value the function will
        /// return zero.
        hpx::util::io_service_pool* get_thread_pool(char const* name) override;

        /// Register an external OS-thread with HPX
        bool register_thread(char const* name, std::size_t num = 0,
            bool service_thread = true, error_code& ec = throws) override;

        /// Generate a new notification policy instance for the given thread
        /// name prefix
        notification_policy_type get_notification_policy(
            char const* prefix, runtime_local::os_thread_type type) override;

        std::uint32_t get_locality_id(error_code& ec) const override;

        std::size_t get_num_worker_threads() const override;

        std::uint32_t get_num_localities(
            hpx::launch::sync_policy, error_code& ec) const override;

        std::uint32_t get_initial_num_localities() const override;

        hpx::future<std::uint32_t> get_num_localities() const override;

        std::string get_locality_name() const override;

        std::uint32_t get_num_localities(hpx::launch::sync_policy,
            components::component_type type, error_code& ec) const;

        hpx::future<std::uint32_t> get_num_localities(
            components::component_type type) const;

        std::uint32_t assign_cores(std::string const& locality_basename,
            std::uint32_t num_threads) override;

        std::uint32_t assign_cores() override;

    private:
        threads::thread_result_type run_helper(
            hpx::function<runtime::hpx_main_function_type> const& func,
            int& result);

        void init_global_data();
        void deinit_global_data();

        void wait_helper(
            std::mutex& mtx, std::condition_variable& cond, bool& running);

        void init_tss_helper(char const* context,
            runtime_local::os_thread_type type, std::size_t local_thread_num,
            std::size_t global_thread_num, char const* pool_name,
            char const* postfix, bool service_thread);

        void deinit_tss_helper(char const* context, std::size_t num) const;

        void init_tss_ex(std::string const& locality, char const* context,
            runtime_local::os_thread_type type, std::size_t local_thread_num,
            std::size_t global_thread_num, char const* pool_name,
            char const* postfix, bool service_thread, error_code& ec) const;

        static void default_errorsink(std::string const&);

        runtime_mode mode_;
        util::unique_id_ranges id_pool_;
#if defined(HPX_HAVE_NETWORKING)
        notification_policy_type parcel_handler_notifier_;
        parcelset::parcelhandler parcel_handler_;
#endif
        agas::addressing_service agas_client_;
        applier::applier applier_;

        // locality basename -> used cores
        using used_cores_map_type = std::map<std::string, std::uint32_t>;
        used_cores_map_type used_cores_map_;

        std::unique_ptr<components::server::runtime_support> runtime_support_;
        std::shared_ptr<util::query_counters> active_counters_;

        int (*pre_main_)(runtime_mode);
        void (*post_main_)();
    };
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
