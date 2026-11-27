//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The region times, their fit, the rules and the sections of the chunk area:
// the fit and the rules on made-up region times (so that the values are
// known), the lines a child writes, and a short measurement on the running
// runtime.

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace tuning = hpx::tuning;

namespace {

    tuning::summary at(double const median)
    {
        tuning::summary s;
        s.count = 5;
        s.min = median * 0.9;
        s.q1 = median * 0.95;
        s.median = median;
        s.q3 = median * 1.05;
        s.p90 = median * 1.08;
        s.max = median * 1.1;
        return s;
    }

    // region times that follow t = r + (indices / n) * o exactly
    void add_times(std::vector<tuning::region_time>& times,
        std::string const& executor, std::size_t const n, double const r,
        double const o)
    {
        for (std::size_t const k : {1, 4, 16})
        {
            times.push_back(tuning::region_time{
                executor, n, k * n, at(r + static_cast<double>(k) * o)});
        }
    }

    bool close_to(double const a, double const b)
    {
        return std::abs(a - b) <= 1e-9 * (std::abs(a) + std::abs(b) + 1.0);
    }

    std::vector<tuning::region_time> made_up_times()
    {
        std::vector<tuning::region_time> times;
        add_times(times, "parallel", 16, 10.0, 0.5);
        add_times(times, "fork_join", 16, 4.0, 0.1);
        add_times(times, "parallel", 32, 20.0, 1.0);
        add_times(times, "fork_join", 32, 6.0, 0.1);
        // only one number of indices: not fitted
        times.push_back(
            tuning::region_time{"suspending_fork_join", 16, 16, at(3.0)});
        return times;
    }

    void test_fit()
    {
        std::vector<tuning::chunk_cost> const costs =
            tuning::fit_chunk_costs(made_up_times());
        HPX_TEST_EQ(costs.size(), std::size_t(4));

        HPX_TEST_EQ(costs[1].executor, std::string("fork_join"));
        HPX_TEST_EQ(costs[1].pool_size, std::size_t(16));
        HPX_TEST(close_to(costs[1].region_us, 4.0));
        HPX_TEST(close_to(costs[1].chunk_us, 0.1));
        HPX_TEST(costs[1].residual_us < 1e-9);
        HPX_TEST(close_to(costs[1].time_at_pool_size_us, 4.1));
        HPX_TEST(close_to(costs[1].spread, 0.1));
    }

    // the minimal chunk time of a section's only setting
    double value_of(std::vector<tuning::ini_section> const& sections,
        std::string const& name)
    {
        for (tuning::ini_section const& s : sections)
        {
            if (s.name == name && s.settings.size() == 1)
            {
                HPX_TEST_EQ(s.settings[0].key, name + ".min_chunk_time_us");
                return std::stod(s.settings[0].value);
            }
        }
        HPX_TEST_MSG(false, ("no section " + name).c_str());
        return -1.0;
    }

    void test_rules()
    {
        std::vector<tuning::chunk_cost> const costs =
            tuning::fit_chunk_costs(made_up_times());

        // fitted: parallel at 16 workers costs 0.5 + 10 / 16 = 1.125 us per
        // worker, so f = 2.5 / 1.125; fork_join costs 0.1 + 4 / 16 = 0.35
        {
            auto const sections =
                tuning::chunk_sections(costs, 2.5, tuning::chunk_rule::fitted);
            HPX_TEST_EQ(sections.size(), std::size_t(6));
            HPX_TEST(std::abs(value_of(sections,
                                  "hpx.task_size_chunk_size.fork_join.16") -
                         2.5 * 0.35 / 1.125) < 0.001);
            HPX_TEST(std::abs(value_of(sections,
                                  "hpx.task_size_chunk_size.parallel.16") -
                         2.5) < 0.001);

            // the section for the pool sizes not measured has the largest
            // pool size's value: 2.5 * (0.1 + 6 / 32) / (1 + 20 / 32)
            HPX_TEST(
                std::abs(
                    value_of(sections, "hpx.task_size_chunk_size.fork_join") -
                    2.5 * (0.1 + 6.0 / 32.0) / (1.0 + 20.0 / 32.0)) < 0.001);

            // parallel_executor's settings are the default, the others not
            for (tuning::ini_section const& s : sections)
            {
                bool const parallel =
                    s.name.find(".parallel") != std::string::npos;
                HPX_TEST_EQ(s.settings[0].is_default, parallel);
                HPX_TEST(
                    s.settings[0].reason.find("workers") != std::string::npos);
            }
        }

        // ratio: 2.5 * 4.1 / 10.5 at 16 workers
        {
            auto const sections =
                tuning::chunk_sections(costs, 2.5, tuning::chunk_rule::ratio);
            HPX_TEST(std::abs(value_of(sections,
                                  "hpx.task_size_chunk_size.fork_join.16") -
                         2.5 * 4.1 / 10.5) < 0.001);
        }

        // no parallel_executor at a pool size: nothing for it
        {
            std::vector<tuning::chunk_cost> only_fork_join;
            for (tuning::chunk_cost const& c : costs)
            {
                if (c.executor == "fork_join")
                    only_fork_join.push_back(c);
            }
            HPX_TEST(tuning::chunk_sections(
                only_fork_join, 2.5, tuning::chunk_rule::fitted)
                    .empty());
        }

        HPX_TEST(tuning::chunk_rule_from_string("fitted") ==
            tuning::chunk_rule::fitted);
        HPX_TEST(tuning::chunk_rule_from_string("ratio") ==
            tuning::chunk_rule::ratio);
        HPX_TEST(!tuning::chunk_rule_from_string("other"));
        HPX_TEST_EQ(std::string(tuning::to_string(tuning::chunk_rule::ratio)),
            std::string("ratio"));
    }

    void test_lines()
    {
        tuning::region_time const time{"fork_join", 16, 64, at(5.25)};
        std::optional<tuning::region_time> const back =
            tuning::from_line(tuning::to_line(time));
        HPX_TEST(back.has_value());
        HPX_TEST_EQ(back->executor, time.executor);
        HPX_TEST_EQ(back->pool_size, time.pool_size);
        HPX_TEST_EQ(back->indices, time.indices);
        HPX_TEST_EQ(back->time_us.count, time.time_us.count);
        HPX_TEST(close_to(back->time_us.median, time.time_us.median));
        HPX_TEST(close_to(back->time_us.q3, time.time_us.q3));

        HPX_TEST(!tuning::from_line(""));
        HPX_TEST(!tuning::from_line("fork_join 16 64"));
        HPX_TEST(!tuning::from_line(tuning::to_line(time) + " more"));
    }

    // the median of several runs: per executor, pool size and number of
    // indices, the run in the middle by its median
    void test_median_of_runs()
    {
        std::vector<tuning::region_time> times;
        for (double const median : {5.0, 9.0, 6.0})
        {
            times.push_back(
                tuning::region_time{"fork_join", 16, 16, at(median)});
            times.push_back(
                tuning::region_time{"fork_join", 16, 64, at(2.0 * median)});
        }
        // a second executor with an even number of runs: the lower middle
        times.push_back(tuning::region_time{"parallel", 16, 16, at(12.0)});
        times.push_back(tuning::region_time{"parallel", 16, 16, at(10.0)});

        std::vector<tuning::region_time> const combined =
            tuning::median_of_runs(times);
        HPX_TEST_EQ(combined.size(), std::size_t(3));

        HPX_TEST_EQ(combined[0].executor, std::string("fork_join"));
        HPX_TEST_EQ(combined[0].indices, std::size_t(16));
        HPX_TEST(close_to(combined[0].time_us.median, 6.0));
        // the whole summary of that run, not a mix of the runs
        HPX_TEST(close_to(combined[0].time_us.q3, 6.0 * 1.05));

        HPX_TEST_EQ(combined[1].indices, std::size_t(64));
        HPX_TEST(close_to(combined[1].time_us.median, 12.0));

        HPX_TEST_EQ(combined[2].executor, std::string("parallel"));
        HPX_TEST(close_to(combined[2].time_us.median, 10.0));

        // one run: unchanged
        std::vector<tuning::region_time> const one(
            times.begin(), times.begin() + 2);
        HPX_TEST_EQ(tuning::median_of_runs(one).size(), std::size_t(2));
        HPX_TEST(tuning::median_of_runs({}).empty());
    }

    // a short measurement: every executor, both numbers of indices
    void test_measure()
    {
        tuning::chunk_cost_options options;
        options.regions = 5;
        options.repeats = 3;
        options.multiples = {1, 2};

        std::vector<tuning::region_time> const times =
            tuning::measure_region_times(options);
        HPX_TEST_EQ(
            times.size(), tuning::chunk_executors().size() * std::size_t(2));
        for (tuning::region_time const& t : times)
        {
            HPX_TEST(t.pool_size > 0);
            HPX_TEST(t.indices == t.pool_size || t.indices == 2 * t.pool_size);
            HPX_TEST_EQ(t.time_us.count, std::size_t(3));
            HPX_TEST(t.time_us.median > 0.0);
        }
        HPX_TEST_EQ(tuning::fit_chunk_costs(times).size(),
            tuning::chunk_executors().size());
    }

    int hpx_main()
    {
        test_measure();
        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    test_fit();
    test_rules();
    test_lines();
    test_median_of_runs();

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
