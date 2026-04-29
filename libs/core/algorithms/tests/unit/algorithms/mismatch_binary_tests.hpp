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
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, 10006);

template <typename IteratorTag>
void test_mismatch_binary1(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::mismatch(begin1, end1, std::begin(c2), std::end(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::mismatch(begin1, end1, std::begin(c2), std::end(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary1(ExPolicy&& policy, IteratorTag)
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

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result =
            hpx::mismatch(policy, begin1, end1, std::begin(c2), std::end(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result =
            hpx::mismatch(policy, begin1, end1, std::begin(c2), std::end(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto f = hpx::mismatch(p, begin1, end1, std::begin(c2), std::end(c2));
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto f = hpx::mismatch(p, begin1, end1, std::begin(c2), std::end(c2));
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_mismatch_binary2(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::mismatch(
            begin1, end1, std::begin(c2), std::end(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::mismatch(
            begin1, end1, std::begin(c2), std::end(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary2(ExPolicy&& policy, IteratorTag)
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

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::mismatch(policy, begin1, end1, std::begin(c2),
            std::end(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::mismatch(policy, begin1, end1, std::begin(c2),
            std::end(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto f = hpx::mismatch(
            p, begin1, end1, std::begin(c2), std::end(c2), std::equal_to<>());
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto f = hpx::mismatch(
            p, begin1, end1, std::begin(c2), std::end(c2), std::equal_to<>());
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_mismatch_binary_exception(IteratorTag)
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
        hpx::mismatch(iterator(std::begin(c1)), iterator(std::end(c1)),
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
void test_mismatch_binary_exception(ExPolicy&& policy, IteratorTag)
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
        hpx::mismatch(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
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
void test_mismatch_binary_exception_async(ExPolicy&& p, IteratorTag)
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
        auto f = hpx::mismatch(p, iterator(std::begin(c1)),
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

/////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary_bad_alloc(ExPolicy&& policy, IteratorTag)
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
        hpx::mismatch(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
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
void test_mismatch_binary_bad_alloc_async(ExPolicy&& p, IteratorTag)
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
        auto f = hpx::mismatch(p, iterator(std::begin(c1)),
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
void test_mismatch_binary1_sender(
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

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto snd_result =
            tt::sync_wait(ex::just(begin1, end1, std::begin(c2), std::end(c2)) |
                hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto snd_result =
            tt::sync_wait(ex::just(begin1, end1, std::begin(c2), std::end(c2)) |
                hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_mismatch_binary2_sender(
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

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto snd_result = tt::sync_wait(ex::just(begin1, end1, std::begin(c2),
                                            std::end(c2), std::equal_to<>()) |
            hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto snd_result = tt::sync_wait(ex::just(begin1, end1, std::begin(c2),
                                            std::end(c2), std::equal_to<>()) |
            hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }

    {
        // edge case: empty range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::begin(c1)),
                std::begin(c2), std::end(c2), std::equal_to<>()) |
            hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST(result.first.base() == std::begin(c1));
        HPX_TEST(result.second == std::begin(c2));
    }
}
