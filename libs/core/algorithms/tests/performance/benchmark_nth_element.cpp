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
#include <iostream>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>
#include <version>

std::mt19937 my_rand(0);

int hpx_main(hpx::program_options::variables_map& vm)
{
    hpx::util::perftests_init(vm, "benchmark_nth_element");

    typedef std::less<uint64_t> compare_t;
    std::vector<uint64_t> A, B;
    uint32_t const NELEM = 1000;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (uint64_t i = 0; i < NELEM; ++i)
        A.emplace_back(i);
    std::shuffle(A.begin(), A.end(), my_rand);

    hpx::util::perftests_report("hpx::nth_element, size: " +
            std::to_string(NELEM) + ", step: " + std::to_string(1),
        "seq", 100, [&] {
            for (uint64_t i = 0; i < NELEM; i++)
            {
                B = A;
                hpx::nth_element(
                    B.begin(), B.begin() + i, B.end(), compare_t());
            }
        });

    NELEM = 100000;

    A.clear();
    B.clear();
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (uint64_t i = 0; i < NELEM; ++i)
        A.emplace_back(i);
    std::shuffle(A.begin(), A.end(), my_rand);
    uint32_t const STEP = NELEM / 20;

    hpx::util::perftests_report("hpx::nth_element, size: " +
            std::to_string(NELEM) + ", step: " + std::to_string(STEP),
        "seq", 100, [&] {
            for (uint64_t i = 0; i < NELEM; i += STEP)
            {
                B = A;
                hpx::nth_element(
                    B.begin(), B.begin() + i, B.end(), compare_t());
            }
        });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

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
