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
void test_equal_binary1(IteratorTag)
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
            iterator(std::end(c1)), std::begin(c2), std::end(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        c1[dis(gen)] += 1;    //-V104
        bool result = hpx::equal(iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::end(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal_binary1(ExPolicy&& policy, IteratorTag)
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
            iterator(std::end(c1)), std::begin(c2), std::end(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        c1[dis(gen)] += 1;    //-V104
        bool result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2), std::end(c2));

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal_binary1_async(ExPolicy&& p, IteratorTag)
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
            iterator(std::end(c1)), std::begin(c2), std::end(c2));
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
            iterator(std::end(c1)), std::begin(c2), std::end(c2));
        result.wait();

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_equal_binary2(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        bool result =
            hpx::equal(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>());

        bool expected = std::equal(
            std::begin(c1), std::end(c1), std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104
        bool result =
            hpx::equal(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>());

        bool expected = std::equal(
            std::begin(c1), std::end(c1), std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal_binary2(ExPolicy&& policy, IteratorTag)
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
        bool result =
            hpx::equal(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>());

        bool expected = std::equal(
            std::begin(c1), std::end(c1), std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104
        bool result =
            hpx::equal(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>());

        bool expected = std::equal(
            std::begin(c1), std::end(c1), std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_equal_binary2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        hpx::future<bool> result =
            hpx::equal(p, iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>());
        result.wait();

        bool expected = std::equal(
            std::begin(c1), std::end(c1), std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        ++c1[dis(gen)];    //-V104

        hpx::future<bool> result =
            hpx::equal(p, iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>());
        result.wait();

        bool expected = std::equal(
            std::begin(c1), std::end(c1), std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(result.get(), expected);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_equal_binary_exception(IteratorTag)
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
            std::begin(c2), std::end(c2),
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
void test_equal_binary_exception(ExPolicy&& policy, IteratorTag)
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
            std::begin(c2), std::end(c2),
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
void test_equal_binary_exception_async(ExPolicy&& p, IteratorTag)
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
            iterator(std::end(c1)), std::begin(c2), std::end(c2),
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
void test_equal_binary_bad_alloc(ExPolicy&& policy, IteratorTag)
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
            std::begin(c2), std::end(c2),
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
void test_equal_binary_bad_alloc_async(ExPolicy&& p, IteratorTag)
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
            iterator(std::end(c1)), std::begin(c2), std::end(c2),
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

////////////////////////////////////////////////////////////////////////////////

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_equal_binary_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
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

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));
    auto policy = ex_policy.on(exec);

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2)) |
            hpx::equal(policy));
        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
        c1[dis(gen)] += 1;    //-V104

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2)) |
            hpx::equal(policy));
        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::equal(std::begin(c1), std::end(c1), std::begin(c2));

        // verify values
        HPX_TEST_EQ(result, expected);
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename ExPolicy, typename IteratorTag>
void test_equal_binary_edge_cases(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c1{0, 1, 2, 3, 4};
    std::vector<int> c2{5, 6, 7, 8, 9};

    {
        // both ranges empty
        auto result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::begin(c1)), std::begin(c2), std::begin(c2));
        HPX_TEST(hpx::unwrap(result));
    }

    {
        // only first range empty
        auto result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::begin(c1)), std::begin(c1), std::end(c1));
        HPX_TEST(!hpx::unwrap(result));
    }

    {
        // only second range empty
        auto result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c1), std::begin(c1));
        HPX_TEST(!hpx::unwrap(result));
    }

    {
        // ranges of different length
        auto result = hpx::equal(policy, iterator(std::begin(c1)),
            iterator(std::begin(c1) + 1), std::begin(c1), std::begin(c1) + 2);
        HPX_TEST(!hpx::unwrap(result));
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_equal_binary_edge_cases_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c1{0, 1, 2, 3, 4};
    std::vector<int> c2{5, 6, 7, 8, 9};

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));
    auto policy = ex_policy.on(exec);

    {
        // both ranges empty
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::begin(c1)),
                std::begin(c2), std::begin(c2)) |
            hpx::equal(policy));

        bool result = hpx::get<0>(*snd_result);

        HPX_TEST(result);
    }

    {
        // only first range empty
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::begin(c1)),
                std::begin(c1), std::end(c1)) |
            hpx::equal(policy));

        bool result = hpx::get<0>(*snd_result);

        HPX_TEST(!result);
    }

    {
        // only second range empty
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c1), std::begin(c1)) |
            hpx::equal(policy));

        bool result = hpx::get<0>(*snd_result);

        HPX_TEST(!result);
    }

    {
        // ranges of different length
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::begin(c1) + 1),
                std::begin(c1), std::begin(c1) + 2) |
            hpx::equal(policy));

        bool result = hpx::get<0>(*snd_result);

        HPX_TEST(!result);
    }
}
