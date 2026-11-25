//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>
#include <hpx/threading_base/scheduling_policy.hpp>
#include <hpx/threading_base/thread_queue_init_parameters.hpp>

#include <cstdint>
#include <limits>

namespace hpx::threads::policies {

    namespace {

        constexpr bool per_worker = true;
        constexpr bool pool_wide = false;

        // the longest time in microseconds
        constexpr float max_time = (std::numeric_limits<float>::max)();

        constexpr std::uint32_t as_switch(bool const value) noexcept
        {
            return value ? 1 : 0;
        }

        constexpr std::uint32_t as_number(
            threads::task_size const size) noexcept
        {
            return static_cast<std::uint32_t>(size);
        }
    }    // namespace

    void scheduling_policy::register_idle_parameters(
        scheduler_tuning_parameters& parameters,
        thread_queue_init_parameters const& init,
        scheduling_policy_init_parameters const& policy_init)
    {
        // times in microseconds
        max_idle_backoff_time_ =
            parameters.add("hpx.max_idle_backoff_time", 0.0f, max_time,
                per_worker, static_cast<float>(init.max_idle_backoff_time_));
        min_idle_backoff_time_ =
            parameters.add("hpx.min_idle_backoff_time", 0.0f, max_time,
                per_worker, static_cast<float>(init.min_idle_backoff_time_));
        steal_check_interval_ = parameters.add(
            "hpx.idle_backoff_steal_check_interval", 0.0f, max_time, per_worker,
            static_cast<float>(init.idle_backoff_steal_check_interval_));
        pauses_per_round_ = parameters.add("hpx.idle_backoff_spin_round_pauses",
            1u, 1024u, per_worker, policy_init.idle_backoff_spin_round_pauses_);
        free_pass_after_wake_ =
            parameters.add("hpx.scheduler_policy.free_pass_after_wake", 0u, 1u,
                pool_wide, as_switch(policy_init.free_pass_after_wake_));
    }

    void scheduling_policy::register_wake_parameters(
        scheduler_tuning_parameters& parameters,
        scheduling_policy_init_parameters const& policy_init)
    {
        // sizes by their number, from unknown (0) to large (4)
        wake_one_max_size_ =
            parameters.add("hpx.scheduler_policy.wake_one_max_size",
                as_number(threads::task_size::unknown),
                as_number(threads::task_size::large), pool_wide,
                as_number(policy_init.wake_one_max_size_));
        wake_target_only_ =
            parameters.add("hpx.scheduler_policy.wake_target_only", 0u, 1u,
                pool_wide, as_switch(policy_init.wake_target_only_));
        wake_if_nobody_spins_ =
            parameters.add("hpx.scheduler_policy.wake_if_nobody_spins", 0u, 1u,
                pool_wide, as_switch(policy_init.wake_if_nobody_spins_));
        wake_one_unless_placed_by_caller_ = parameters.add(
            "hpx.scheduler_policy.wake_one_unless_placed_by_caller", 0u, 1u,
            pool_wide,
            as_switch(policy_init.wake_one_unless_placed_by_caller_));
    }
}    // namespace hpx::threads::policies
