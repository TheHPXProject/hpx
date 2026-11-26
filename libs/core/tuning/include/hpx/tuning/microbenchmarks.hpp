//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file microbenchmarks.hpp
/// \brief The measurements of the machine that need no HPX runtime: the
///        cost of a clock read, the rate of the scheduler's spin rounds, the
///        real length of short timed waits and the cost of waking a sleeping
///        thread.

#pragma once

#include <hpx/config.hpp>
#include <hpx/tuning/statistics.hpp>

#include <cstddef>
#include <vector>

namespace hpx::tuning {

    /// How many times each measurement is repeated (the summaries are over
    /// these repeats).
    HPX_CXX_CORE_EXPORT struct microbenchmark_options
    {
        std::size_t repeats = 21;
    };

    /// The cost of one std::chrono::steady_clock::now(), in nanoseconds.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT summary measure_clock_read(
        microbenchmark_options const& options = {});

    /// The rate of the idle backoff's spin rounds on the calling thread, in
    /// rounds per microsecond: a round of
    /// scheduling_policy::calibration_pauses_per_round pauses and a relaxed
    /// load, as the scheduler calibrates its spin. The rate depends on the
    /// core the thread runs on (core types differ), so the caller pins the
    /// thread first.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT summary measure_spin_rate(
        microbenchmark_options const& options = {});

    /// The actual length of a timed wait of a condition variable that
    /// nobody notifies, for one requested length.
    HPX_CXX_CORE_EXPORT struct timed_wait
    {
        double requested_us = 0.0;
        summary actual_us;
    };

    /// The actual lengths of timed waits of 1, 2, 4, ... up to max_us
    /// microseconds. The shortest one is the shortest sleep of the machine:
    /// about the timer slack on Linux, a millisecond or more on Windows.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<timed_wait>
    measure_timed_waits(
        double max_us = 1024.0, microbenchmark_options const& options = {});

    /// The time from a notification to the notified thread running, in
    /// microseconds, for a thread that sleeps on a condition variable and for
    /// one that spins on a flag; their difference is the cost of a wake up
    /// outside the scheduler.
    HPX_CXX_CORE_EXPORT struct wake_cost
    {
        summary asleep_us;
        summary spinning_us;
    };

    /// Measure wake_cost with two threads of the operating system.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT wake_cost measure_wake_cost(
        microbenchmark_options const& options = {});
}    // namespace hpx::tuning
