//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/threading_base/scheduler_tuning_counters.hpp>

#include <array>
#include <cstddef>

namespace hpx::threads::policies {

    namespace {

        constexpr std::array<char const*, scheduler_tuning_counters::size>
            counter_names = {{
                "created_unknown",
                "created_nano",
                "created_small",
                "created_medium",
                "created_large",
                "run_unknown",
                "run_nano",
                "run_small",
                "run_medium",
                "run_large",
                "wake_none",
                "wake_target_if_sleeping",
                "wake_one",
                "wake_all",
                "wake_by_creator",
                "woke_sleeper",
                "sleeper_woken_while_spinning",
                "sleeper_woken_with_sentinel",
                "idle_spin_own_work",
                "idle_spin_work_to_steal",
                "idle_spin_ran_out",
                "idle_sentinel",
                "idle_work_before_wait",
                "idle_wait_notified",
                "idle_wait_timed_out",
                "stolen_unknown",
                "stolen_nano",
                "stolen_small",
                "stolen_medium",
                "stolen_large",
                "steal_staged",
            }};

        // a name for every counter
        static_assert(counter_names.back() != nullptr);
    }    // namespace

    char const* get_scheduler_counter_name(scheduler_counter const c) noexcept
    {
        auto const i = static_cast<std::size_t>(c);
        return i < counter_names.size() ? counter_names[i] : "unknown";
    }

    scheduler_tuning_counters::scheduler_tuning_counters(
        std::size_t const num_workers)
      : values_(num_workers)
    {
    }
}    // namespace hpx::threads::policies
