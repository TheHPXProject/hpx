//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file idle_settings.hpp
/// \brief The rules that turn the measurements of the machine into the
///        scheduler's idle settings.

#pragma once

#include <hpx/config.hpp>
#include <hpx/tuning/microbenchmarks.hpp>
#include <hpx/tuning/statistics.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <vector>

namespace hpx::tuning {

    /// The measurements the idle rules use (see microbenchmarks.hpp).
    HPX_CXX_CORE_EXPORT struct idle_measurements
    {
        std::vector<timed_wait> timed_waits;
        wake_cost wake;
    };

    /// HPX's defaults the rules start from and compare with.
    HPX_CXX_CORE_EXPORT struct idle_rule_options
    {
        /// hpx.min_idle_backoff_time: the spin, in microseconds
        double default_spin_us = 50.0;
        /// hpx.max_idle_backoff_time: the longest backoff step, in
        /// microseconds
        double default_max_backoff_us = 1000.0;
    };

    /// The shortest sleep of the machine in microseconds: the median actual
    /// length of the shortest timed wait measured, 0 if none was.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT double shortest_sleep_us(
        std::vector<timed_wait> const& waits) noexcept;

    /// The cost of a wake up in microseconds: the median time from a
    /// notification to a sleeping thread running, less that to a spinning
    /// thread seeing a flag; at least 0.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT double wake_cost_us(
        wake_cost const& wake) noexcept;

    /// The idle settings for the given measurements:
    ///
    /// - hpx.max_idle_backoff_time: the default, or the shortest sleep if
    ///   that is longer (a step can't be shorter than a sleep lasts);
    /// - hpx.min_idle_backoff_time (the spin): the shortest sleep, at least
    ///   the cost of a wake up, at most hpx.max_idle_backoff_time. A backoff
    ///   step shorter than the shortest sleep, slept, lasts as long as that
    ///   sleep, so it is spun instead; and spinning about as long as a wake
    ///   costs is the competitive rule of spinning before blocking.
    ///
    /// Without measurements a setting keeps its default and says so.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<tuned_setting>
    derive_idle_settings(idle_measurements const& measured,
        idle_rule_options const& options = {});
}    // namespace hpx::tuning
