//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks that steady_duration keeps a duration below a clock tick: value()
// is rounded up to whole ticks as before, as_picoseconds() gives the duration
// as it was given, and saturates for durations beyond the range of
// picoseconds.

#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <cstdint>

using hpx::chrono::picoseconds;
using hpx::chrono::steady_duration;

namespace {

    void check_duration(steady_duration const& d,
        std::chrono::nanoseconds const value, std::int64_t const ps)
    {
        HPX_TEST(d.value() == value);
        HPX_TEST_EQ(d.as_picoseconds().count(), ps);
    }

    void test_below_a_tick()
    {
        using std::chrono::nanoseconds;

        // rounded up, the given duration kept
        check_duration(picoseconds(250), nanoseconds(1), 250);
        check_duration(picoseconds(1500), nanoseconds(2), 1500);
        check_duration(std::chrono::duration<double, std::nano>(0.75),
            nanoseconds(1), 750);

        // negative durations: rounded toward zero (up), the given duration
        // kept
        check_duration(picoseconds(-250), nanoseconds(0), -250);
        check_duration(picoseconds(-1500), nanoseconds(-1), -1500);

        // whole ticks
        check_duration(picoseconds(1000), nanoseconds(1), 1000);
        check_duration(nanoseconds(5), nanoseconds(5), 5000);
        check_duration(
            std::chrono::microseconds(3), nanoseconds(3000), 3000000);

        // a duration below a tick is not zero
        HPX_TEST(static_cast<bool>(steady_duration(picoseconds(1))));
        HPX_TEST(!static_cast<bool>(hpx::chrono::null_duration));
        HPX_TEST_EQ(hpx::chrono::null_duration.as_picoseconds().count(),
            std::int64_t(0));
    }

    void test_long_durations()
    {
        // long durations as before
        steady_duration const two_hours(std::chrono::hours(2));
        HPX_TEST(two_hours.value() == std::chrono::hours(2));
        HPX_TEST_EQ(two_hours.as_picoseconds().count(),
            std::int64_t(7200) * 1000000000000);

        // beyond the range of picoseconds (about 106 days)
        steady_duration const long_wait(std::chrono::hours(24 * 200));
        HPX_TEST(long_wait.value() == std::chrono::hours(24 * 200));
        HPX_TEST(long_wait.as_picoseconds() == (picoseconds::max) ());
    }

    void test_saturating_product()
    {
        using hpx::chrono::detail::saturating_product;

        HPX_TEST_EQ(saturating_product(picoseconds(250), 4).count(),
            std::int64_t(1000));
        HPX_TEST_EQ(
            saturating_product(picoseconds(250), 0).count(), std::int64_t(0));

        // beyond the range of picoseconds, also for counts that don't fit
        // in std::int64_t
        HPX_TEST(saturating_product(picoseconds(2), std::uint64_t(1) << 62) ==
            (picoseconds::max) ());
        HPX_TEST(saturating_product(picoseconds(2), std::uint64_t(1) << 63) ==
            (picoseconds::max) ());
        HPX_TEST(saturating_product(picoseconds(1), ~std::uint64_t(0)) ==
            (picoseconds::max) ());
    }
}    // namespace

int main()
{
    test_below_a_tick();
    test_long_durations();
    test_saturating_product();

    return hpx::util::report_errors();
}
