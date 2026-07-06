//----------------------------------------------------------------------------
/// \file benchmark_nth_element.cpp
/// \brief Benchmark program of the nth_element function
///
//  Copyright (c) 2020 Francisco Jose Tapia (fjtapia@gmail.com )
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <version>

std::mt19937 my_rand(0);

int hpx_main(hpx::program_options::variables_map& vm)
{
    int test_count = vm["test_count"].as<int>();

    hpx::util::perftests_init(vm, "benchmark_nth_element");

    typedef std::less<std::uint64_t> compare_t;
    std::vector<std::uint64_t> A, B;
    std::uint32_t NELEM = 1000;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
        A.emplace_back(i);
    std::shuffle(A.begin(), A.end(), my_rand);

    std::uniform_int_distribution<std::uint64_t> i_dist(0, NELEM - 1);

    hpx::util::perftests_report(
        "hpx::nth_element, size: " + std::to_string(NELEM) +
            ", step: " + std::to_string(1),
        "seq", test_count * NELEM,
        [&] {
            hpx::nth_element(B.begin(), std::next(B.begin(), i_dist(my_rand)),
                B.end(), compare_t());
        },
        [&] { B = A; });

    NELEM = 100000;

    A.clear();
    B.clear();
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
        A.emplace_back(i);
    std::shuffle(A.begin(), A.end(), my_rand);
    uint32_t const STEP = NELEM / 20;

    hpx::util::perftests_report(
        "hpx::nth_element, size: " + std::to_string(NELEM) +
            ", step: " + std::to_string(STEP),
        "seq", test_count * NELEM,
        [&] {
            hpx::nth_element(B.begin(), std::next(B.begin(), i_dist(my_rand)),
                B.end(), compare_t());
        },
        [&] { B = A; });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("test_count",
        hpx::program_options::value<int>()->default_value(100),
        "number of tests to be averaged (default: 100)");

    hpx::util::perftests_cfg(desc_commandline);

    std::vector<std::string> cfg;
    cfg.push_back("hpx.os_threads=all");
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    // Initialize and run HPX.
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
