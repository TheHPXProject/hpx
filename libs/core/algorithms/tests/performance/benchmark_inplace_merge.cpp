///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = (unsigned int) std::random_device{}();
std::mt19937 _rand(seed);
///////////////////////////////////////////////////////////////////////////////

struct random_fill
{
    random_fill(std::size_t random_range)
      : gen(_rand())
      , dist(0, static_cast<int>(random_range - 1))
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void run_benchmark(std::size_t vector_left_size, std::size_t vector_right_size,
    int test_count, std::size_t random_range, IteratorTag)
{
    typedef test_container<IteratorTag> test_container;
    typedef typename test_container::type container;

    container c =
        test_container::get_container(vector_left_size + vector_right_size);
    container org_c;

    auto first = std::begin(c);
    auto middle = first + vector_left_size;
    auto last = std::end(c);

    // initialize data
    using namespace hpx::execution;
    hpx::generate(par, first, middle, random_fill(random_range));
    hpx::generate(par, middle, last, random_fill(random_range));
    hpx::sort(par, first, middle);
    hpx::sort(par, middle, last);
    org_c = c;

    auto org_first = std::begin(org_c);
    auto org_last = std::end(org_c);

    hpx::util::perftests_report("hpx::inplace_merge", "seq", test_count, [&] {
        hpx::copy(par, org_first, org_last, first);
        hpx::inplace_merge(seq, first, middle, last);
    });

    hpx::util::perftests_report("hpx::inplace_merge", "par", test_count, [&] {
        hpx::copy(par, org_first, org_last, first);
        hpx::inplace_merge(par, first, middle, last);
    });

    hpx::util::perftests_report(
        "hpx::inplace_merge", "par_unseq", test_count, [&] {
            hpx::copy(par, org_first, org_last, first);
            hpx::inplace_merge(par_unseq, first, middle, last);
        });

    hpx::util::perftests_print_times();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        _rand.seed(seed);
    }

    // pull values from cmd
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    double vector_ratio = vm["vector_ratio"].as<double>();
    std::size_t random_range = vm["random_range"].as<std::size_t>();
    int test_count = vm["test_count"].as<int>();

    [[maybe_unused]] std::size_t const os_threads = hpx::get_os_thread_count();

    hpx::util::perftests_init(vm, "benchmark_inplace_merge");

    if (random_range < 1)
        random_range = 1;

    std::size_t vector_left_size = static_cast<std::size_t>(
        static_cast<double>(vector_size) * vector_ratio);
    std::size_t vector_right_size = vector_size - vector_left_size;

    run_benchmark(vector_left_size, vector_right_size, test_count, random_range,
        std::random_access_iterator_tag());

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("vector_size",
            hpx::program_options::value<std::size_t>()->default_value(1000000),
            "sum of sizes of two vectors (default: 1000000)")
        ("vector_ratio",
            hpx::program_options::value<double>()->default_value(0.7),
            "ratio of two vector sizes (default: 0.7)")
        ("random_range",
            hpx::program_options::value<std::size_t>()->default_value(6),
            "range of random numbers [0, x) (default: 6)")
        ("test_count",
            hpx::program_options::value<int>()->default_value(100),
            "number of tests to be averaged (default: 100)")
        ("seed,s",
            hpx::program_options::value<unsigned int>(),
            "the random number generator seed to use for this run")
    ;
    // clang-format on

    // initialize program
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::util::perftests_cfg(desc_commandline);

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
