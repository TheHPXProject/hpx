//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Compares parallel_executor, fork_join_executor,
// suspending_fork_join_executor, and shared_suspending_fork_join_executor:
//
//  - region: the time of one bulk_sync_execute with a trivial body, for
//    regions that follow each other directly (latency for small sizes,
//    throughput for large ones);
//  - gap: the same, but the calling thread works alone for a while between
//    two regions, so that the waiting worker threads of the fork-join
//    executors spin, yield, or suspend;
//  - other work: the time of a parallel loop run through a parallel_executor
//    while a fork-join executor exists but is idle (its worker threads wait
//    for the next region).
//
// All times are the median of --batches batches.

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

///////////////////////////////////////////////////////////////////////////////
struct options
{
    std::size_t regions = 1000;
    std::size_t batches = 7;
    std::vector<std::size_t> sizes;
    std::vector<std::size_t> gaps_us;
    std::size_t other_work_size = 100000;
    std::size_t other_work_us = 1;
};

// Spin for the given time on the calling thread, standing in for serial work
// between two parallel regions.
void work_for(std::uint64_t const ns)
{
    hpx::chrono::high_resolution_timer const t;
    while (t.elapsed_nanoseconds() < static_cast<std::int64_t>(ns))
    {
    }
}

double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

void print(std::string const& executor, std::string const& test,
    std::size_t const size, std::size_t const gap_us, double const ns)
{
    std::cout << std::left << std::setw(40) << executor << std::setw(12) << test
              << std::right << std::setw(10) << size << std::setw(8) << gap_us
              << std::setw(14) << std::fixed << std::setprecision(0) << ns
              << "\n"
              << std::flush;
}

// The time of one region, in ns, excluding the gap between regions.
template <typename Executor>
double time_regions(Executor const& exec, options const& opts,
    std::size_t const size, std::size_t const gap_us)
{
    std::vector<std::size_t> data(size);
    auto body = [&](std::size_t const i) { data[i] += i; };
    hpx::util::counting_shape shape(std::size_t(0), size);

    // the regions themselves take some time, run fewer with a gap
    std::size_t const regions = gap_us == 0 ?
        opts.regions :
        (std::max) (opts.regions / 10, std::size_t(10));

    // warm up
    for (std::size_t r = 0; r != regions / 10 + 1; ++r)
    {
        hpx::parallel::execution::bulk_sync_execute(exec, body, shape);
    }

    std::vector<double> batches;
    for (std::size_t b = 0; b != opts.batches; ++b)
    {
        double elapsed = 0;
        for (std::size_t r = 0; r != regions; ++r)
        {
            if (gap_us != 0)
            {
                work_for(gap_us * 1000);
            }

            hpx::chrono::high_resolution_timer const t;
            hpx::parallel::execution::bulk_sync_execute(exec, body, shape);
            elapsed += static_cast<double>(t.elapsed_nanoseconds());
        }
        batches.push_back(elapsed / static_cast<double>(regions));
    }
    return median(batches);
}

// The time of a parallel loop through a parallel_executor, in ns, while exec
// is idle.
template <typename Executor>
double time_other_work(Executor const* exec, options const& opts)
{
    if (exec != nullptr)
    {
        // make sure the worker threads exist and wait for the next region
        hpx::parallel::execution::bulk_sync_execute(
            *exec, [](std::size_t) {},
            hpx::util::counting_shape(std::size_t(0), std::size_t(64)));
    }

    std::uint64_t const work_ns = opts.other_work_us * 1000;
    auto loop = [&] {
        hpx::experimental::for_loop(hpx::execution::par, std::size_t(0),
            opts.other_work_size, [&](std::size_t) { work_for(work_ns); });
    };

    loop();    // warm up

    std::vector<double> batches;
    for (std::size_t b = 0; b != opts.batches; ++b)
    {
        hpx::chrono::high_resolution_timer const t;
        loop();
        batches.push_back(static_cast<double>(t.elapsed_nanoseconds()));
    }
    return median(batches);
}

template <typename Executor>
void run_benchmarks(
    std::string const& name, Executor const& exec, options const& opts)
{
    for (std::size_t const size : opts.sizes)
    {
        print(name, "region", size, 0, time_regions(exec, opts, size, 0));
    }

    for (std::size_t const gap_us : opts.gaps_us)
    {
        for (std::size_t const size : opts.sizes)
        {
            print(name, "gap", size, gap_us,
                time_regions(exec, opts, size, gap_us));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    options opts;
    opts.regions = vm["regions"].as<std::size_t>();
    opts.batches = vm["batches"].as<std::size_t>();
    opts.sizes = vm["sizes"].as<std::vector<std::size_t>>();
    opts.gaps_us = vm["gaps-us"].as<std::vector<std::size_t>>();
    opts.other_work_size = vm["other-work-size"].as<std::size_t>();
    opts.other_work_us = vm["other-work-us"].as<std::size_t>();

    std::cout << "threads: " << hpx::get_num_worker_threads()
              << ", regions: " << opts.regions << ", batches: " << opts.batches
              << "\n";
    std::cout << std::left << std::setw(40) << "executor" << std::setw(12)
              << "test" << std::right << std::setw(10) << "size" << std::setw(8)
              << "gap_us" << std::setw(14) << "ns" << "\n";

    {
        hpx::execution::parallel_executor const exec;
        run_benchmarks("parallel_executor", exec, opts);
    }
    {
        ex::fork_join_executor const exec;
        run_benchmarks("fork_join_executor", exec, opts);
    }
    {
        ex::suspending_fork_join_executor const exec;
        run_benchmarks("suspending_fork_join_executor", exec, opts);
    }
    {
        ex::shared_suspending_fork_join_executor const exec;
        run_benchmarks("shared_suspending_fork_join_executor", exec, opts);
    }

    if (opts.other_work_size != 0)
    {
        print("none", "other_work", opts.other_work_size, 0,
            time_other_work<hpx::execution::parallel_executor>(nullptr, opts));
        {
            ex::fork_join_executor const exec;
            print("fork_join_executor", "other_work", opts.other_work_size, 0,
                time_other_work(&exec, opts));
        }
        {
            ex::suspending_fork_join_executor const exec;
            print("suspending_fork_join_executor", "other_work",
                opts.other_work_size, 0, time_other_work(&exec, opts));
        }
        {
            ex::shared_suspending_fork_join_executor const exec;
            print("shared_suspending_fork_join_executor", "other_work",
                opts.other_work_size, 0, time_other_work(&exec, opts));
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    // clang-format off
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()
        ("regions", value<std::size_t>()->default_value(1000),
            "number of parallel regions per batch")
        ("batches", value<std::size_t>()->default_value(7),
            "number of batches (the median is reported)")
        ("sizes", value<std::vector<std::size_t>>()->multitoken()
            ->default_value(std::vector<std::size_t>{0, 64, 10000, 1000000},
                "0 64 10000 1000000"),
            "sizes of the parallel regions")
        ("gaps-us", value<std::vector<std::size_t>>()->multitoken()
            ->default_value(std::vector<std::size_t>{10, 100, 1000},
                "10 100 1000"),
            "serial work between two regions in microseconds")
        ("other-work-size", value<std::size_t>()->default_value(100000),
            "iterations of the parallel loop run while an executor is idle "
            "(0: skip)")
        ("other-work-us", value<std::size_t>()->default_value(1),
            "time of one iteration of that loop in microseconds");
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
