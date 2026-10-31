//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Sweep the two parameters of task_size_chunk_size, the minimal run time of a
// chunk and the number of chunks per core, for hpx::for_each with nano
// (about 1 ns), small (about 100 ns) and medium (about 10 us) loop bodies. The
// loops run for 1 us, 10 us, ... up to --max_work_us of work in total, as
// given by the task size of their body. Each case is compared to the default
// executor parameters.
//
// Prints one CSV line per case and parameters (min_us and cpc are 0 for the
// default parameters), then for each pair of parameters the geometric mean of
// the run times relative to the default parameters and to the best parameters
// of each case, and the worst case relative to the default parameters.
// Finally, it prints the pair closest to the best parameters of each case
// (the lowest geometric mean relative to them) as the configuration entries
// that make it the default of task_size_chunk_size, for instance for an
// hpx.ini file. Run it with all cores of the machine to tune for (the default
// of --hpx:threads).
//
// The compiled defaults of task_size_chunk_size (2.5 us and 8 chunks per
// core) were chosen from this sweep at 4, 8, 16 and 32 cores.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

    namespace ex = hpx::execution::experimental;

    constexpr double min_chunk_times_us[] = {
        1.0, 2.5, 5.0, 10.0, 25.0, 50.0, 100.0};
    constexpr std::size_t chunks_per_core[] = {1, 2, 4, 8};

    struct sweep_config
    {
        double max_work_us;
        std::chrono::milliseconds time_per_case;
        std::size_t min_reps;
        std::size_t max_reps;
    };

    // the median run times of one loop, for the default parameters and for
    // each pair of parameters in the order of the sweep
    struct case_result
    {
        std::string label;
        double default_us;
        std::vector<double> sweep_us;
    };

    // spin for about d (the body of small and medium iterations)
    void spin(std::chrono::nanoseconds const d)
    {
        auto const end = std::chrono::steady_clock::now() + d;
        while (std::chrono::steady_clock::now() < end)
        {
        }
    }

    // run f once to warm up, then repeat it for about time_per_case, at
    // least min_reps and at most max_reps times, print and return the median
    template <typename F>
    double run(sweep_config const& cfg, std::string const& size,
        std::size_t const count, double const min_us, std::size_t const cpc,
        F&& f)
    {
        f();

        std::vector<double> times;
        auto const start = std::chrono::steady_clock::now();
        while (times.size() < cfg.min_reps ||
            (times.size() < cfg.max_reps &&
                std::chrono::steady_clock::now() - start < cfg.time_per_case))
        {
            auto const t0 = std::chrono::steady_clock::now();
            f();
            times.push_back(std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0)
                    .count());
        }

        std::sort(times.begin(), times.end());
        double const median = times[times.size() / 2];
        std::cout << size << ',' << count << ',' << min_us << ',' << cpc << ','
                  << median << ',' << times.front() << ',' << times.size()
                  << std::endl;
        return median;
    }

    template <typename Body>
    void sweep_size(sweep_config const& cfg, std::string const& size, Body body,
        std::vector<case_result>& results)
    {
        auto const per_iteration =
            hpx::task_size_duration(hpx::traits::get_task_size_v<Body>);
        double const per_iteration_us =
            std::chrono::duration<double, std::micro>(per_iteration).count();

        for (double work_us = 1.0; work_us <= cfg.max_work_us; work_us *= 10)
        {
            auto const count = static_cast<std::size_t>(
                std::llround(work_us / per_iteration_us));
            if (count == 0)
            {
                continue;
            }

            std::vector<std::uint64_t> v(count, 1);
            auto const for_each_with = [&](auto policy) {
                return [&, policy] {
                    hpx::for_each(policy, v.begin(), v.end(), body);
                };
            };

            case_result result{size + " x " + std::to_string(count),
                run(cfg, size, count, 0, 0, for_each_with(hpx::execution::par)),
                {}};

            for (double const min_us : min_chunk_times_us)
            {
                auto const min_chunk_time = std::chrono::nanoseconds(
                    static_cast<std::int64_t>(min_us * 1000));
                for (std::size_t const cpc : chunks_per_core)
                {
                    result.sweep_us.push_back(run(cfg, size, count, min_us, cpc,
                        for_each_with(hpx::execution::par.with(
                            ex::task_size_chunk_size(min_chunk_time, cpc)))));
                }
            }
            results.push_back(std::move(result));
        }
    }

    void print_summary(std::vector<case_result> const& results)
    {
        if (results.empty())
        {
            return;
        }

        std::cout << "\nmin_us,cpc,geomean_vs_default,geomean_vs_best,"
                     "worst_vs_default,worst_case"
                  << std::endl;

        std::size_t index = 0;
        double best_geomean = 0.0;
        double best_min_us = 0.0;
        std::size_t best_cpc = 0;
        for (double const min_us : min_chunk_times_us)
        {
            for (std::size_t const cpc : chunks_per_core)
            {
                double log_vs_default = 0.0;
                double log_vs_best = 0.0;
                double worst = 0.0;
                std::string const* worst_case = nullptr;
                for (case_result const& r : results)
                {
                    double const t = r.sweep_us[index];
                    double const vs_default = t / r.default_us;
                    double const best = (std::min) (r.default_us,
                        *std::min_element(
                            r.sweep_us.begin(), r.sweep_us.end()));

                    log_vs_default += std::log(vs_default);
                    log_vs_best += std::log(t / best);
                    if (vs_default > worst)
                    {
                        worst = vs_default;
                        worst_case = &r.label;
                    }
                }

                auto const n = static_cast<double>(results.size());
                double const geomean_vs_best = std::exp(log_vs_best / n);
                std::cout << min_us << ',' << cpc << ','
                          << std::exp(log_vs_default / n) << ','
                          << geomean_vs_best << ',' << worst << ','
                          << *worst_case << std::endl;

                if (best_cpc == 0 || geomean_vs_best < best_geomean)
                {
                    best_geomean = geomean_vs_best;
                    best_min_us = min_us;
                    best_cpc = cpc;
                }
                ++index;
            }
        }

        std::cout << "\nhpx.task_size_chunk_size.min_chunk_time_us="
                  << best_min_us
                  << "\nhpx.task_size_chunk_size.chunks_per_core=" << best_cpc
                  << std::endl;
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    using namespace std::chrono_literals;

    sweep_config const cfg{vm["max_work_us"].as<double>(),
        std::chrono::milliseconds(vm["time_per_case_ms"].as<int>()),
        vm["min_reps"].as<std::size_t>(), vm["max_reps"].as<std::size_t>()};

    std::cout << "size,count,min_us,cpc,median_us,min_time_us,reps"
              << std::endl;

    std::vector<case_result> results;
    sweep_size(cfg, "nano",
        hpx::nano_task([](std::uint64_t& x) { x = x * 3 + 1; }), results);
    sweep_size(cfg, "small",
        hpx::small_task([](std::uint64_t&) { spin(100ns); }), results);
    sweep_size(cfg, "medium",
        hpx::medium_task([](std::uint64_t&) { spin(10us); }), results);

    print_summary(results);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("max_work_us", value<double>()->default_value(10000.0),
            "longest loop to run, in microseconds of work in total "
            "(default: 10000)")
        ("time_per_case_ms", value<int>()->default_value(300),
            "time to repeat each loop for, in milliseconds (default: 300)")
        ("min_reps", value<std::size_t>()->default_value(5),
            "minimal number of repetitions of each loop (default: 5)")
        ("max_reps", value<std::size_t>()->default_value(300),
            "maximal number of repetitions of each loop (default: 300)")
        ;
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
