//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <vector>

template <typename Rep, typename Period>
void test_duration(std::chrono::duration<Rep, Period> const& d)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << d;

    hpx::serialization::input_archive iarchive(buffer);
    std::chrono::duration<Rep, Period> loaded{};
    iarchive >> loaded;

    HPX_TEST(d == loaded);
    HPX_TEST_EQ(d.count(), loaded.count());
}

template <typename Clock, typename Duration>
void test_time_point(std::chrono::time_point<Clock, Duration> const& tp)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << tp;

    hpx::serialization::input_archive iarchive(buffer);
    std::chrono::time_point<Clock, Duration> loaded{};
    iarchive >> loaded;

    HPX_TEST(tp == loaded);
    HPX_TEST(tp.time_since_epoch() == loaded.time_since_epoch());
}

int main()
{
    // Various Rep/Period combinations.
    test_duration(std::chrono::nanoseconds(0));
    test_duration(std::chrono::nanoseconds(-42));
    test_duration(std::chrono::milliseconds(123456));
    test_duration(std::chrono::seconds(60));
    test_duration(std::chrono::duration<double>(3.14159));

    // Round-trip the sentinel values consumers such as supervision's
    // await_terminal timeout parameter rely on.
    test_duration((std::chrono::steady_clock::duration::max) ());
    test_duration((std::chrono::steady_clock::duration::min) ());

    // time_point: system_clock is meaningful across processes since it is
    // conventionally tied to the Unix epoch; steady_clock time_points are
    // only round-tripped here within the same process/clock instance.
    test_time_point(std::chrono::system_clock::now());
    test_time_point(std::chrono::steady_clock::now());
    test_time_point((std::chrono::steady_clock::time_point::min) ());
    test_time_point((std::chrono::steady_clock::time_point::max) ());

    return hpx::util::report_errors();
}
