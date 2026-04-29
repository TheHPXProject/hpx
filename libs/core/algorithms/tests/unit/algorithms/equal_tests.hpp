//  Copyright (c) 2014-2025 Hartmut Kaiser
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

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename IteratorTag>
void test_equal1(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        bool result = hpx::equal(
            iterator(std::begin(c1)), iterator(std::end(c1)), std::begin(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104
        bool result = hpx::equal(
            iterator(std::begin(c1)), iterator(std::end(c1)), std::begin(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        bool result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104
        bool result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_equal1_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {
        auto snd_result = tt::sync_wait(
            ex::just(std::begin(c1), std::end(c1), std::begin(c2)) |
            hpx::equal(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104

        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(c1)),
                              iterator(std::end(c1)), std::begin(c2)) |
                hpx::equal(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        // edge case: empty range

        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(c1)),
                              iterator(std::begin(c1)), std::begin(c2)) |
                hpx::equal(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        HPX_TEST(result);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        hpx::future<bool> result = hpx::equal(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2));
        result.wait();

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104

        hpx::future<bool> result = hpx::equal(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2));
        result.wait();

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_equal2(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        bool result = hpx::equal(iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::equal_to<>());

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104
        bool result = hpx::equal(iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::equal_to<>());

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        bool result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::equal_to<>());

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104
        bool result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::equal_to<>());

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_equal2_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::equal_to<>()) |
            hpx::equal(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::equal_to<>()) |
            hpx::equal(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        // edge case: empty range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::begin(c1)),
                std::begin(c2), std::equal_to<>()) |
            hpx::equal(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        HPX_TEST(result);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        hpx::future<bool> result = hpx::equal(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::equal_to<>());
        result.wait();

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104

        hpx::future<bool> result = hpx::equal(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::equal_to<>());
        result.wait();

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_equal_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    try
    {
        hpx::equal(iterator(std::begin(c1)), iterator(std::end(c1)),
            std::begin(c2),
            [](auto, auto) { return throw std::runtime_error("test"), true; });

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
void test_equal_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    try
    {
        hpx::equal(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
            std::begin(c2),
            [](auto, auto) { return throw std::runtime_error("test"), true; });

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
void test_equal_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> f = hpx::equal(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2),
            [](auto, auto) { return throw std::runtime_error("test"), true; });
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
void test_equal_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_bad_alloc = false;
    try
    {
        hpx::equal(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
            std::begin(c2),
            [](auto, auto) { return throw std::bad_alloc(), true; });

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
void test_equal_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> f = hpx::equal(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2),
            [](auto, auto) { return throw std::bad_alloc(), true; });
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
