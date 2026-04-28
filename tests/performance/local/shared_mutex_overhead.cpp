//  (C) Copyright 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/synchronization/shared_mutex.hpp>

#include <cstdint>
#include <iostream>
#include <vector>

std::uint64_t num_iterations = 100000;
std::uint64_t reader_threads = 4;

hpx::shared_mutex mtx;

void reader()
{
    for (std::uint64_t i = 0; i < num_iterations; ++i)
    {
        std::shared_lock<hpx::shared_mutex> l(mtx);
    }
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    num_iterations = vm["iterations"].as<std::uint64_t>();
    reader_threads = hpx::get_num_worker_threads();

    std::cout << "Starting benchmark with " << reader_threads << " threads..."
              << std::endl;

    std::vector<hpx::future<void>> futures;
    futures.reserve(reader_threads);

    hpx::chrono::high_resolution_timer walltime;

    for (std::uint64_t i = 0; i < reader_threads; ++i)
    {
        futures.push_back(hpx::async(&reader));
    }

    hpx::wait_all(futures);

    double const duration = walltime.elapsed();

    std::cout << "Total time: " << duration << " seconds" << std::endl;
    std::cout << "Average time per reader thread: " << duration / reader_threads
              << " seconds" << std::endl;

    hpx::util::print_cdash_timing("SharedMutexOverhead", duration);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description cmdline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("iterations",
        hpx::program_options::value<std::uint64_t>()->default_value(100000),
        "number of iterations per thread");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
