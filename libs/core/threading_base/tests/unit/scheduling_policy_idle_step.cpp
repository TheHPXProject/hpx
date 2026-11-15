//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The idle steps of the tuned scheduling rules (rules 6 and 7) follow the
// current values of the per-worker settings, also for a period an idle worker
// sleeps through (or spins through as the sentinel).

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
using hpx::threads::policies::idle_step;
using hpx::threads::policies::scheduling_policy;
using hpx::threads::policies::scheduling_policy_init_parameters;
using hpx::threads::policies::thread_queue_init_parameters;
using hpx::threads::policies::tuned_scheduling_rules;

namespace {

    // rounds of calibration_pauses_per_round pauses per microsecond, as
    // measured at startup
    constexpr double rate = 4.0;

    std::uint64_t rounds(double const us, double const rounds_per_us)
    {
        return static_cast<std::uint64_t>(std::llround(us * rounds_per_us));
    }

    struct rules_for_test
    {
        rules_for_test()
          : rules(2)
        {
            rules.register_parameters(thread_queue_init_parameters(),
                scheduling_policy_init_parameters());

            auto const& parameters = rules.parameters();
            min_time = parameters.find("hpx.min_idle_backoff_time");
            max_time = parameters.find("hpx.max_idle_backoff_time");
            interval = parameters.find("hpx.idle_backoff_steal_check_interval");
            HPX_TEST(min_time != parameters.npos);
            HPX_TEST(max_time != parameters.npos);
            HPX_TEST(interval != parameters.npos);
        }

        tuned_scheduling_rules rules;
        std::size_t min_time;
        std::size_t max_time;
        std::size_t interval;
    };

    // the period after this many periods without work (2^30 us) is capped at
    // hpx.max_idle_backoff_time, which is not shorter than
    // hpx.min_idle_backoff_time: it is slept through
    constexpr std::uint32_t sleep_count = 30;

    void test_spin_step()
    {
        rules_for_test t;
        auto& parameters = t.rules.parameters();

        // the first period (1 us) is spun through
        idle_step const step = t.rules.idle_step_for(0, 0, rate);
        HPX_TEST_EQ(step.period.count(), 1);
        HPX_TEST_EQ(step.pauses_per_round,
            scheduling_policy::calibration_pauses_per_round);
        HPX_TEST_EQ(step.spin_rounds, rounds(1.0, rate));
        HPX_TEST_EQ(step.steal_check_rounds,
            rounds(parameters.get(0, t.interval), rate));
    }

    void test_sleep_step()
    {
        rules_for_test t;
        auto& parameters = t.rules.parameters();

        idle_step const step = t.rules.idle_step_for(0, sleep_count, rate);
        HPX_TEST_EQ(step.spin_rounds, std::uint64_t(0));
        HPX_TEST_EQ(static_cast<double>(step.period.count()),
            parameters.get(0, t.max_time));

        // the interval to look at the queues to steal from is given in
        // rounds also for a period to sleep, for a worker that becomes the
        // sentinel and spins through it instead
        HPX_TEST_EQ(step.steal_check_rounds,
            rounds(parameters.get(0, t.interval), rate));
    }

    void test_tuned_interval()
    {
        rules_for_test t;
        auto& parameters = t.rules.parameters();

        // a tuner changes the interval of worker 1 only
        parameters.set(1, t.interval, 20.0);

        idle_step const step0 = t.rules.idle_step_for(0, sleep_count, rate);
        idle_step const step1 = t.rules.idle_step_for(1, sleep_count, rate);
        HPX_TEST_EQ(step0.steal_check_rounds,
            rounds(parameters.get(0, t.interval), rate));
        HPX_TEST_EQ(step1.steal_check_rounds, rounds(20.0, rate));

        idle_step const spin1 = t.rules.idle_step_for(1, 0, rate);
        HPX_TEST_EQ(spin1.steal_check_rounds, rounds(20.0, rate));
    }

    void test_at_least_one_round()
    {
        rules_for_test t;
        auto& parameters = t.rules.parameters();

        // an interval shorter than a round still looks every round
        parameters.set_all(t.interval, 0.0);

        HPX_TEST_EQ(t.rules.idle_step_for(0, 0, rate).steal_check_rounds,
            std::uint64_t(1));
        HPX_TEST_EQ(
            t.rules.idle_step_for(0, sleep_count, rate).steal_check_rounds,
            std::uint64_t(1));
    }
}    // namespace
#endif

int main()
{
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
    test_spin_step();
    test_sleep_step();
    test_tuned_interval();
    test_at_least_one_round();
#endif

    return hpx::util::report_errors();
}
