//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Work-stealing smoke test.
//
// PURPOSE: Validate that task stealing between worker cores emits
//          "Work Stolen: Thief W#X <- Victim W#Y" point messages in Tracy.
//
// TRACY USAGE:
//   1. Start Tracy profiler GUI.
//   2. Run: TRACY_NO_EXIT=1 ./bin/work_stealing_smoke --iterations=100
//   3. Connect Tracy to the process.
//   4. In the Tracy message log, search for "Work Stolen" or color 0xFFC107.
//      You will see thief-victim core mappings and task pointers.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/thread.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

std::size_t fibonacci(std::size_t n)
{
    if (n <= 1)
        return n;

    auto fib_fn = hpx::annotated_function(&fibonacci, "fibonacci");
    hpx::future<std::size_t> f1 = hpx::async(fib_fn, n - 1);
    hpx::future<std::size_t> f2 = hpx::async(fib_fn, n - 2);

    return f1.get() + f2.get();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const iterations = vm["iterations"].as<std::size_t>();
    std::size_t const depth = vm["depth"].as<std::size_t>();

    std::cout << "work_stealing_smoke: starting (" << iterations
              << " iterations, fib depth " << depth << ")\n"
              << std::flush;

    auto fib_fn = hpx::annotated_function(&fibonacci, "fibonacci");

    for (std::size_t i = 0; i < iterations; ++i)
    {
        hpx::tracing::frame_mark("work_stealing_iteration");

        // Spawn unbalanced tasks across threads to trigger active work stealing
        std::vector<hpx::future<std::size_t>> futures;
        futures.reserve(8);

        for (std::size_t d = 0; d < 8; ++d)
        {
            futures.push_back(hpx::async(fib_fn, depth + (d % 3)));
        }

        hpx::wait_all(futures);

        // Brief pause so each iteration is clearly visible in Tracy timeline
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (iterations >= 10 && i % (iterations / 5) == 0)
        {
            std::cout << "  iteration " << i << "/" << iterations << " OK\n"
                      << std::flush;
        }
    }

    std::cout << "work_stealing_smoke: completed successfully!\n" << std::flush;

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;
    po::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("iterations",
        po::value<std::size_t>()->default_value(5),
        "Number of iterations to run")("depth",
        po::value<std::size_t>()->default_value(8),
        "Fibonacci recursion depth");

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(hpx_main, argc, argv, init_args);
}
