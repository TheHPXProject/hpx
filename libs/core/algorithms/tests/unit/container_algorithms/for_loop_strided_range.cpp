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
std::uniform_int_distribution<> dis(1, 10006);

template <typename ExPolicy>
void test_for_loop_strided(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using iterator = std::vector<std::size_t>::iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::for_each(std::begin(c), std::end(c), [](std::size_t& v) -> void {
        if (v == 42)
            v = 43;
    });

    int stride = dis(gen);    //-V103

    hpx::ranges::experimental::for_loop_strided(std::forward<ExPolicy>(policy),
        c, stride, [](iterator it) { *it = 42; });

    // verify values
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i % stride == 0)    //-V104
        {
            HPX_TEST_EQ(c[i], std::size_t(42));
        }
        else
        {
            HPX_TEST_NEQ(c[i], std::size_t(42));
        }
        ++count;
    }
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy>
void test_for_loop_strided_async(ExPolicy&& p)
{
    using iterator = std::vector<std::size_t>::iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::for_each(std::begin(c), std::end(c), [](std::size_t& v) -> void {
        if (v == 42)
            v = 43;
    });

    int stride = dis(gen);    //-V103

    auto f = hpx::ranges::experimental::for_loop_strided(
        std::forward<ExPolicy>(p), c, stride, [](iterator it) { *it = 42; });
    f.wait();

    // verify values
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i % stride == 0)    //-V104
        {
            HPX_TEST_EQ(c[i], std::size_t(42));
        }
        else
        {
            HPX_TEST_NEQ(c[i], std::size_t(42));
        }
        ++count;
    }
    HPX_TEST_EQ(count, c.size());
}

void test_for_loop_strided()
{
    test_for_loop_strided(hpx::execution::seq);
    test_for_loop_strided(hpx::execution::par);
    test_for_loop_strided(hpx::execution::par_unseq);

    test_for_loop_strided_async(hpx::execution::seq(hpx::execution::task));
    test_for_loop_strided_async(hpx::execution::par(hpx::execution::task));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    test_for_loop_strided();

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
