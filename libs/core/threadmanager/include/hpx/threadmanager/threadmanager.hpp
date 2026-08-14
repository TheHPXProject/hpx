//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2007-2009 Chirag Dekate, Anshul Tandon
//  Copyright (c)      2011 Bryce Lelbach, Katelyn Kufahl
//  Copyright (c)      2017 Shoshana Jakobovits
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/threadmanager/threadmanager_fwd.hpp>
#include <hpx/util/get_entry_as.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// The \a thread-manager class is the central instance of management for
    /// all (non-depleted) threads
    HPX_CXX_CORE_EXPORT class threadmanager
    {
    private:
        // we use a simple mutex to protect the data members of the
        // thread manager for now
        using mutex_type = std::mutex;

    public:
        using notification_policy_type = threads::policies::callback_notifier;
        using pool_type = std::unique_ptr<thread_pool_base>;
        using pool_vector = std::vector<pool_type>;

        threadmanager(hpx::util::runtime_configuration& rtcfg_,
#ifdef HPX_HAVE_TIMER_POOL
            util::io_service_pool& timer_pool,
#endif
            notification_policy_type& notifier,
            detail::network_background_callback_type
                network_background_callback =
                    detail::network_background_callback_type());

        threadmanager(threadmanager const&) = delete;
        threadmanager(threadmanager&&) = delete;
        threadmanager& operator=(threadmanager const&) = delete;
        threadmanager& operator=(threadmanager&&) = delete;

        ~threadmanager();

        void init() const;
        void create_pools();

        //! FIXME move to private and add --hpx:printpools cmd line option
        void print_pools(std::ostream&) const;

        // Get functions
        thread_pool_base& default_pool() const;

        thread_pool_base& get_pool(std::string const& pool_name) const;
        thread_pool_base& get_pool(pool_id_type const& pool_id) const;
        thread_pool_base& get_pool(std::size_t thread_index) const;

        bool pool_exists(std::string const& pool_name) const;
        bool pool_exists(std::size_t pool_index) const;

        /// The function \a register_work adds a new work item to the thread
        /// manager. It doesn't immediately create a new \a thread, it just adds
        /// the task parameters (function, initial state and description) to
        /// the internal management data structures. The thread itself will be
        /// created when the number of existing threads drops below the number
        /// of threads specified by the constructors max_count parameter.
        ///
        /// \param data   [in] The value of this parameter allows to
        ///               specify a description of the thread to create. This
        ///               information is used for logging purposes mainly, but
        ///               might be useful for debugging as well. This parameter
        ///               is optional and defaults to an empty string.
        /// \param ec
        thread_id_ref_type register_work(
            thread_init_data& data, error_code& ec = throws) const;

        /// The function \a register_thread adds a new work item to the thread
        /// manager. It creates a new \a thread, adds it to the internal
        /// management data structures, and schedules the new thread, if
        /// appropriate.
        ///
        /// \param data   [in] The value of this parameter allows to
        ///               specify a description of the thread to create. This
        ///               information is used for logging purposes mainly, but
        ///               might be useful for debugging as well. This parameter
        ///               is optional and defaults to an empty string.
        /// \param id     [out] This parameter will hold the id of the created
        ///               thread. This id is guaranteed to be validly
        ///               initialized before the thread function is executed.
        /// \param ec
        void register_thread(thread_init_data& data, thread_id_ref_type& id,
            error_code& ec = throws) const;

        /// \brief  Run the thread manager's work queue. This function
        ///         instantiates the specified number of OS threads in each
        ///         pool. All OS threads are started to execute the function
        ///         \a tfunc.
        ///
        /// \returns      The function returns \a true if the thread manager
        ///               has been started successfully, otherwise it returns
        ///               \a false.
        bool run() const;

        /// \brief Forcefully stop the thread-manager
        ///
        /// \param blocking
        ///
        void stop(bool blocking = true) const;

        bool is_busy() const;
        bool is_idle() const;

        void wait() const;
        bool wait_for(hpx::chrono::steady_duration const& rel_time) const;

        template <typename StopToken>
        bool wait_for(StopToken&& stop_token,
            hpx::chrono::steady_duration const& rel_time) const
        {
            auto const shutdown_check_count =
                hpx::util::get_entry_as<std::size_t>(
                    rtcfg_, "hpx.shutdown_check_count", 10);
            return hpx::util::detail::yield_while_count_timeout(
                [this]() { return is_busy(); },
                HPX_FORWARD(StopToken, stop_token), shutdown_check_count,
                rel_time);
        }

        // \brief Suspend all thread pools.
        void suspend() const;

        // \brief Resume all thread pools.
        void resume() const;

        /// Return whether the thread manager is still running This returns the
        /// "minimal state", i.e. the state of the least advanced thread pool
        hpx::state status() const;

        /// \brief return the number of HPX-threads with the given state
        ///
        /// \note This function locks the internal OS lock in the thread manager
        std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown,
            thread_priority priority = thread_priority::default_,
            std::size_t num_thread = static_cast<std::size_t>(-1),
            bool reset = false) const;

        std::int64_t get_idle_core_count() const;

        mask_type get_idle_core_mask() const;

        std::int64_t get_background_thread_count() const;

        // Enumerate all matching threads
        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state = thread_schedule_state::unknown) const;

        // \brief Abort all threads which are in suspended state. This will set
        //        the state of all suspended threads to \a pending while
        //        supplying the wait_abort extended state flag
        void abort_all_suspended_threads() const;

        // \brief Clean up terminated threads. This deletes all threads which
        //        have been terminated but which are still held in the queue
        //        of terminated threads. Some schedulers might not do anything
        //        here.
        bool cleanup_terminated(bool delete_all) const;

        /// \brief Return the number of OS threads running in this thread-manager
        ///
        /// This function will return correct results only if the thread-manager
        /// is running.
        std::size_t get_os_thread_count() const;

        std::thread& get_os_thread_handle(std::size_t num_thread) const;

    public:
        /// API functions forwarding to notification policy

        /// This notifies the thread manager that the passed exception has been
        /// raised. The exception will be routed through the notifier and the
        /// scheduler (which will result in it being passed to the runtime
        /// object, which in turn will report it to the console, etc.).
        void report_error(
            std::size_t num_thread, std::exception_ptr const& e) const;

    public:
        /// Returns the mask identifying all processing units used by this
        /// thread manager.
        mask_type get_used_processing_units() const;

        hwloc_bitmap_ptr get_pool_numa_bitmap(
            std::string const& pool_name) const;

        void set_scheduler_mode(threads::policies::scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) const noexcept;
        void add_scheduler_mode(threads::policies::scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) const noexcept;
        void add_remove_scheduler_mode(
            threads::policies::scheduler_mode to_add_mode,
            threads::policies::scheduler_mode to_remove_mode,
            hpx::threads::mask_cref_type pu_mask) const noexcept;
        void remove_scheduler_mode(threads::policies::scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) const noexcept;

        void reset_thread_distribution() const noexcept;

        static void init_tss(std::size_t global_thread_num);
        static void deinit_tss();

    public:
        // performance counters
        std::int64_t get_queue_length(bool reset) const;
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        std::int64_t get_average_thread_wait_time(bool reset) const;
        std::int64_t get_average_task_wait_time(bool reset) const;
#endif
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
        std::int64_t get_background_work_duration(bool reset) const;
        std::int64_t get_background_overhead(bool reset) const;

        std::int64_t get_background_send_duration(bool reset) const;
        std::int64_t get_background_send_overhead(bool reset) const;

        std::int64_t get_background_receive_duration(bool reset) const;
        std::int64_t get_background_receive_overhead(bool reset) const;
#endif    //HPX_HAVE_BACKGROUND_THREAD_COUNTERS

        std::int64_t get_cumulative_duration(bool reset) const;

        std::int64_t get_thread_count_unknown(bool reset) const;
        std::int64_t get_thread_count_active(bool reset) const;
        std::int64_t get_thread_count_pending(bool reset) const;
        std::int64_t get_thread_count_suspended(bool reset) const;
        std::int64_t get_thread_count_terminated(bool reset) const;
        std::int64_t get_thread_count_staged(bool reset) const;

#ifdef HPX_HAVE_THREAD_IDLE_RATES
        std::int64_t avg_idle_rate(bool reset) const noexcept;
#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::int64_t avg_creation_idle_rate(bool reset) const noexcept;
        std::int64_t avg_cleanup_idle_rate(bool reset) const noexcept;
#endif
#endif

#ifdef HPX_HAVE_THREAD_CUMULATIVE_COUNTS
        std::int64_t get_executed_threads(bool reset) const noexcept;
        std::int64_t get_executed_thread_phases(bool reset) const noexcept;
#ifdef HPX_HAVE_THREAD_IDLE_RATES
        std::int64_t get_thread_duration(bool reset) const;
        std::int64_t get_thread_phase_duration(bool reset) const;
        std::int64_t get_thread_overhead(bool reset) const;
        std::int64_t get_thread_phase_overhead(bool reset) const;
        std::int64_t get_cumulative_thread_duration(bool reset) const;
        std::int64_t get_cumulative_thread_overhead(bool reset) const;
#endif
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(bool reset) const;
        std::int64_t get_num_pending_accesses(bool reset) const;
        std::int64_t get_num_stolen_from_pending(bool reset) const;
        std::int64_t get_num_stolen_from_staged(bool reset) const;
        std::int64_t get_num_stolen_to_pending(bool reset) const;
        std::int64_t get_num_stolen_to_staged(bool reset) const;
#endif

    private:
        policies::thread_queue_init_parameters get_init_parameters() const;
        void create_scheduler_user_defined(
            hpx::resource::scheduler_function const&,
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&);
        void create_scheduler_local(thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_local_priority_fifo(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_local_priority_fifo_double(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_local_priority_lifo(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_static(thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_static_priority(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_abp_priority_fifo(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_abp_priority_lifo(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_shared_priority(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_local_workrequesting_fifo(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_local_workrequesting_lifo(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);
        void create_scheduler_local_workrequesting_mc(
            thread_pool_init_parameters const&,
            policies::thread_queue_init_parameters const&, std::size_t);

        mutable mutex_type mtx_;    // mutex protecting the members

        hpx::util::runtime_configuration& rtcfg_;
        std::vector<pool_id_type> threads_lookup_;

#ifdef HPX_HAVE_TIMER_POOL
        util::io_service_pool& timer_pool_;    // used for timed set_state
#endif
        pool_vector pools_;

        notification_policy_type& notifier_;
        detail::network_background_callback_type network_background_callback_;
    };
}    // namespace hpx::threads

#include <hpx/config/warnings_suffix.hpp>
