//  Copyright (c) 2014-2020 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_transform_reduce_binary(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c = test::random_iota<int>(1007);
    std::vector<int> d = test::random_iota<int>(1007);
    int init = std::rand() % 1007;    //-V101

    int r = hpx::transform_reduce(
        iterator(std::begin(c)), iterator(std::end(c)), std::begin(d), init);

    HPX_TEST_EQ(
        r, std::inner_product(std::begin(c), std::end(c), std::begin(d), init));
}

template <typename ExPolicy, typename IteratorTag>
void test_transform_reduce_binary(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c = test::random_iota<int>(1007);
    std::vector<int> d = test::random_iota<int>(1007);
    int init = std::rand() % 1007;    //-V101

    int r = hpx::transform_reduce(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), init);

    HPX_TEST_EQ(
        r, std::inner_product(std::begin(c), std::end(c), std::begin(d), init));
}

template <typename ExPolicy, typename IteratorTag>
void test_transform_reduce_binary_async(ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c = test::random_iota<int>(1007);
    std::vector<int> d = test::random_iota<int>(1007);
    int init = std::rand() % 1007;    //-V101

    hpx::future<int> fut_r = hpx::transform_reduce(
        p, iterator(std::begin(c)), iterator(std::end(c)), std::begin(d), init);

    fut_r.wait();
    HPX_TEST_EQ(fut_r.get(),
        std::inner_product(std::begin(c), std::end(c), std::begin(d), init));
}

////////////////////////////////////////////////////////////////////////////////

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_transform_reduce_binary_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<int> c = test::random_iota<int>(1007);
    std::vector<int> d = test::random_iota<int>(1007);
    int init = std::rand() % 1007;    //-V101

    {
        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(c)),
                              iterator(std::end(c)), std::begin(d), init) |
                hpx::transform_reduce(ex_policy.on(exec)));
        int result = hpx::get<0>(*snd_result);

        HPX_TEST_EQ(result,
            std::inner_product(
                std::begin(c), std::end(c), std::begin(d), init));
    }

    {
        // edge case: empty range

        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(c)),
                              iterator(std::begin(c)), std::begin(d), init) |
                hpx::transform_reduce(ex_policy.on(exec)));
        int result = hpx::get<0>(*snd_result);

        HPX_TEST_EQ(init, result);
        HPX_TEST_EQ(result,
            std::inner_product(
                std::begin(c), std::begin(c), std::begin(d), init));
    }
}
