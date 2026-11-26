//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/tuning/idle_settings.hpp>
#include <hpx/tuning/microbenchmarks.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace hpx::tuning {

    double shortest_sleep_us(std::vector<timed_wait> const& waits) noexcept
    {
        auto const it = std::min_element(waits.begin(), waits.end(),
            [](timed_wait const& lhs, timed_wait const& rhs) {
                return lhs.requested_us < rhs.requested_us;
            });
        return it == waits.end() ? 0.0 : it->actual_us.median;
    }

    double wake_cost_us(wake_cost const& wake) noexcept
    {
        return (
            std::max) (wake.asleep_us.median - wake.spinning_us.median, 0.0);
    }

    namespace {

        // settings are whole microseconds
        std::string microseconds(double const us)
        {
            return std::to_string(std::llround(us));
        }
    }    // namespace

    std::vector<tuned_setting> derive_idle_settings(
        idle_measurements const& measured, idle_rule_options const& options)
    {
        std::vector<tuned_setting> settings;

        double const sleep = shortest_sleep_us(measured.timed_waits);
        double const wake = wake_cost_us(measured.wake);
        bool const have_sleep = sleep > 0.0;

        // the longest step: at least as long as the shortest sleep
        double const max_backoff =
            (std::max) (options.default_max_backoff_us, std::ceil(sleep));
        settings.push_back(tuned_setting{"hpx.max_idle_backoff_time",
            microseconds(max_backoff),
            have_sleep ? hpx::util::format("shortest sleep {:.1f} us", sleep) :
                         std::string("no timed waits measured"),
            max_backoff == options.default_max_backoff_us});

        // the spin: the shortest sleep, at least the cost of a wake up, at
        // most the longest step
        double spin = options.default_spin_us;
        std::string reason = "no timed waits measured";
        if (have_sleep)
        {
            spin = (std::min) ((std::max) (sleep, wake), max_backoff);
            spin = (std::max) (std::round(spin), 1.0);
            reason = hpx::util::format(
                "shortest sleep {:.1f} us, wake cost {:.1f} us", sleep, wake);
        }
        settings.push_back(
            tuned_setting{"hpx.min_idle_backoff_time", microseconds(spin),
                HPX_MOVE(reason), spin == options.default_spin_us});

        return settings;
    }
}    // namespace hpx::tuning
