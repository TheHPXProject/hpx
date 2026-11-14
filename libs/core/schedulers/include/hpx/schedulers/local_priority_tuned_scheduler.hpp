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
#include <hpx/modules/threading_base.hpp>
#include <hpx/schedulers/local_priority_queue_scheduler.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>

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
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
