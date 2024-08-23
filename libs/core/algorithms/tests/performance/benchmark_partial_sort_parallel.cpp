//  Copyright (c) 2020 Francisco Jose Tapia (fjtapia@gmail.com )
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    int test_count = vm["test_count"].as<int>();

    hpx::util::perftests_init(vm, "benchmark_partial_sort_parallel");

    // test_main();
    // Test 1
    std::uint32_t NELEM = 1000;

    typedef std::less<std::uint64_t> compare_t;

    std::vector<std::uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
    }

    std::shuffle(A.begin(), A.end(), gen);
    hpx::util::perftests_report("hpx::partial_sort, size: " +
            std::to_string(NELEM) + ", step: " + std::to_string(1),
        "par", test_count, [&] {
            for (uint32_t i = 0; i < NELEM; ++i)
            {
                B = A;
                hpx::partial_sort(::hpx::execution::par, B.begin(),
                    B.begin() + i, B.end(), compare_t());
            }
        });

    // Test 2
    NELEM = 100000;

    A.clear();
    B.clear();
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
    }

    std::shuffle(A.begin(), A.end(), gen);
    hpx::util::perftests_report(
        "hpx::partial_sort, size: " + std::to_string(NELEM), "par", test_count,
        [&] {
            B = A;
            hpx::partial_sort(::hpx::execution::par, B.begin(), B.end(),
                B.end(), compare_t());
        });

    // Test 3
    std::shuffle(A.begin(), A.end(), gen);
    uint32_t STEP = NELEM / 100;
    hpx::util::perftests_report("hpx::partial_sort, size: " +
            std::to_string(NELEM) + ", step: " + std::to_string(STEP),
        "par", test_count, [&] {
            for (uint32_t i = 0; i < NELEM; i += STEP)
            {
                B = A;
                hpx::partial_sort(::hpx::execution::par, B.begin(),
                    B.begin() + i, B.end(), compare_t());
            }
        });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run")("test_count",
        value<int>()->default_value(10),
        "number of tests to be averaged (default: 10)");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::util::perftests_cfg(desc_commandline);

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
