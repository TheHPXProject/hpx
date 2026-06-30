//  Copyright (c) 2026 Anfsity
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt\)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_iota(IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    hpx::iota(iterator(std::begin(c)), iterator(std::end(c)), 10);

    // verify values
    std::size_t count = 0;
    int expected = 10;
    std::for_each(
        std::begin(c), std::end(c), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy, typename IteratorTag>
    requires(hpx::is_execution_policy_v<ExPolicy>)
void test_iota(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    hpx::iota(policy, iterator(std::begin(c)), iterator(std::end(c)), 10);

    // verify values
    std::size_t count = 0;
    int expected = 10;
    std::for_each(
        std::begin(c), std::end(c), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy, typename IteratorTag>
    requires(hpx::is_execution_policy_v<ExPolicy>)
void test_iota_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    auto f =
        hpx::iota(policy, iterator(std::begin(c)), iterator(std::end(c)), 10);
    f.wait();

    // verify values
    std::size_t count = 0;
    int expected = 10;
    std::for_each(
        std::begin(c), std::end(c), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c.size());
}

template <typename IteratorTag>
void test_iota()
{
    using namespace hpx::execution;
    test_iota(IteratorTag());
    test_iota(seq, IteratorTag());
    test_iota(par, IteratorTag());
    test_iota(par_unseq, IteratorTag());

    test_iota_async(seq(task), IteratorTag());
    test_iota_async(par(task), IteratorTag());
}

void iota_test()
{
    test_iota<std::random_access_iterator_tag>();
    test_iota<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    iota_test();
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
