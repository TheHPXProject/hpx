//  Copyright (c)      2018 Mikael Simberg
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/affinity.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/threading_base/callback_notifier.hpp>
#include <hpx/threading_base/detail/get_default_pool.hpp>
#include <hpx/threading_base/network_background_callback.hpp>
#include <hpx/threading_base/scheduler_mode.hpp>
#include <hpx/threading_base/scheduler_state.hpp>
#include <hpx/threading_base/thread_init_data.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT struct pool_id_type
    {
        pool_id_type(std::size_t index, std::string name) noexcept
          : index_(index)
          , name_(HPX_MOVE(name))
        {
        }

        std::size_t index() const noexcept
        {
            return index_;
        }

        std::string const& name() const noexcept
        {
            return name_;
        }

    private:
        std::size_t const index_;
        std::string const name_;
    };
    /// \endcond

    HPX_CXX_CORE_EXPORT struct thread_pool_init_parameters
    {
        std::string const& name_;
        std::size_t index_;
        policies::scheduler_mode mode_;
        std::size_t num_threads_;
        std::size_t thread_offset_;
        hpx::threads::policies::callback_notifier& notifier_;
        hpx::threads::policies::detail::affinity_data const& affinity_data_;
        hpx::threads::detail::network_background_callback_type const&
            network_background_callback_;
        std::size_t max_background_threads_;
        std::size_t max_idle_loop_count_;
        std::size_t max_busy_loop_count_;
        std::size_t shutdown_check_count_;

        thread_pool_init_parameters(std::string const& name, std::size_t index,
            policies::scheduler_mode mode, std::size_t num_threads,
            std::size_t thread_offset,
            hpx::threads::policies::callback_notifier& notifier,
            hpx::threads::policies::detail::affinity_data const& affinity_data,
            hpx::threads::detail::network_background_callback_type const&
                network_background_callback =
                    hpx::threads::detail::network_background_callback_type(),
            std::size_t max_background_threads = static_cast<std::size_t>(-1),
            std::size_t max_idle_loop_count = HPX_IDLE_LOOP_COUNT_MAX,
            std::size_t max_busy_loop_count = HPX_BUSY_LOOP_COUNT_MAX,
            std::size_t shutdown_check_count = 10)
          : name_(name)
          , index_(index)
          , mode_(mode)
          , num_threads_(num_threads)
          , thread_offset_(thread_offset)
          , notifier_(notifier)
          , affinity_data_(affinity_data)
          , network_background_callback_(network_background_callback)
          , max_background_threads_(max_background_threads)
          , max_idle_loop_count_(max_idle_loop_count)
          , max_busy_loop_count_(max_busy_loop_count)
          , shutdown_check_count_(shutdown_check_count)
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // note: this data structure has to be protected from races from the outside

    /// \brief The base class used to manage a pool of OS threads.
    HPX_CXX_CORE_EXPORT class thread_pool_base
    {
    public:
        /// \cond NOINTERNAL
        explicit thread_pool_base(thread_pool_init_parameters const& init);

        thread_pool_base(thread_pool_base const&) = delete;
        thread_pool_base(thread_pool_base&&) = delete;
        thread_pool_base& operator=(thread_pool_base const&) = delete;
        thread_pool_base& operator=(thread_pool_base&&) = delete;

        virtual ~thread_pool_base() = default;

        virtual void init(std::size_t num_threads, std::size_t threads_offset);

        virtual bool run(
            std::unique_lock<std::mutex>& l, std::size_t num_threads) = 0;

        virtual void stop(
            std::unique_lock<std::mutex>& l, bool blocking = true) = 0;

        virtual void wait(std::unique_lock<std::mutex>& l) = 0;
        virtual bool is_busy() = 0;
        virtual bool is_idle() = 0;

        virtual void print_pool(std::ostream&) const = 0;

        pool_id_type get_pool_id() const
        {
            return id_;
        }
        /// \endcond

        /// Suspends the given processing unit. Blocks until the processing unit
        /// has been suspended.
        ///
        /// \param virt_core   [in] The processing unit on the pool to be
        ///                    suspended. The processing units are indexed
        ///                    starting from 0.
        /// \param ec [in,out] this represents the error status on exit, if this
        ///           is pre-initialized to \a hpx#throws the function will
        ///           throw on error instead.
        virtual void suspend_processing_unit_direct(
            std::size_t virt_core, error_code& ec = throws) = 0;

        /// Resumes the given processing unit. Blocks until the processing unit
        /// has been resumed.
        ///
        /// \param virt_core   [in] The processing unit on the pool to be resumed.
        ///                    The processing units are indexed starting from 0.
        /// \param ec [in,out] this represents the error status on exit, if this
        ///           is pre-initialized to \a hpx#throws the function will
        ///           throw on error instead.
        virtual void resume_processing_unit_direct(
            std::size_t virt_core, error_code& ec = throws) = 0;

        /// Resumes the thread pool. Blocks until all OS threads on the thread pool
        /// have been resumed.
        ///
        /// \param ec [in,out] this represents the error status on exit, if this
        ///           is pre-initialized to \a hpx#throws the function will
        ///           throw on error instead.
        virtual void resume_direct(error_code& ec = throws) = 0;

        /// Suspends the thread pool. Blocks until all OS threads on the thread pool
        /// have been suspended.
        ///
        /// \note A thread pool cannot be suspended from an HPX thread running
        ///       on the pool itself.
        ///
        /// \param ec [in,out] this represents the error status on exit, if this
        ///           is pre-initialized to \a hpx#throws the function will
        ///           throw on error instead.
        ///
        /// \throws hpx::exception if called from an HPX thread which is running
        ///         on the pool itself.
        virtual void suspend_direct(error_code& ec = throws) = 0;

    public:
        /// \cond NOINTERNAL
        virtual std::size_t get_os_thread_count() const = 0;

        virtual std::thread& get_os_thread_handle(std::size_t num_thread) = 0;

        virtual std::size_t get_active_os_thread_count() const;

        virtual void create_thread(
            thread_init_data& data, thread_id_ref_type& id, error_code& ec) = 0;
        virtual thread_id_ref_type create_work(
            thread_init_data& data, error_code& ec) = 0;

        virtual thread_state set_state(thread_id_type const& id,
            thread_schedule_state new_state, thread_restart_state new_state_ex,
            thread_priority priority, error_code& ec) = 0;

        virtual thread_id_ref_type set_state(
            hpx::chrono::steady_time_point const& abs_time,
            thread_id_type const& id, thread_schedule_state newstate,
            thread_restart_state newstate_ex, thread_priority priority,
            error_code& ec) = 0;

        [[nodiscard]] std::size_t get_pool_index() const noexcept
        {
            return id_.index();
        }

        [[nodiscard]] std::string const& get_pool_name() const noexcept
        {
            return id_.name();
        }

        [[nodiscard]] std::size_t get_thread_offset() const noexcept
        {
            return thread_offset_;
        }

        virtual policies::scheduler_base* get_scheduler() const
        {
            return nullptr;
        }

        mask_type get_used_processing_units(bool full_cores = false) const;
        mask_type get_used_processing_units(
            std::size_t num_cores, bool full_cores = false) const;
        mask_type get_used_processing_unit(
            std::size_t thread_num, bool full_cores = false) const;

        hwloc_bitmap_ptr get_numa_domain_bitmap() const;

        // performance counters
#if defined(HPX_HAVE_THREAD_CUMULATIVE_COUNTS)
        virtual std::int64_t get_executed_threads(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_executed_thread_phases(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
#if defined(HPX_HAVE_THREAD_IDLE_RATES)
        virtual std::int64_t get_thread_phase_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_thread_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_thread_phase_overhead(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_thread_overhead(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_cumulative_thread_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_cumulative_thread_overhead(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
#endif
#endif
        virtual std::int64_t get_cumulative_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }

#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
        virtual std::int64_t get_background_work_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_background_overhead(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }

        virtual std::int64_t get_background_send_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_background_send_overhead(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }

        virtual std::int64_t get_background_receive_duration(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_background_receive_overhead(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS

#if defined(HPX_HAVE_THREAD_IDLE_RATES)
        virtual std::int64_t avg_idle_rate_all(bool /*reset*/) noexcept
        {
            return 0;
        }
        virtual std::int64_t avg_idle_rate(std::size_t, bool) noexcept
        {
            return 0;
        }

#if defined(HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES)
        virtual std::int64_t avg_creation_idle_rate(
            std::size_t /*thread_num*/, bool /*reset*/) noexcept
        {
            return 0;
        }
        virtual std::int64_t avg_cleanup_idle_rate(
            std::size_t /*thread_num*/, bool /*reset*/) noexcept
        {
            return 0;
        }
#endif
#endif
        virtual std::int64_t get_queue_length(std::size_t, bool)
        {
            return 0;
        }

#if defined(HPX_HAVE_THREAD_QUEUE_WAITTIME)
        virtual std::int64_t get_average_thread_wait_time(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_average_task_wait_time(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
#endif

#if defined(HPX_HAVE_THREAD_STEALING_COUNTS)
        virtual std::int64_t get_num_pending_misses(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_num_pending_accesses(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }

        virtual std::int64_t get_num_stolen_from_pending(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_num_stolen_to_pending(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_num_stolen_from_staged(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
        virtual std::int64_t get_num_stolen_to_staged(
            std::size_t /*thread_num*/, bool /*reset*/)
        {
            return 0;
        }
#endif
        virtual std::int64_t get_thread_count(thread_schedule_state /*state*/,
            thread_priority /*priority*/, std::size_t /*num_thread*/,
            bool /*reset*/)
        {
            return 0;
        }

        virtual std::int64_t get_idle_core_count() const
        {
            return 0;
        }

        virtual void get_idle_core_mask(mask_type&) const {}

        virtual std::int64_t get_background_thread_count() const
        {
            return 0;
        }

        std::int64_t get_thread_count_unknown(
            std::size_t num_thread, bool reset);
        std::int64_t get_thread_count_active(
            std::size_t num_thread, bool reset);
        std::int64_t get_thread_count_pending(
            std::size_t num_thread, bool reset);
        std::int64_t get_thread_count_suspended(
            std::size_t num_thread, bool reset);
        std::int64_t get_thread_count_terminated(
            std::size_t num_thread, bool reset);
        std::int64_t get_thread_count_staged(
            std::size_t num_thread, bool reset);

        virtual std::int64_t get_scheduler_utilization() const = 0;

        virtual std::int64_t get_idle_loop_count(
            std::size_t num, bool reset) = 0;
        virtual std::int64_t get_busy_loop_count(
            std::size_t num, bool reset) = 0;

        ///////////////////////////////////////////////////////////////////////
        virtual bool enumerate_threads(
            hpx::function<bool(thread_id_type)> const& /*f*/,
            thread_schedule_state /*state*/ =
                thread_schedule_state::unknown) const
        {
            return false;
        }

        virtual void reset_thread_distribution() noexcept {}

        virtual void abort_all_suspended_threads() {}
        virtual bool cleanup_terminated(bool /*delete_all*/)
        {
            return false;
        }

        virtual hpx::state get_state() const = 0;
        virtual hpx::state get_state(std::size_t num_thread) const = 0;

        virtual bool has_reached_state(hpx::state s) const = 0;

        virtual void do_some_work(std::size_t /*num_thread*/) {}

        virtual bool report_error(
            std::size_t global_thread_num, std::exception_ptr const& e)
        {
            return notifier_.on_error(global_thread_num, e);
        }

        [[nodiscard]] double timestamp_scale() const noexcept
        {
            return timestamp_scale_;
        }
        /// \endcond

    protected:
        /// \cond NOINTERNAL
        void init_pool_time_scale();
        /// \endcond

    protected:
        /// \cond NOINTERNAL
        pool_id_type id_;

        // The thread_offset is equal to the accumulated number of
        // threads in all pools preceding this pool
        // in the thread indexation. That means, that in order to know
        // the global index of a thread it owns, the pool has to compute:
        // global index = thread_offset_ + local index.
        std::size_t thread_offset_;

        policies::detail::affinity_data const& affinity_data_;

        // scale timestamps to nanoseconds
        double timestamp_scale_;

        // callback functions to invoke at start, stop, and error
        threads::policies::callback_notifier& notifier_;
        /// \endcond
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, thread_pool_base const& thread_pool);
}    // namespace hpx::threads

#include <hpx/config/warnings_suffix.hpp>
