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
void test_iota_range(IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);

    // Test 1: Wrapping iterators inside a subrange
    auto res = hpx::ranges::iota(
        std::ranges::subrange(iterator(std::begin(c)), iterator(std::end(c))),
        10);
    HPX_TEST(res.out == iterator(std::end(c)));
    HPX_TEST_EQ(res.value, 10017);

    std::size_t count = 0;
    int expected = 10;
    std::for_each(
        std::begin(c), std::end(c), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c.size());

    // Test 2: Directly passing the container
    std::vector<int> c2(10007);
    auto res2 = hpx::ranges::iota(c2, 100);
    HPX_TEST(res2.out == std::end(c2));
    HPX_TEST_EQ(res2.value, 10107);

    count = 0;
    expected = 100;
    std::for_each(
        std::begin(c2), std::end(c2), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c2.size());
}

template <typename ExPolicy, typename IteratorTag>
    requires hpx::is_execution_policy_v<ExPolicy>
void test_iota_range(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);

    // Test 1: Wrapping iterators inside a subrange
    auto res = hpx::ranges::iota(policy,
        std::ranges::subrange(iterator(std::begin(c)), iterator(std::end(c))),
        10);
    HPX_TEST(res.out == iterator(std::end(c)));
    HPX_TEST_EQ(res.value, 10017);

    std::size_t count = 0;
    int expected = 10;
    std::for_each(
        std::begin(c), std::end(c), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c.size());

    // Test 2: Directly passing the container
    std::vector<int> c2(10007);
    auto res2 = hpx::ranges::iota(policy, c2, 100);
    HPX_TEST(res2.out == std::end(c2));
    HPX_TEST_EQ(res2.value, 10107);

    count = 0;
    expected = 100;
    std::for_each(
        std::begin(c2), std::end(c2), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c2.size());
}

template <typename ExPolicy, typename IteratorTag>
    requires hpx::is_execution_policy_v<ExPolicy>
void test_iota_range_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);

    // Test 1: Wrapping iterators inside a subrange
    auto f = hpx::ranges::iota(policy,
        std::ranges::subrange(iterator(std::begin(c)), iterator(std::end(c))),
        10);
    f.wait();

    std::size_t count = 0;
    int expected = 10;
    std::for_each(
        std::begin(c), std::end(c), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c.size());

    // Test 2: Directly passing the container
    std::vector<int> c2(10007);
    auto f2 = hpx::ranges::iota(policy, c2, 100);
    f2.wait();

    count = 0;
    expected = 100;
    std::for_each(
        std::begin(c2), std::end(c2), [&count, &expected](int v) -> void {
            HPX_TEST_EQ(v, expected++);
            ++count;
        });
    HPX_TEST_EQ(count, c2.size());
}

template <typename IteratorTag>
void test_iota_range()
{
    using namespace hpx::execution;
    test_iota_range(IteratorTag());
    test_iota_range(seq, IteratorTag());
    test_iota_range(par, IteratorTag());
    test_iota_range(par_unseq, IteratorTag());

    test_iota_range_async(seq(task), IteratorTag());
    test_iota_range_async(par(task), IteratorTag());
}

void iota_range_test()
{
    test_iota_range<std::random_access_iterator_tag>();
    test_iota_range<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    iota_range_test();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
