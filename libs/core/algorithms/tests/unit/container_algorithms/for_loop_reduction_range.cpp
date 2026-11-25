//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename ExPolicy>
void test_for_loop_reduction_plus(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using iterator = std::vector<std::size_t>::iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t sum = 0;
    hpx::ranges::experimental::for_loop(std::forward<ExPolicy>(policy), c,
        hpx::experimental::reduction_plus(sum),
        [](iterator it, std::size_t& sum) { sum += *it; });

    // verify values
    std::size_t sum2 =
        std::accumulate(std::begin(c), std::end(c), std::size_t(0));
    HPX_TEST_EQ(sum, sum2);
}

template <typename ExPolicy>
void test_for_loop_reduction_multiplies(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using iterator = std::vector<std::size_t>::iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t prod = 0;
    hpx::ranges::experimental::for_loop(std::forward<ExPolicy>(policy), c,
        hpx::experimental::reduction_multiplies(prod),
        [](iterator it, std::size_t& prod) { prod *= *it; });

    // verify values
    std::size_t prod2 = std::accumulate(std::begin(c), std::end(c),
        std::size_t(1), std::multiplies<std::size_t>());
    HPX_TEST_EQ(prod, prod2);
}

template <typename ExPolicy>
void test_for_loop_reduction_min(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using iterator = std::vector<std::size_t>::iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t minval = c[0];

    hpx::ranges::experimental::for_loop(std::forward<ExPolicy>(policy), c,
        hpx::experimental::reduction_min(minval),
        [](iterator it, std::size_t& minval) {
            minval = (std::min) (minval, *it);
        });

    // verify values
    std::size_t minval2 = std::accumulate(std::begin(c), std::end(c), c[0],
        hpx::parallel::detail::min_of<std::size_t>());
    HPX_TEST_EQ(minval, minval2);
}

///////////////////////////////////////////////////////////////////////////////
void test_for_loop_reduction()
{
    test_for_loop_reduction_plus(hpx::execution::seq);
    test_for_loop_reduction_plus(hpx::execution::par);
    test_for_loop_reduction_plus(hpx::execution::par_unseq);

    test_for_loop_reduction_multiplies(hpx::execution::seq);
    test_for_loop_reduction_multiplies(hpx::execution::par);
    test_for_loop_reduction_multiplies(hpx::execution::par_unseq);

    test_for_loop_reduction_min(hpx::execution::seq);
    test_for_loop_reduction_min(hpx::execution::par);
    test_for_loop_reduction_min(hpx::execution::par_unseq);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    test_for_loop_reduction();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
