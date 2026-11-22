//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file local_priority_tuned_scheduler.hpp
/// \brief The local priority queue scheduler with tunable rules
///        (local-priority-tuned-fifo).

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/schedulers/local_priority_queue_scheduler.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::policies {

    /// The local priority queue scheduler with tunable rules
    /// (local-priority-tuned-fifo). It places, queues and steals threads as the
    /// local priority queue scheduler does, but takes the rules that decide
    /// whom to wake up for new threads and what an idle worker does from
    /// scheduling_policy. The numbers and switches of these rules are
    /// tunable settings (see scheduler_tuning_parameters), which the tuner
    /// selected with hpx.scheduler_tuner may change while the scheduler runs.
    /// With the default settings and tuner ("static") it decides as the local
    /// priority queue scheduler does.
    ///
    /// The settings start with the values of init_parameter: those of the
    /// local priority queue scheduler and scheduling_policy_init_parameters,
    /// which only this scheduler reads. The largest size of new threads that
    /// wake up at most one worker (hpx.scheduler_policy.wake_one_max_size) is
    /// taken once when the scheduler is created; a tuner can't change it yet.
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_local_priority_queue_scheduler_terminated_queue>
    class local_priority_tuned_scheduler final
      : public local_priority_queue_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing, tuned_scheduling_rules>
    {
    public:
        using base_type = local_priority_queue_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing, tuned_scheduling_rules>;

        // the parameters of the local priority queue scheduler, and the
        // values the settings of the rules start with
        struct init_parameter : base_type::init_parameter_type
        {
            init_parameter(base_type::init_parameter_type const& init,
                scheduling_policy_init_parameters const& policy_init =
                    scheduling_policy_init_parameters{}) noexcept
              : base_type::init_parameter_type(init)
              , policy_init_(policy_init)
            {
            }

            scheduling_policy_init_parameters policy_init_;
        };
        using init_parameter_type = init_parameter;

        explicit local_priority_tuned_scheduler(init_parameter_type const& init,
            bool const deferred_initialization = true)
          : base_type(init, deferred_initialization)
        {
            this->rules_.register_parameters(
                this->thread_queue_init_, init.policy_init_);
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
            this->wake_one_max_size_ = init.policy_init_.wake_one_max_size_;
#endif
            this->rules_.set_tuner(create_scheduler_tuner("static"));
        }

        static std::string_view get_scheduler_name()
        {
            return "local_priority_tuned_scheduler";
        }

        /// Replace the tuner that decides the values of the tunable settings
        /// (see hpx.scheduler_tuner). Called before the workers start.
        void set_tuner(std::unique_ptr<scheduler_tuner> tuner)
        {
            this->rules_.set_tuner(HPX_MOVE(tuner));
        }

        /// The tunable settings of the rules of this scheduler.
        [[nodiscard]] scheduler_tuning_parameters&
        get_tuning_parameters() noexcept
        {
            return this->rules_.parameters();
        }
        [[nodiscard]] scheduler_tuning_parameters const& get_tuning_parameters()
            const noexcept
        {
            return this->rules_.parameters();
        }

        /// What this scheduler counted for its tuner.
        [[nodiscard]] scheduler_tuning_counters const* get_tuning_counters()
            const noexcept override
        {
            return &this->rules_.counters();
        }

        // The tuner sees the counters every update period (see
        // scheduler_tuner::update). This runs on an ordinary HPX thread of
        // this scheduler's pool, started once the workers run and ended
        // before they stop (the pool calls these two). The thread waits on a
        // condition variable with the period as its timeout, so that a stop
        // ends the wait at once without waking up the thread from whatever
        // else it may wait for. It is a background thread of the pool, so a
        // tuner that waits for its next update doesn't keep the pool from
        // being suspended or stopped. A tuner without an update period runs
        // no thread.
        void on_pool_started() override
        {
            auto const period = this->rules_.update_period();
            if (period.count() <= 0)
                return;

            {
                std::unique_lock<hpx::spinlock> l(tuner_mtx_);
                tuner_stop_ = false;
                tuner_done_ = false;
            }

            thread_init_data data(
                [this, period](thread_restart_state) -> thread_result_type {
                    run_tuner(period);
                    return {
                        thread_schedule_state::terminated, invalid_thread_id};
                },
                threads::thread_description("scheduler_tuner_update"),
                thread_priority::normal, thread_schedule_hint(),
                thread_stacksize::small_,
                // created suspended, so that it is a background thread
                // before it runs; run_now, as only threads created pending
                // can be staged
                thread_schedule_state::suspended, true, this);

            this->create_thread(data, &tuner_thread_, hpx::throws);
            HPX_ASSERT(tuner_thread_);

            // The thread is waiting for its next update most of the time.
            // An ordinary thread that waits counts as work the pool still
            // has: suspending the pool waits until only background threads
            // are left (suspend_internal), and a blocking stop waits on
            // is_busy, which counts the same, so either would wait for the
            // tuner forever. As a background thread (marked, and counted
            // with increment_background_thread_count, as the background
            // work of the scheduling loop is) it is left out of these
            // counts; a suspended pool just doesn't run it until it is
            // resumed. While it waits, suspend doesn't keep a background
            // thread alive (keep_alive_thread_id), tuner_thread_ does, until
            // on_pool_stopping has ended it. Marked before it is made
            // pending, so it is never counted as ordinary work.
            get_thread_id_data(tuner_thread_)->set_is_background();
            this->increment_background_thread_count();
            threads::set_thread_state(
                tuner_thread_.noref(), thread_schedule_state::pending);
        }

        void on_pool_stopping() override
        {
            if (!tuner_thread_)
                return;

            {
                std::unique_lock<hpx::spinlock> l(tuner_mtx_);
                tuner_stop_ = true;
            }
            tuner_cond_.notify_all();

            // the pool may be stopped from an HPX thread or from another
            // thread, yield_while works for both
            hpx::util::yield_while(
                [this] {
                    std::unique_lock<hpx::spinlock> l(tuner_mtx_);
                    return !tuner_done_;
                },
                "local_priority_tuned_scheduler::on_pool_stopping");

            tuner_thread_ = invalid_thread_id;
        }

    private:
        void run_tuner(std::chrono::milliseconds const period)
        {
            std::unique_lock<hpx::spinlock> l(tuner_mtx_);

            // however the loop ends (also if the tuner throws), stop being a
            // background thread and let on_pool_stopping return; runs with
            // the lock held, before l is destroyed
            auto on_exit = hpx::experimental::scope_exit([this] {
                this->decrement_background_thread_count();
                tuner_done_ = true;
            });

            while (!tuner_stop_)
            {
                {
                    // the tuner runs without the lock, a stop waits for it
                    hpx::unlock_guard<std::unique_lock<hpx::spinlock>> ul(l);
                    this->rules_.update_tuner();
                }

                tuner_cond_.wait_for(l, hpx::chrono::steady_duration(period),
                    [this] { return tuner_stop_; });
            }
        }

        hpx::spinlock tuner_mtx_;
        hpx::condition_variable_any tuner_cond_;
        bool tuner_stop_ = false;
        bool tuner_done_ = true;
        thread_id_ref_type tuner_thread_;
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
