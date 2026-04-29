//  Copyright (c) 2014-2020 Hartmut Kaiser
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

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce1(IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int val(42);
    auto op = [](auto v1, auto v2) { return v1 * v2; };

    int r1 =
        hpx::reduce(iterator(std::begin(c)), iterator(std::end(c)), val, op);

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), val, op);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int val(42);
    auto op = [](auto v1, auto v2) { return v1 * v2; };

    int r1 = hpx::reduce(
        policy, iterator(std::begin(c)), iterator(std::end(c)), val, op);

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), val, op);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce1_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int val(42);
    auto op = [](auto v1, auto v2) { return v1 * v2; };

    hpx::future<int> f =
        hpx::reduce(p, iterator(std::begin(c)), iterator(std::end(c)), val, op);
    f.wait();

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), val, op);
    HPX_TEST_EQ(f.get(), r2);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce2(IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int const val(42);
    int r1 = hpx::reduce(iterator(std::begin(c)), iterator(std::end(c)), val);

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), val);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int const val(42);
    int r1 = hpx::reduce(
        policy, iterator(std::begin(c)), iterator(std::end(c)), val);

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), val);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce2_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int const val(42);
    hpx::future<int> f =
        hpx::reduce(p, iterator(std::begin(c)), iterator(std::end(c)), val);
    f.wait();

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), val);
    HPX_TEST_EQ(f.get(), r2);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce3(IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int r1 = hpx::reduce(iterator(std::begin(c)), iterator(std::end(c)));

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), int(0));
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce3(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int r1 =
        hpx::reduce(policy, iterator(std::begin(c)), iterator(std::end(c)));

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), int(0));
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce3_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    hpx::future<int> f =
        hpx::reduce(p, iterator(std::begin(c)), iterator(std::end(c)));
    f.wait();

    // verify values
    int r2 = std::accumulate(std::begin(c), std::end(c), int(0));
    HPX_TEST_EQ(f.get(), r2);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_reduce_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    try
    {
        hpx::reduce(policy, iterator(std::begin(c)), iterator(std::end(c)),
            int(42), [](auto v1, auto v2) {
                return throw std::runtime_error("test"), v1 + v2;
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
void test_reduce_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::reduce(p, iterator(std::begin(c)),
            iterator(std::end(c)), int(42), [](auto v1, auto v2) {
                return throw std::runtime_error("test"), v1 + v2;
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

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_reduce_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    try
    {
        hpx::reduce(policy, iterator(std::begin(c)), iterator(std::end(c)),
            int(42),
            [](auto v1, auto v2) { return throw std::bad_alloc(), v1 + v2; });

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::reduce(p, iterator(std::begin(c)),
            iterator(std::end(c)), int(42),
            [](auto v1, auto v2) { return throw std::bad_alloc(), v1 + v2; });
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

////////////////////////////////////////////////////////////////////////////////

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_reduce_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    int val(42);
    auto op = [](auto v1, auto v2) { return v1 * v2; };

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)), val, op) |
            hpx::reduce(ex_policy.on(exec)));
        int result = hpx::get<0>(*snd_result);

        // verify values
        int expected = std::accumulate(std::begin(c), std::end(c), val, op);
        HPX_TEST_EQ(result, expected);
    }

    {
        auto snd_result = tt::sync_wait(ex::just(iterator(std::begin(c)),
                                            iterator(std::begin(c)), val, op) |
            hpx::reduce(ex_policy.on(exec)));
        int result = hpx::get<0>(*snd_result);

        HPX_TEST_EQ(result, val);
    }
}
