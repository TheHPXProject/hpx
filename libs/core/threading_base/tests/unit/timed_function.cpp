//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks timed_task: a time maps to the largest size whose run time it
// reaches, nano below that (task_size_of_duration), the wrapper has that size and
// keeps the time (get_task_size, get_task_time), forwards its arguments, and
// the wrappers that forward sizes (the binders, deferred_call, one_shot,
// annotated_function, size_of_function) forward the time as well.

#include <hpx/functional.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <cstdint>

using hpx::chrono::picoseconds;
using hpx::threads::task_size;
using hpx::threads::task_size_and_time;
using hpx::threads::task_size_of_duration;
using hpx::traits::get_task_size;
using hpx::traits::get_task_time;

namespace {

    int add_one(int i)
    {
        return i + 1;
    }

    void test_task_size_of_duration()
    {
        // no time, no size
        HPX_TEST(task_size_of_duration(picoseconds(0)) == task_size::unknown);
        HPX_TEST(task_size_of_duration(picoseconds(-1)) == task_size::unknown);

        // the largest size whose run time the time reaches, nano below
        HPX_TEST(task_size_of_duration(picoseconds(250)) == task_size::nano);
        HPX_TEST(task_size_of_duration(picoseconds(1000)) == task_size::nano);
        HPX_TEST(task_size_of_duration(picoseconds(30000)) == task_size::nano);
        HPX_TEST(
            task_size_of_duration(picoseconds(100000)) == task_size::small_);
        HPX_TEST(
            task_size_of_duration(picoseconds(5000000)) == task_size::small_);
        HPX_TEST(
            task_size_of_duration(picoseconds(2000000000)) == task_size::large);

        static_assert(
            task_size_of_duration(picoseconds(500)) == task_size::nano);

        // where only the size is needed
        task_size const size =
            task_size_and_time{task_size::nano, picoseconds(250)};
        HPX_TEST(size == task_size::nano);
    }

    template <typename F>
    void check_timed(F const& f, task_size const size, std::int64_t const ps)
    {
        HPX_TEST(get_task_size<F>::call(f) == size);
        HPX_TEST_EQ(get_task_time<F>::call(f).count(), ps);
    }

    void test_timed_task()
    {
        auto const t = hpx::timed_task(&add_one, picoseconds(250));
        check_timed(t, task_size::nano, 250);
        HPX_TEST_EQ(t(1), 2);

        // the type alone doesn't know the time
        static_assert(
            hpx::traits::get_task_size_v<decltype(t)> == task_size::unknown);

        // any duration type, kept in picoseconds
        check_timed(hpx::timed_task(&add_one,
                        std::chrono::duration<double, std::nano>(0.75)),
            task_size::nano, 750);
        check_timed(hpx::timed_task(&add_one, std::chrono::nanoseconds(30)),
            task_size::nano, 30000);
        check_timed(hpx::timed_task(&add_one, std::chrono::microseconds(10)),
            task_size::medium, 10000000);

        // no time, no size
        check_timed(hpx::timed_task(&add_one, std::chrono::nanoseconds(0)),
            task_size::unknown, 0);
        check_timed(hpx::timed_task(&add_one, std::chrono::hours(-1)),
            task_size::unknown, 0);

        // beyond the range of picoseconds (about 106 days): saturated
        check_timed(hpx::timed_task(&add_one, std::chrono::hours(24 * 200)),
            task_size::large, (picoseconds::max) ().count());

        // functions that say nothing finer than their size
        auto const lambda = [](int i) { return i; };
        HPX_TEST_EQ(get_task_time<decltype(lambda)>::call(lambda).count(),
            std::int64_t(0));
        auto const nano = hpx::nano_task(lambda);
        HPX_TEST_EQ(
            get_task_time<decltype(nano)>::call(nano).count(), std::int64_t(0));
    }

    void test_wrappers()
    {
        auto const t = hpx::timed_task(&add_one, picoseconds(750));

        auto const bound = hpx::bind(t, hpx::placeholders::_1);
        check_timed(bound, task_size::nano, 750);
        HPX_TEST_EQ(bound(1), 2);

        check_timed(hpx::bind_front(t), task_size::nano, 750);
        check_timed(hpx::bind_back(t), task_size::nano, 750);
        check_timed(hpx::util::deferred_call(t, 1), task_size::nano, 750);
        check_timed(hpx::util::one_shot(t), task_size::nano, 750);
        check_timed(hpx::annotated_function(t, "timed"), task_size::nano, 750);

        // size_of_function: read from a function, or given
        auto const g = [](int i) { return i; };
        check_timed(hpx::threads::detail::with_task_size_of(g, t),
            task_size::nano, 750);
        check_timed(hpx::threads::detail::with_task_size(g, task_size::nano),
            task_size::nano, 0);
        check_timed(
            hpx::threads::detail::with_task_size(
                g, task_size_and_time{task_size::small_, picoseconds(30000)}),
            task_size::small_, 30000);
    }
}    // namespace

int main()
{
    test_task_size_of_duration();
    test_timed_task();
    test_wrappers();

    return hpx::util::report_errors();
}
