//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2015 Hartmut Kaiser
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
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
void test_copy_if_seq()
{
    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);
    std::iota(std::begin(c), middle, std::rand());
    std::fill(middle, std::end(c), -1);

    hpx::ranges::copy_if(c, std::begin(d), [](int i) { return !(i < 0); });

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + d.size() / 2,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy>
void test_copy_if(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);
    std::iota(std::begin(c), middle, std::rand());
    std::fill(middle, std::end(c), -1);

    hpx::ranges::copy_if(
        policy, c, std::begin(d), [](int i) { return !(i < 0); });

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + d.size() / 2,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy>
void test_copy_if_async(ExPolicy&& p)
{
    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);
    std::iota(std::begin(c), middle, std::rand());
    std::fill(middle, std::end(c), -1);

    auto f = hpx::ranges::copy_if(
        p, c, std::begin(d), [](int i) { return !(i < 0); });
    f.wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + d.size() / 2,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename Sentinel, typename... ExPolicy>
void test_copy_if_sent(ExPolicy&&... policy)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator =
        test::test_iterator<base_iterator, std::random_access_iterator_tag>;

    std::vector<int> c(10007);
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);
    std::iota(std::begin(c), middle, std::rand());
    std::fill(middle, std::end(c), -1);
    std::shuffle(std::begin(c), std::end(c), std::mt19937(std::rand()));

    auto pred = [](int i) { return !(i < 0); };

    std::vector<int> expected(c.size(), 0);
    auto expected_end =
        std::copy_if(std::begin(c), std::end(c), std::begin(expected), pred);

    std::vector<int> d(c.size(), 0);
    auto result = hpx::ranges::copy_if(policy..., iterator(std::begin(c)),
        Sentinel(iterator(std::end(c))), std::begin(d), pred);

    HPX_TEST(result.in == iterator(std::end(c)));
    HPX_TEST(result.out - std::begin(d) == expected_end - std::begin(expected));
    HPX_TEST(d == expected);
}

void test_copy_if()
{
    using namespace hpx::execution;

    test_copy_if_seq();

    test_copy_if(seq);
    test_copy_if(par);
    test_copy_if(par_unseq);

    test_copy_if_async(seq(task));
    test_copy_if_async(par(task));

    using iterator = test::test_iterator<std::vector<int>::iterator,
        std::random_access_iterator_tag>;
    test_copy_if_sent<test::sentinel_from_iterator<iterator>>();
    test_copy_if_sent<test::sized_sentinel_from_iterator<iterator>>(seq);
    test_copy_if_sent<test::sized_sentinel_from_iterator<iterator>>(par);
    test_copy_if_sent<test::sized_sentinel_from_iterator<iterator>>(par_unseq);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_copy_if();
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
