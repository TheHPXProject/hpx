//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file local_workrequesting_tuned_scheduler.hpp
/// \brief The local workrequesting scheduler with tunable rules
///        (local-workrequesting-tuned-fifo).

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)

#include <hpx/modules/threading_base.hpp>
#include <hpx/schedulers/local_workrequesting_scheduler.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>

#include <memory>
#include <mutex>
#include <string_view>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::policies {

    /// The local workrequesting scheduler with tunable rules
    /// (local-workrequesting-tuned-fifo). It places, queues, and requests
    /// tasks as the local workrequesting scheduler does, while taking the rules
    /// that control idle backoff behavior from scheduling_policy. The numbers
    /// and switches of these rules are tunable settings (see
    /// scheduler_tuning_parameters), which the tuner selected with
    /// hpx.scheduler_tuner may change while the scheduler runs.
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_local_workrequesting_scheduler_terminated_queue>
    class local_workrequesting_tuned_scheduler final
      : public local_workrequesting_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing, tuned_scheduling_rules>
    {
    public:
        using base_type = local_workrequesting_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing, tuned_scheduling_rules>;

        // the parameters of the local workrequesting scheduler, and the
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

        explicit local_workrequesting_tuned_scheduler(
            init_parameter_type const& init,
            bool const deferred_initialization = true)
          : base_type(init, deferred_initialization)
        {
            this->rules_.register_parameters(
                this->thread_queue_init_, init.policy_init_);
            this->rules_.set_tuner(create_scheduler_tuner("static"));
        }

        static constexpr std::string_view get_scheduler_name() noexcept
        {
            return "local_workrequesting_tuned_scheduler";
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

#endif
