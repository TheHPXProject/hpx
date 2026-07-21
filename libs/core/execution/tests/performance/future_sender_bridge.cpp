//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Benchmark: HPX future to sender bridge overhead.
//
// This measures the cost of crossing from hpx::future<T> into P2300 senders,
// including the optimized scheduler-aware and pipe-form continues_on paths.
//
// Usage:
//   ./future_sender_bridge_test --iterations=10000 --repetitions=5 --hpx:threads=4

#include <hpx/assert.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

std::uint64_t iterations = 1000;
std::uint64_t warmup_iterations = 1000;
std::uint64_t repetitions = 3;
std::uint64_t volatile sink = 0;

struct benchmark_result
{
    char const* label;
    double seconds;
    double ns_per_iteration;
};

template <typename F>
void run_iterations(std::uint64_t count, F&& f)
{
    for (std::uint64_t i = 0; i < count; ++i)
    {
        f(i);
    }
}

template <typename F>
benchmark_result measure(char const* label, F&& f)
{
    if (warmup_iterations != 0)
    {
        run_iterations(warmup_iterations, f);
    }

    double total = 0.0;
    for (std::uint64_t rep = 0; rep < repetitions; ++rep)
    {
        hpx::chrono::high_resolution_timer timer;
        run_iterations(iterations, f);
        total += timer.elapsed();
    }

    double const average = total / static_cast<double>(repetitions);
    return benchmark_result{
        label, average, average * 1e9 / static_cast<double>(iterations)};
}

void print_result(benchmark_result const& result)
{
    std::cout << std::left << std::setw(46) << result.label << std::right
              << std::setw(14) << std::setprecision(8) << result.seconds << " s"
              << std::setw(14) << std::fixed << std::setprecision(2)
              << result.ns_per_iteration << " ns/iter\n";
}

template <typename Sender>
std::uint64_t sync_wait_value(Sender&& sender)
{
    auto result = tt::sync_wait(HPX_FORWARD(Sender, sender));
    HPX_ASSERT(result.has_value());
    return static_cast<std::uint64_t>(std::get<0>(*result));
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    iterations = vm["iterations"].as<std::uint64_t>();
    warmup_iterations = vm["warmup-iterations"].as<std::uint64_t>();
    repetitions = vm["repetitions"].as<std::uint64_t>();

    if (iterations == 0 || repetitions == 0)
    {
        std::cerr << "iterations and repetitions must be non-zero\n";
        hpx::local::finalize();
        return -1;
    }

    ex::thread_pool_scheduler sched{};

    std::cout << "\n-------------- Benchmark Config --------------\n"
              << "iterations        : " << iterations << "\n"
              << "warmup_iterations : " << warmup_iterations << "\n"
              << "repetitions       : " << repetitions << "\n"
              << "os threads        : " << hpx::get_num_worker_threads() << "\n"
              << "----------------------------------------------\n";

    std::vector<benchmark_result> results;
    results.reserve(4);

    results.push_back(
        measure("baseline: ready_future.get()", [](std::uint64_t i) {
            auto f = hpx::make_ready_future<std::uint64_t>(i);
            sink += f.get();
        }));

    results.push_back(measure("as_sender(future)", [](std::uint64_t i) {
        auto sender = ex::as_sender(hpx::make_ready_future<std::uint64_t>(i));
        sink += sync_wait_value(HPX_MOVE(sender));
    }));

    results.push_back(
        measure("as_sender(future, scheduler)", [&](std::uint64_t i) {
            auto sender =
                ex::as_sender(hpx::make_ready_future<std::uint64_t>(i), sched);
            sink += sync_wait_value(HPX_MOVE(sender));
        }));

    results.push_back(measure(
        "as_sender(future) | continues_on(scheduler)", [&](std::uint64_t i) {
            auto sender =
                ex::as_sender(hpx::make_ready_future<std::uint64_t>(i)) |
                ex::continues_on(sched);
            sink += sync_wait_value(HPX_MOVE(sender));
        }));

    std::cout << "\n-------------- Future Sender Bridge --------------\n";
    for (benchmark_result const& result : results)
    {
        print_result(result);
    }

    double const baseline = results.front().ns_per_iteration;
    std::cout << "\n-------------- Relative to baseline --------------\n";
    for (benchmark_result const& result : results)
    {
        std::cout << std::left << std::setw(46) << result.label << std::right
                  << std::setw(10) << std::fixed << std::setprecision(2)
                  << (result.ns_per_iteration / baseline) << "x\n";
    }
    std::cout << "sink: " << sink << "\n";

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("iterations", value<std::uint64_t>()->default_value(1000),
            "number of benchmark iterations per repetition")
        ("warmup-iterations", value<std::uint64_t>()->default_value(1000),
            "number of warmup iterations")
        ("repetitions", value<std::uint64_t>()->default_value(3),
            "number of benchmark repetitions");
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
