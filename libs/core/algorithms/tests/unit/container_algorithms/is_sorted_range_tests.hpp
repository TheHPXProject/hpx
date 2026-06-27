//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2015 Daniel Bourgeois
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

using identity = hpx::identity;

struct negate
{
    int operator()(int x)
    {
        return -x;
    }
};

template <typename ExPolicy, typename IteratorTag>
void test_sorted1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    //Fill with sorted values from 0 to 10006
    std::iota(std::begin(c), std::end(c), 0);

    HPX_TEST(hpx::ranges::is_sorted(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::less<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::greater<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::less<int>(), negate()));
    HPX_TEST(hpx::ranges::is_sorted(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::greater<int>(), negate()));

    using element = std::pair<int, int>;
    std::vector<element> sorted_c = {{10, 1}, {20, 3}, {30, 5}, {40, 8}};
    auto proj = [](element const& p) { return p.second; };

    HPX_TEST(hpx::ranges::is_sorted(
        policy, sorted_c.begin(), sorted_c.end(), std::less<int>{}, proj));
}

template <typename ExPolicy, typename IteratorTag>
void test_sorted1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    //Fill with sorted values from 0 to 10006
    std::iota(std::begin(c), std::end(c), 0);

    hpx::future<bool> f1 = hpx::ranges::is_sorted(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::less<int>(), identity());
    f1.wait();
    HPX_TEST(f1.get());
    hpx::future<bool> f2 = hpx::ranges::is_sorted(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::greater<int>(), identity());
    f2.wait();
    HPX_TEST(!f2.get());
    hpx::future<bool> f3 = hpx::ranges::is_sorted(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::less<int>(), negate());
    f3.wait();
    HPX_TEST(!f3.get());
    hpx::future<bool> f4 = hpx::ranges::is_sorted(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::greater<int>(), negate());
    f4.wait();
    HPX_TEST(f4.get());
}

template <typename IteratorTag>
void test_sorted1_seq(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    //Fill with sorted values from 0 to 10006
    std::iota(std::begin(c), std::end(c), 0);

    HPX_TEST(hpx::ranges::is_sorted(iterator(std::begin(c)),
        iterator(std::end(c)), std::less<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(iterator(std::begin(c)),
        iterator(std::end(c)), std::greater<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(iterator(std::begin(c)),
        iterator(std::end(c)), std::less<int>(), negate()));
    HPX_TEST(hpx::ranges::is_sorted(iterator(std::begin(c)),
        iterator(std::end(c)), std::greater<int>(), negate()));
}

template <typename ExPolicy>
void test_sorted1(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<int> c(10007);
    //Fill with sorted values from 0 to 10006
    std::iota(std::begin(c), std::end(c), 0);

    HPX_TEST(hpx::ranges::is_sorted(policy, c, std::less<int>(), identity()));
    HPX_TEST(
        !hpx::ranges::is_sorted(policy, c, std::greater<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(policy, c, std::less<int>(), negate()));
    HPX_TEST(hpx::ranges::is_sorted(policy, c, std::greater<int>(), negate()));
}

template <typename ExPolicy>
void test_sorted1_async(ExPolicy&& p)
{
    std::vector<int> c(10007);
    //Fill with sorted values from 0 to 10006
    std::iota(std::begin(c), std::end(c), 0);

    hpx::future<bool> f1 =
        hpx::ranges::is_sorted(p, c, std::less<int>(), identity());
    f1.wait();
    HPX_TEST(f1.get());
    hpx::future<bool> f2 =
        hpx::ranges::is_sorted(p, c, std::greater<int>(), identity());
    f2.wait();
    HPX_TEST(!f2.get());
    hpx::future<bool> f3 =
        hpx::ranges::is_sorted(p, c, std::less<int>(), negate());
    f3.wait();
    HPX_TEST(!f3.get());
    hpx::future<bool> f4 =
        hpx::ranges::is_sorted(p, c, std::greater<int>(), negate());
    f4.wait();
    HPX_TEST(f4.get());
}

void test_sorted1_seq()
{
    std::vector<int> c(10007);
    //Fill with sorted values from 0 to 10006
    std::iota(std::begin(c), std::end(c), 0);

    HPX_TEST(hpx::ranges::is_sorted(c, std::less<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(c, std::greater<int>(), identity()));
    HPX_TEST(!hpx::ranges::is_sorted(c, std::less<int>(), negate()));
    HPX_TEST(hpx::ranges::is_sorted(c, std::greater<int>(), negate()));
}
