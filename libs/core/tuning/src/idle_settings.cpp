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

        // the longest step: at least as long as the shortest sleep; with a
        // coarse timer (a shortest sleep longer than the default longest
        // step) rounded up to whole milliseconds, as the sleep is then the
        // timer's tick as one run sees it, and two runs on the same machine
        // should write the same value
        bool const coarse_timer =
            have_sleep && sleep > options.default_max_backoff_us;
        double const max_backoff = coarse_timer ?
            std::ceil(sleep / 1000.0) * 1000.0 :
            (std::max) (options.default_max_backoff_us, std::ceil(sleep));
        std::string max_reason = "no timed waits measured";
        if (coarse_timer)
        {
            max_reason = hpx::util::format(
                "shortest sleep {:.1f} us, rounded up to whole milliseconds "
                "(a coarse timer)",
                sleep);
        }
        else if (have_sleep)
        {
            max_reason = hpx::util::format("shortest sleep {:.1f} us", sleep);
        }
        if (have_sleep && measured.timer_resolution_us > 0.0)
        {
            max_reason +=
                hpx::util::format(", under a timer resolution of {:.1f} us",
                    measured.timer_resolution_us);
        }
        settings.push_back(tuned_setting{"hpx.max_idle_backoff_time",
            microseconds(max_backoff), HPX_MOVE(max_reason),
            max_backoff == options.default_max_backoff_us});

        // the spin: the shortest sleep, at least the cost of a wake up, at
        // most the longest step
        double spin = options.default_spin_us;
        std::string reason = "no timed waits measured";
        if (coarse_timer)
        {
            // A coarse timer (Windows: a timed wait lasts the 15.6 ms tick):
            // the shortest sleep says how long an idle worker stays asleep,
            // not how soon new work reaches it, as a notification wakes it
            // after about the wake cost. Spinning that long would keep every
            // idle worker busy for little latency, so the spin stays HPX's
            // default.
            reason = hpx::util::format(
                "shortest sleep {:.1f} us is longer than the default longest "
                "step ({:.0f} us), a coarse timer; wake cost {:.1f} us",
                sleep, options.default_max_backoff_us, wake);
        }
        else if (have_sleep)
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
