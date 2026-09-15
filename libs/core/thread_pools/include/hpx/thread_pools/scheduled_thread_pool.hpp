//  Copyright (c) 2017 Shoshana Jakobovits
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/affinity.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <hpx/thread_pools/scheduling_loop.hpp>

#include <atomic>
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

namespace hpx::threads::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Scheduler>
    struct init_tss_helper;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Scheduler>
    class scheduled_thread_pool final : public hpx::threads::thread_pool_base
    {
    public:
        ///////////////////////////////////////////////////////////////////
        scheduled_thread_pool(std::unique_ptr<Scheduler> sched,
            thread_pool_init_parameters const& init);

        scheduled_thread_pool(scheduled_thread_pool const&) = delete;
        scheduled_thread_pool(scheduled_thread_pool&&) = delete;
        scheduled_thread_pool& operator=(scheduled_thread_pool const&) = delete;
        scheduled_thread_pool& operator=(scheduled_thread_pool&&) = delete;

        ~scheduled_thread_pool() override;

        void print_pool(std::ostream& os) const override;

        threads::policies::scheduler_base* get_scheduler() const override
        {
            return sched_.get();
        }

        ///////////////////////////////////////////////////////////////////
        hpx::state get_state() const override;
        hpx::state get_state(std::size_t num_thread) const override;

        bool has_reached_state(hpx::state s) const override
        {
            return sched_->Scheduler::has_reached_state(s);
        }

        ///////////////////////////////////////////////////////////////////
        void do_some_work(std::size_t num_thread) override
        {
            sched_->Scheduler::do_some_work(num_thread);
        }

        void create_thread(thread_init_data& data, thread_id_ref_type& id,
            error_code& ec) override;

        thread_id_ref_type create_work(
            thread_init_data& data, error_code& ec) override;

        thread_state set_state(thread_id_type const& id,
            thread_schedule_state new_state, thread_restart_state new_state_ex,
            thread_priority priority, error_code& ec) override;

        thread_id_ref_type set_state(
            hpx::chrono::steady_time_point const& abs_time,
            thread_id_type const& id, thread_schedule_state newstate,
            thread_restart_state newstate_ex, thread_priority priority,
            error_code& ec) override;

        bool report_error(
            std::size_t num, std::exception_ptr const& e) override;

        void abort_all_suspended_threads() override
        {
            sched_->Scheduler::abort_all_suspended_threads();
        }

        bool cleanup_terminated(bool delete_all) override
        {
            return sched_->Scheduler::cleanup_terminated(delete_all);
        }

        std::int64_t get_thread_count(thread_schedule_state state,
            thread_priority priority, std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_thread_count(
                state, priority, num, reset);
        }

        std::int64_t get_idle_core_count() const override;

        void get_idle_core_mask(mask_type&) const override;

        std::int64_t get_background_thread_count() const override
        {
            return sched_->Scheduler::get_background_thread_count();
        }

        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state) const override
        {
            return sched_->Scheduler::enumerate_threads(f, state);
        }

        void reset_thread_distribution() noexcept override
        {
            return sched_->Scheduler::reset_thread_distribution();
        }

        ///////////////////////////////////////////////////////////////////
        bool run(
            std::unique_lock<std::mutex>& l, std::size_t pool_threads) override;

        template <typename Lock>
        void stop_locked(Lock& l, bool blocking = true);
        void stop(
            std::unique_lock<std::mutex>& l, bool blocking = true) override;

        void wait(std::unique_lock<std::mutex>& l) override;
        bool is_busy() override;
        bool is_idle() override;

        void suspend_direct(error_code& ec = throws) override;
        void resume_direct(error_code& ec = throws) override;

        void suspend_processing_unit_direct(
            std::size_t virt_core, error_code& = hpx::throws) override;
        void resume_processing_unit_direct(
            std::size_t virt_core, error_code& = hpx::throws) override;

        ///////////////////////////////////////////////////////////////////
        std::thread& get_os_thread_handle(
            std::size_t global_thread_num) override
        {
            std::size_t const num_thread_local =
                global_thread_num - this->thread_offset_;
            HPX_ASSERT(num_thread_local < threads_.size());
            return threads_[num_thread_local];
        }

        void thread_func(std::size_t thread_num, std::size_t global_thread_num,
            std::shared_ptr<util::barrier> const& startup);

        std::size_t get_os_thread_count() const override
        {
            return thread_count_;
        }

        std::size_t get_active_os_thread_count() const override
        {
            std::size_t active_os_thread_count = 0;
            for (std::size_t thread_num = 0; thread_num < threads_.size();
                ++thread_num)
            {
                if (sched_->Scheduler::get_state(thread_num).load() ==
                    hpx::state::running)
                {
                    ++active_os_thread_count;
                }
            }

            return active_os_thread_count;
        }

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(
            std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_num_pending_misses(num, reset);
        }

        std::int64_t get_num_pending_accesses(
            std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_num_pending_accesses(num, reset);
        }

        std::int64_t get_num_stolen_from_pending(
            std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_num_stolen_from_pending(num, reset);
        }

        std::int64_t get_num_stolen_to_pending(
            std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_num_stolen_to_pending(num, reset);
        }

        std::int64_t get_num_stolen_from_staged(
            std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_num_stolen_from_staged(num, reset);
        }

        std::int64_t get_num_stolen_to_staged(
            std::size_t num, bool reset) override
        {
            return sched_->Scheduler::get_num_stolen_to_staged(num, reset);
        }
#endif
        std::int64_t get_queue_length(
            std::size_t num_thread, bool /* reset */) override
        {
            return sched_->Scheduler::get_queue_length(num_thread);
        }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        std::int64_t get_average_thread_wait_time(
            std::size_t num_thread, bool /* reset */) override
        {
            return sched_->Scheduler::get_average_thread_wait_time(num_thread);
        }

        std::int64_t get_average_task_wait_time(
            std::size_t num_thread, bool /* reset */) override
        {
            return sched_->Scheduler::get_average_task_wait_time(num_thread);
        }
#endif

        std::int64_t get_executed_threads() const;

#if defined(HPX_HAVE_THREAD_CUMULATIVE_COUNTS)
        std::int64_t get_executed_threads(std::size_t, bool) override;
        std::int64_t get_executed_thread_phases(std::size_t, bool) override;
#if defined(HPX_HAVE_THREAD_IDLE_RATES)
        std::int64_t get_thread_phase_duration(std::size_t, bool) override;
        std::int64_t get_thread_duration(std::size_t, bool) override;
        std::int64_t get_thread_phase_overhead(std::size_t, bool) override;
        std::int64_t get_thread_overhead(std::size_t, bool) override;
        std::int64_t get_cumulative_thread_duration(std::size_t, bool) override;
        std::int64_t get_cumulative_thread_overhead(std::size_t, bool) override;
#endif
#endif

        std::int64_t get_cumulative_duration(std::size_t, bool) override;

#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
        std::int64_t get_background_work_duration(std::size_t, bool) override;
        std::int64_t get_background_overhead(std::size_t, bool) override;

        std::int64_t get_background_send_duration(std::size_t, bool) override;
        std::int64_t get_background_send_overhead(std::size_t, bool) override;

        std::int64_t get_background_receive_duration(
            std::size_t, bool) override;
        std::int64_t get_background_receive_overhead(
            std::size_t, bool) override;
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS

#if defined(HPX_HAVE_THREAD_IDLE_RATES)
        std::int64_t avg_idle_rate_all(bool reset) noexcept override;
        std::int64_t avg_idle_rate(std::size_t, bool) noexcept override;

#if defined(HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES)
        std::int64_t avg_creation_idle_rate(
            std::size_t, bool) noexcept override;
        std::int64_t avg_cleanup_idle_rate(std::size_t, bool) noexcept override;
#endif
#endif

        std::int64_t get_idle_loop_count(std::size_t num, bool reset) override;
        std::int64_t get_busy_loop_count(std::size_t num, bool reset) override;
        std::int64_t get_scheduler_utilization() const override;

    protected:
        friend struct init_tss_helper<Scheduler>;

        void resume_internal(bool blocking, error_code& ec);
        void suspend_internal(error_code& ec);

        void remove_processing_unit_internal(
            std::size_t virt_core, error_code& = hpx::throws);
        void add_processing_unit_internal(std::size_t virt_core,
            std::size_t thread_num, std::shared_ptr<util::barrier> startup,
            error_code& ec = hpx::throws);

    private:
        std::vector<std::thread> threads_;    // vector of OS-threads

        // holds the used scheduler
        std::unique_ptr<Scheduler> sched_;

    public:
        void init_perf_counter_data(std::size_t pool_threads);

    private:
        // store data for the various thread-specific counters together to
        // reduce false sharing
        struct scheduling_counter_data
        {
            // count number of executed HPX-threads and thread phases
            // (invocations)
            std::int64_t executed_threads_;
            std::int64_t executed_thread_phases_;

#if defined(HPX_HAVE_THREAD_CUMULATIVE_COUNTS)
            // timestamps/values of last reset operation for various performance
            // counters
            std::int64_t reset_executed_threads_;
            std::int64_t reset_executed_thread_phases_;

#if defined(HPX_HAVE_THREAD_IDLE_RATES)
            std::int64_t reset_thread_duration_;
            std::int64_t reset_thread_duration_times_;

            std::int64_t reset_thread_overhead_;
            std::int64_t reset_thread_overhead_times_;
            std::int64_t reset_thread_overhead_times_total_;

            std::int64_t reset_thread_phase_duration_;
            std::int64_t reset_thread_phase_duration_times_;

            std::int64_t reset_thread_phase_overhead_;
            std::int64_t reset_thread_phase_overhead_times_;
            std::int64_t reset_thread_phase_overhead_times_total_;

            std::int64_t reset_cumulative_thread_duration_;

            std::int64_t reset_cumulative_thread_overhead_;
            std::int64_t reset_cumulative_thread_overhead_total_;
#endif
#endif

#if defined(HPX_HAVE_THREAD_IDLE_RATES)
            std::int64_t reset_idle_rate_time_;
            std::int64_t reset_idle_rate_time_total_;

#if defined(HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES)
            std::int64_t reset_creation_idle_rate_time_;
            std::int64_t reset_creation_idle_rate_time_total_;

            std::int64_t reset_cleanup_idle_rate_time_;
            std::int64_t reset_cleanup_idle_rate_time_total_;
#endif
#endif
            // tfunc_impl timers
            std::int64_t exec_times_;
            std::int64_t tfunc_times_;
            std::int64_t reset_tfunc_times_;

#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
            // overall counters for background work
            std::int64_t background_duration_;
            std::int64_t reset_background_duration_;
            std::int64_t reset_background_tfunc_times_;
            std::int64_t reset_background_overhead_;

            // counters for background work related to sending parcels
            std::int64_t background_send_duration_;
            std::int64_t reset_background_send_duration_;
            std::int64_t reset_background_send_tfunc_times_;
            std::int64_t reset_background_send_overhead_;

            // counters for background work related to receiving parcels
            std::int64_t background_receive_duration_;
            std::int64_t reset_background_receive_duration_;
            std::int64_t reset_background_receive_tfunc_times_;
            std::int64_t reset_background_receive_overhead_;
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS

            std::int64_t idle_loop_counts_;
            std::int64_t busy_loop_counts_;

            // scheduler utilization data
            bool tasks_active_;
        };

        std::vector<scheduling_counter_data> counter_data_;

        // support detail::manage_executor interface
        std::atomic<long> thread_count_;
        std::atomic<std::int64_t> tasks_scheduled_;
        network_background_callback_type network_background_callback_;

        std::size_t max_background_threads_;
        std::size_t max_idle_loop_count_;
        std::size_t max_busy_loop_count_;
        std::size_t shutdown_check_count_;
    };
}    // namespace hpx::threads::detail

#include <hpx/config/warnings_suffix.hpp>
