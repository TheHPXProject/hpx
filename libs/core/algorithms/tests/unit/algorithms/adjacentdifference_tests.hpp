//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2022 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_adjacent_difference(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<int> c = test::random_iota<int>(10007);
    std::vector<int> d(10007);
    std::vector<int> d_ans(10007);

    auto it = hpx::adjacent_difference(
        policy, std::begin(c), std::end(c), std::begin(d));
    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
        [](auto lhs, auto rhs) { return lhs == rhs; }));

    HPX_TEST(std::end(d) == it);
}

template <typename Policy, typename ExPolicy>
void test_adjacent_difference_direct(Policy l, ExPolicy policy)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<int> c = test::random_iota<int>(10007);
    std::vector<int> d(10007);
    std::vector<int> d_ans(10007);

    namespace ex = hpx::execution::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    auto it = hpx::adjacent_difference(
        policy.on(exec), std::begin(c), std::end(c), std::begin(d));

    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
        [](auto lhs, auto rhs) { return lhs == rhs; }));

    HPX_TEST(std::end(d) == it);
}

template <typename Policy, typename ExPolicy>
void test_adjacent_difference_sender(Policy l, ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<int> c = test::random_iota<int>(10007);
    std::vector<int> d(10007);
    std::vector<int> d_ans(10007);

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    {
        auto snd_result =
            tt::sync_wait(ex::just(std::begin(c), std::end(c), std::begin(d)) |
                hpx::adjacent_difference(policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

        HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
            [](auto lhs, auto rhs) { return lhs == rhs; }));
        HPX_TEST(std::end(d) == result);
    }

    {
        // edge case: empty range
        auto snd_result = tt::sync_wait(
            ex::just(std::begin(c), std::begin(c), std::begin(d)) |
            hpx::adjacent_difference(policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        std::adjacent_difference(
            std::begin(c), std::begin(c), std::begin(d_ans));

        HPX_TEST(std::begin(d) == result);
        HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
            [](auto lhs, auto rhs) { return lhs == rhs; }));
    }

    {
        // edge case: range of size one
        auto snd_result = tt::sync_wait(
            ex::just(std::begin(c), ++std::begin(c), std::begin(d)) |
            hpx::adjacent_difference(policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        std::adjacent_difference(
            std::begin(c), ++std::begin(c), std::begin(d_ans));

        HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
            [](auto lhs, auto rhs) { return lhs == rhs; }));
        HPX_TEST(++std::begin(d) == result);
    }
}

template <typename ExPolicy>
void test_adjacent_difference_async(ExPolicy&& p)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<int> c = test::random_iota<int>(10007);
    std::vector<int> d(10007);
    std::vector<int> d_ans(10007);

    auto f_it =
        hpx::adjacent_difference(p, std::begin(c), std::end(c), std::begin(d));
    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    f_it.wait();
    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
        [](auto lhs, auto rhs) { return lhs == rhs; }));

    HPX_TEST(std::end(d) == f_it.get());
}

template <typename Policy, typename ExPolicy>
void test_adjacent_difference_async_direct(Policy l, ExPolicy&& p)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<int> c = test::random_iota<int>(10007);
    std::vector<int> d(10007);
    std::vector<int> d_ans(10007);

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto result = tt::sync_wait(hpx::adjacent_difference(
        p.on(exec), std::begin(c), std::end(c), std::begin(d)));

    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans),
        [](auto lhs, auto rhs) { return lhs == rhs; }));

    HPX_TEST(std::end(d) == hpx::get<0>(*result));
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_adjacent_difference_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::vector<int> d(10007);

    bool caught_exception = false;
    try
    {
        hpx::adjacent_difference(policy, decorated_iterator(std::begin(c)),
            decorated_iterator(std::end(c)), std::begin(d),
            [](auto lhs, auto rhs) {
                throw std::runtime_error("test");
                return lhs - rhs;
            });
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_adjacent_difference_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::vector<int> d(10007);

    bool caught_exception = false;
    bool returned_from_algorithm = false;

    try
    {
        hpx::future<base_iterator> f = hpx::adjacent_difference(p,
            decorated_iterator(std::begin(c)), decorated_iterator(std::end(c)),
            std::begin(d), [](auto lhs, auto rhs) {
                throw std::runtime_error("test");
                return lhs - rhs;
            });

        returned_from_algorithm = true;

        f.get();

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_adjacent_difference_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::vector<int> d(10007);

    bool caught_bad_alloc = false;
    try
    {
        hpx::adjacent_difference(policy, decorated_iterator(std::begin(c)),
            decorated_iterator(std::end(c)), std::begin(d),
            [](auto lhs, auto rhs) {
                throw std::bad_alloc();
                return lhs - rhs;
            });
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
}

template <typename ExPolicy, typename IteratorTag>
void test_adjacent_difference_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::vector<int> d(10007);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;

    try
    {
        hpx::future<base_iterator> f = hpx::adjacent_difference(p,
            decorated_iterator(std::begin(c)), decorated_iterator(std::end(c)),
            std::begin(d), [](auto lhs, auto rhs) {
                throw std::bad_alloc();
                return lhs - rhs;
            });
        returned_from_algorithm = true;

        f.get();

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST(returned_from_algorithm);
}
