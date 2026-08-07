//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Causal tracing unit test - exception set path.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
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

    std::cout << "causal_exception: starting (" << iterations
              << " iterations)\n"
              << std::flush;

    for (std::size_t i = 0; i < iterations; ++i)
    {
        hpx::tracing::frame_mark("smoke_exception_iteration");

        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        hpx::future<int> result = f.then([](hpx::future<int> fut) {
            try
            {
                return fut.get();
            }
            catch (std::runtime_error const&)
            {
                return -1;
            }
        });

        p.set_exception(
            std::make_exception_ptr(std::runtime_error("causal test error")));

        int const value = result.get();
        HPX_TEST_EQ(value, -1);

        hpx::this_thread::sleep_for(std::chrono::milliseconds(150));

        if (iterations >= 20 && i % (iterations / 10) == 0)
        {
            std::cout << "  iteration " << i << "/" << iterations << " OK\n"
                      << std::flush;
        }
    }

    std::cout << "causal_exception completed (" << iterations
              << " iterations)\n";

    return hpx::local::finalize();
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

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
