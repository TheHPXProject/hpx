//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Causal tracing smoke test - exception set path.
//
// PURPOSE: Validate that set_exception() emits a RED producer-side causal
//          signal in Tracy when a shared state receives an exception.
//
// TRACY USAGE:
//   1. Start Tracy profiler GUI.
//   2. Run:  TRACY_NO_EXIT=1 ./bin/causal_exception_smoke --iterations=200
//   3. Connect Tracy to the process.
//   4. In the Tracy message log, filter by color 0xFF0000 (bright red).
//      You should see repeated:
//        "Future Exception Set: 0x<addr>"  [bright red]
//      immediately followed by consumer handling event.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/thread.hpp>

#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const iterations = vm["iterations"].as<std::size_t>();

    std::cout << "causal_exception_smoke: starting (" << iterations
              << " iterations)\n"
              << std::flush;

    std::size_t failures = 0;

    for (std::size_t i = 0; i < iterations; ++i)
    {
        hpx::tracing::frame_mark("smoke_exception_iteration");

        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        // Attach continuation *before* set_exception so it triggers on_completed_
        hpx::future<int> result = f.then([](hpx::future<int> fut) {
            try
            {
                return fut.get();
            }
            catch (std::runtime_error const&)
            {
                return -1;    // Handled exception
            }
        });

        // --- Producer side (exception) -----------------------------------
        // set_exception() emits:
        //   [red] "Future Exception Set: 0x<future_data*>"
        p.set_exception(
            std::make_exception_ptr(std::runtime_error("causal test error")));

        int const value = result.get();
        if (value != -1)
        {
            std::cerr << "FAIL at iteration " << i << ": expected -1, got "
                      << value << "\n";
            ++failures;
        }

        // Brief pause so each iteration is clearly visible in Tracy timeline
        hpx::this_thread::sleep_for(std::chrono::milliseconds(150));

        if (iterations >= 20 && i % (iterations / 10) == 0)
        {
            std::cout << "  iteration " << i << "/" << iterations << " OK\n"
                      << std::flush;
        }
    }

    if (failures == 0)
    {
        std::cout << "OK: causal_exception_smoke - all " << iterations
                  << " iterations passed\n";
    }
    else
    {
        std::cerr << "FAIL: " << failures << " failures\n";
    }

    hpx::local::finalize();
    return failures == 0 ? 0 : -1;
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("iterations",
        hpx::program_options::value<std::size_t>()->default_value(5),
        "Number of test iterations to execute (default: 5 for fast CI)");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = {"hpx.os_threads=2"};
    return hpx::local::init(hpx_main, argc, argv, init_args);
}
