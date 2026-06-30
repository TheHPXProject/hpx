//  Copyright (c) 2014-2016 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

//////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, (std::numeric_limits<int>::max)());

struct smaller_than_50
{
    template <typename T>
    auto operator()(T const& x) const -> decltype(x < 50)
    {
        return x < 50;
    }
};

struct always_true
{
    template <typename T>
    bool operator()(T const&) const
    {
        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_count_if(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef std::vector<int>::difference_type diff_type;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(100007);
    std::iota(std::begin(c), std::begin(c) + 50, 0);
    std::iota(std::begin(c) + 50, std::end(c), dis(gen) + 50);

    diff_type num_items = hpx::count_if(
        iterator(std::begin(c)), iterator(std::end(c)), smaller_than_50());

    HPX_TEST_EQ(num_items, 50u);
}

template <typename ExPolicy, typename IteratorTag>
void test_count_if(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef std::vector<int>::difference_type diff_type;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(100007);
    std::iota(std::begin(c), std::begin(c) + 50, 0);
    std::iota(std::begin(c) + 50, std::end(c), dis(gen) + 50);

    diff_type num_items = hpx::count_if(policy, iterator(std::begin(c)),
        iterator(std::end(c)), smaller_than_50());

    HPX_TEST_EQ(num_items, 50u);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_count_if_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using diff_type = std::vector<int>::difference_type;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c(100007);
    std::iota(std::begin(c), std::begin(c) + 50, 0);
    std::iota(std::begin(c) + 50, std::end(c), dis(gen) + 50);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto snd_result =
        tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                          smaller_than_50()) |
            hpx::count_if(ex_policy.on(exec)));

    diff_type num_items = hpx::get<0>(*snd_result);

    HPX_TEST_EQ(num_items, 50u);
}

template <typename ExPolicy, typename IteratorTag>
void test_count_if_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef std::vector<int>::difference_type diff_type;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::begin(c) + 50, 0);
    std::iota(std::begin(c) + 50, std::end(c), dis(gen) + 50);

    hpx::future<diff_type> f = hpx::count_if(
        p, iterator(std::begin(c)), iterator(std::end(c)), smaller_than_50());

    HPX_TEST_EQ(f.get(), 50);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_count_if_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), dis(gen));

    bool caught_exception = false;
    try
    {
        // pred should never proc, so simple 'returns true'
        hpx::count_if(decorated_iterator(std::begin(c),
                          []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), always_true());
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<hpx::execution::sequenced_policy,
            IteratorTag>::call(hpx::execution::seq, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_count_if_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), dis(gen));

    bool caught_exception = false;
    try
    {
        // pred should never proc, so simple 'returns true'
        hpx::count_if(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), always_true());
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
void test_count_if_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef std::vector<int>::difference_type diff_type;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), 10);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<diff_type> f = hpx::count_if(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), always_true());
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

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_count_if_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), dis(gen));

    bool caught_bad_alloc = false;
    try
    {
        hpx::count_if(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), always_true());
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
}

template <typename ExPolicy, typename IteratorTag>
void test_count_if_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef std::vector<int>::difference_type diff_type;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), dis(gen));

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<diff_type> f = hpx::count_if(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), always_true());
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
