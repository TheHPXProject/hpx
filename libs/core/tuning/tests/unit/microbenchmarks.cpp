//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Smoke tests: the measurements run and return plausible values. Their size
// depends on the machine, so only signs and orders are checked.

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <cstddef>
#include <vector>

namespace tuning = hpx::tuning;

int main()
{
    tuning::microbenchmark_options const options{5};

    tuning::summary const clock = tuning::measure_clock_read(options);
    HPX_TEST_EQ(clock.count, std::size_t(5));
    HPX_TEST_LT(0.0, clock.median);
    HPX_TEST_LTE(clock.min, clock.median);
    HPX_TEST_LTE(clock.median, clock.max);

    tuning::summary const spin = tuning::measure_spin_rate(options);
    HPX_TEST_EQ(spin.count, std::size_t(5));
    HPX_TEST_LT(0.0, spin.median);

    std::vector<tuning::timed_wait> const waits =
        tuning::measure_timed_waits(8.0, options);
    HPX_TEST_EQ(waits.size(), std::size_t(4));    // 1, 2, 4 and 8 us
    for (tuning::timed_wait const& w : waits)
    {
        HPX_TEST_EQ(w.actual_us.count, std::size_t(5));
        HPX_TEST_LT(0.0, w.actual_us.median);
    }
    HPX_TEST_LT(0.0, tuning::shortest_sleep_us(waits));

    tuning::wake_cost const wake = tuning::measure_wake_cost(options);
    HPX_TEST_EQ(wake.asleep_us.count, std::size_t(5));
    HPX_TEST_EQ(wake.spinning_us.count, std::size_t(5));
    HPX_TEST_LTE(0.0, tuning::wake_cost_us(wake));

    return hpx::util::report_errors();
}
