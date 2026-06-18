//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>

#include <string>
#include <vector>

#include "test_utils.hpp"

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_starts_with(IteratorTag)
{
    std::uniform_int_distribution<int> dis1(1, 10007);
    auto end1 = dis1(gen);
    std::uniform_int_distribution<int> dis2(1, end1);
    auto end2 = dis2(gen);
    auto some_ints = std::vector<int>(end1);
    std::iota(some_ints.begin(), some_ints.end(), 1);
    auto some_more_ints = std::vector<int>(end2);
    std::iota(some_more_ints.begin(), some_more_ints.end(), 1);
    auto some_wrong_ints = std::vector<int>(end2);
    std::iota(
        some_wrong_ints.begin(), some_wrong_ints.end(), std::rand() % end2 + 2);

    auto result1 = hpx::starts_with(std::begin(some_ints), std::end(some_ints),
        std::begin(some_more_ints), std::end(some_more_ints));
    HPX_TEST_EQ(result1, true);

    auto result2 = hpx::starts_with(std::begin(some_ints), std::end(some_ints),
        std::begin(some_wrong_ints), std::end(some_wrong_ints));
    HPX_TEST_EQ(result2, false);
}

template <typename ExPolicy, typename IteratorTag>
void test_starts_with(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::uniform_int_distribution<int> dis1(1, 10007);
    auto end1 = dis1(gen);
    std::uniform_int_distribution<int> dis2(1, end1);
    auto end2 = dis2(gen);
    auto some_ints = std::vector<int>(end1);
    std::iota(some_ints.begin(), some_ints.end(), 1);
    auto some_more_ints = std::vector<int>(end2);
    std::iota(some_more_ints.begin(), some_more_ints.end(), 1);
    auto some_wrong_ints = std::vector<int>(end2);
    std::iota(
        some_wrong_ints.begin(), some_wrong_ints.end(), std::rand() % end2 + 2);

    auto result1 =
        hpx::starts_with(policy, std::begin(some_ints), std::end(some_ints),
            std::begin(some_more_ints), std::end(some_more_ints));
    HPX_TEST_EQ(result1, true);

    auto result2 =
        hpx::starts_with(policy, std::begin(some_ints), std::end(some_ints),
            std::begin(some_wrong_ints), std::end(some_wrong_ints));
    HPX_TEST_EQ(result2, false);
}

template <typename ExPolicy, typename IteratorTag>
void test_starts_with_async(ExPolicy p, IteratorTag)
{
    std::uniform_int_distribution<int> dis1(1, 10007);
    auto end1 = dis1(gen);
    std::uniform_int_distribution<int> dis2(1, end1);
    auto end2 = dis2(gen);
    auto some_ints = std::vector<int>(end1);
    std::iota(some_ints.begin(), some_ints.end(), 1);
    auto some_more_ints = std::vector<int>(end2);
    std::iota(some_more_ints.begin(), some_more_ints.end(), 1);
    auto some_wrong_ints = std::vector<int>(end2);
    std::iota(
        some_wrong_ints.begin(), some_wrong_ints.end(), std::rand() % end2 + 2);

    hpx::future<bool> result1 =
        hpx::starts_with(p, std::begin(some_ints), std::end(some_ints),
            std::begin(some_more_ints), std::end(some_more_ints));
    result1.wait();
    HPX_TEST_EQ(result1.get(), true);

    hpx::future<bool> result2 =
        hpx::starts_with(p, std::begin(some_ints), std::end(some_ints),
            std::begin(some_wrong_ints), std::end(some_wrong_ints));
    result2.wait();
    HPX_TEST_EQ(result2.get(), false);
}

template <typename IteratorTag>
void test_starts_with()
{
    using namespace hpx::execution;

    test_starts_with(IteratorTag());
    test_starts_with(seq, IteratorTag());
    test_starts_with(par, IteratorTag());
    test_starts_with(par_unseq, IteratorTag());

    test_starts_with_async(seq(task), IteratorTag());
    test_starts_with_async(par(task), IteratorTag());
}

void starts_with_test()
{
    test_starts_with<std::random_access_iterator_tag>();
    test_starts_with<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    starts_with_test();
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
