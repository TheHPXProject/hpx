//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <string>
#include <vector>

namespace tuning = hpx::tuning;

namespace {

    tuning::summary at(double const median)
    {
        tuning::summary s;
        s.count = 1;
        s.min = s.q1 = s.median = s.q3 = s.p90 = s.max = median;
        return s;
    }

    // timed waits of 1, 2, 4 ... 1024 us that last at least the given time
    std::vector<tuning::timed_wait> waits(double const shortest)
    {
        std::vector<tuning::timed_wait> w;
        for (double r = 1.0; r <= 1024.0; r *= 2.0)
            w.push_back(tuning::timed_wait{r, at(r < shortest ? shortest : r)});
        return w;
    }

    tuning::tuned_setting const& find(
        std::vector<tuning::tuned_setting> const& s, std::string const& key)
    {
        for (auto const& setting : s)
        {
            if (setting.key == key)
                return setting;
        }
        HPX_TEST_MSG(false, key.c_str());
        return s.front();
    }
}    // namespace

// a Linux timer slack of about 55 us and a wake cost of 18 us: the spin
// is the shortest sleep, the longest step stays the default
void test_linux_like()
{
    tuning::idle_measurements m;
    m.timed_waits = waits(55.2);
    m.wake.asleep_us = at(22.0);
    m.wake.spinning_us = at(4.0);

    HPX_TEST_EQ(tuning::shortest_sleep_us(m.timed_waits), 55.2);
    HPX_TEST_EQ(tuning::wake_cost_us(m.wake), 18.0);

    auto const s = tuning::derive_idle_settings(m);
    HPX_TEST_EQ(find(s, "hpx.min_idle_backoff_time").value, std::string("55"));
    HPX_TEST(!find(s, "hpx.min_idle_backoff_time").is_default);
    HPX_TEST_EQ(
        find(s, "hpx.max_idle_backoff_time").value, std::string("1000"));
    HPX_TEST(find(s, "hpx.max_idle_backoff_time").is_default);
}

// Windows: every timed wait lasts a millisecond or more, longer than the
// default longest step (a coarse timer); the longest step grows to the
// shortest sleep, the spin stays the default, as a notification wakes a
// sleeping worker long before its timed wait would end
void test_windows_like()
{
    tuning::idle_measurements m;
    m.timed_waits = waits(1100.4);
    m.wake.asleep_us = at(30.0);
    m.wake.spinning_us = at(1.0);

    auto const s = tuning::derive_idle_settings(m);
    HPX_TEST_EQ(
        find(s, "hpx.max_idle_backoff_time").value, std::string("1101"));
    HPX_TEST_EQ(find(s, "hpx.min_idle_backoff_time").value, std::string("50"));
    HPX_TEST(find(s, "hpx.min_idle_backoff_time").is_default);
    HPX_TEST(find(s, "hpx.min_idle_backoff_time").reason.find("coarse") !=
        std::string::npos);
}

// the 15.6 ms tick of Windows' default timer, and a wake cost of 108 us as
// measured on a 32 core Windows machine
void test_windows_timer_tick()
{
    tuning::idle_measurements m;
    m.timed_waits = waits(15821.5);
    m.wake.asleep_us = at(110.2);
    m.wake.spinning_us = at(1.8);

    auto const s = tuning::derive_idle_settings(m);
    HPX_TEST_EQ(
        find(s, "hpx.max_idle_backoff_time").value, std::string("15822"));
    HPX_TEST_EQ(find(s, "hpx.min_idle_backoff_time").value, std::string("50"));
    // the reason records the wake cost
    HPX_TEST(find(s, "hpx.min_idle_backoff_time").reason.find("108.4") !=
        std::string::npos);
}

// a shortest sleep of exactly the default longest step is not a coarse
// timer: the spin is the shortest sleep
void test_sleep_at_longest_step()
{
    tuning::idle_measurements m;
    m.timed_waits = waits(1000.0);
    m.wake.asleep_us = at(30.0);
    m.wake.spinning_us = at(1.0);

    auto const s = tuning::derive_idle_settings(m);
    HPX_TEST_EQ(
        find(s, "hpx.min_idle_backoff_time").value, std::string("1000"));
}

// a wake costing more than the shortest sleep lengthens the spin
void test_expensive_wake()
{
    tuning::idle_measurements m;
    m.timed_waits = waits(50.0);
    m.wake.asleep_us = at(130.0);
    m.wake.spinning_us = at(10.0);

    auto const s = tuning::derive_idle_settings(m);
    HPX_TEST_EQ(find(s, "hpx.min_idle_backoff_time").value, std::string("120"));
}

// without measurements both settings keep their defaults and say why
void test_no_measurements()
{
    auto const s = tuning::derive_idle_settings(tuning::idle_measurements{});
    HPX_TEST_EQ(find(s, "hpx.min_idle_backoff_time").value, std::string("50"));
    HPX_TEST(find(s, "hpx.min_idle_backoff_time").is_default);
    HPX_TEST_EQ(
        find(s, "hpx.max_idle_backoff_time").value, std::string("1000"));
    HPX_TEST(find(s, "hpx.max_idle_backoff_time").is_default);
    HPX_TEST(!find(s, "hpx.min_idle_backoff_time").reason.empty());
}

int main()
{
    test_linux_like();
    test_windows_like();
    test_windows_timer_tick();
    test_sleep_at_longest_step();
    test_expensive_wake();
    test_no_measurements();
    return hpx::util::report_errors();
}
