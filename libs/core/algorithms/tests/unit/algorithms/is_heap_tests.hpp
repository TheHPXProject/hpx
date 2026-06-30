//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2020 Hartmut Kaiser
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
std::uniform_int_distribution<> dis(0, 10006);

struct throw_always
{
    template <typename T1, typename T2>
    bool operator()(T1 const&, T2 const&) const
    {
        throw std::runtime_error("test");
    }
};

struct throw_bad_alloc
{
    template <typename T1, typename T2>
    bool operator()(T1 const&, T2 const&) const
    {
        throw std::bad_alloc();
    }
};

struct user_defined_type
{
    user_defined_type() = default;
    user_defined_type(int rand_no)
      : val(rand_no)
    {
    }

    bool operator<(user_defined_type const& t) const
    {
        if (this->name < t.name)
            return true;
        else if (this->name > t.name)
            return false;
        else
            return this->val < t.val;
    }

    user_defined_type const& operator++()
    {
        static std::vector<std::string> const name_list = {
            "ABB", "ABC", "ACB", "BCA", "CAA", "CAAA", "CAAB"};
        std::uniform_int_distribution<> dist(
            0, static_cast<int>(name_list.size() - 1));
        name = name_list[dist(gen)];
        ++val;
        return *this;
    }

    std::string name;
    int val;
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType>
void test_is_heap(IteratorTag, DataType, bool test_for_is_heap = true)
{
    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<DataType> c(10007);
    std::iota(std::begin(c), std::end(c), DataType(gen()));

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    if (test_for_is_heap)
    {
        bool result =
            hpx::is_heap(iterator(std::begin(c)), iterator(std::end(c)));
        bool solution = std::is_heap(std::begin(c), std::end(c));

        HPX_TEST_EQ(result, solution);
    }
    else
    {
        iterator result =
            hpx::is_heap_until(iterator(std::begin(c)), iterator(std::end(c)));
        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result.base() == solution);
    }
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_is_heap(
    ExPolicy&& policy, IteratorTag, DataType, bool test_for_is_heap = true)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<DataType> c(10007);
    std::iota(std::begin(c), std::end(c), DataType(gen()));

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    if (test_for_is_heap)
    {
        bool result = hpx::is_heap(
            policy, iterator(std::begin(c)), iterator(std::end(c)));
        bool solution = std::is_heap(std::begin(c), std::end(c));

        HPX_TEST_EQ(result, solution);
    }
    else
    {
        iterator result = hpx::is_heap_until(
            policy, iterator(std::begin(c)), iterator(std::end(c)));
        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result.base() == solution);
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_is_heap_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::size_t(gen()));

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c))) |
            hpx::is_heap(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool solution = std::is_heap(std::begin(c), std::end(c));

        HPX_TEST_EQ(result, solution);
    }

    {
        // edge case: empty range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c))) |
            hpx::is_heap(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool solution = std::is_heap(std::begin(c), std::begin(c));

        HPX_TEST(result);
        HPX_TEST_EQ(result, solution);
    }

    {
        // edge case: range with only one element

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(++std::begin(c))) |
            hpx::is_heap(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool solution = std::is_heap(std::begin(c), ++std::begin(c));

        HPX_TEST(result);
        HPX_TEST_EQ(result, solution);
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_is_heap_until_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::size_t(gen()));

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c))) |
            hpx::is_heap_until(ex_policy.on(exec)));

        iterator result = hpx::get<0>(*snd_result);

        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result.base() == solution);
    }

    {
        // edge case: empty range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c))) |
            hpx::is_heap_until(ex_policy.on(exec)));

        iterator result = hpx::get<0>(*snd_result);

        auto solution = std::is_heap_until(std::begin(c), std::begin(c));

        HPX_TEST(result.base() == std::begin(c));
        HPX_TEST(result.base() == solution);
    }

    {
        // edge case: range of length 1

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(++std::begin(c))) |
            hpx::is_heap_until(ex_policy.on(exec)));

        iterator result = hpx::get<0>(*snd_result);

        auto solution = std::is_heap_until(std::begin(c), ++std::begin(c));

        HPX_TEST(result.base() == ++std::begin(c));
        HPX_TEST(result.base() == solution);
    }
}

template <typename IteratorTag, typename DataType, typename Pred>
void test_is_heap_with_pred(
    IteratorTag, DataType, Pred pred, bool test_for_is_heap = true)
{
    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<DataType> c(10007);
    std::iota(std::begin(c), std::end(c), DataType(gen()));

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    if (test_for_is_heap)
    {
        bool result =
            hpx::is_heap(iterator(std::begin(c)), iterator(std::end(c)), pred);
        bool solution = std::is_heap(std::begin(c), std::end(c), pred);

        HPX_TEST_EQ(result, solution);
    }
    else
    {
        iterator result = hpx::is_heap_until(
            iterator(std::begin(c)), iterator(std::end(c)), pred);
        auto solution = std::is_heap_until(std::begin(c), std::end(c), pred);

        HPX_TEST(result.base() == solution);
    }
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_is_heap_with_pred(ExPolicy&& policy, IteratorTag, DataType, Pred pred,
    bool test_for_is_heap = true)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<DataType> c(10007);
    std::iota(std::begin(c), std::end(c), DataType(gen()));

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    if (test_for_is_heap)
    {
        bool result = hpx::is_heap(
            policy, iterator(std::begin(c)), iterator(std::end(c)), pred);
        bool solution = std::is_heap(std::begin(c), std::end(c), pred);

        HPX_TEST_EQ(result, solution);
    }
    else
    {
        iterator result = hpx::is_heap_until(
            policy, iterator(std::begin(c)), iterator(std::end(c)), pred);
        auto solution = std::is_heap_until(std::begin(c), std::end(c), pred);

        HPX_TEST(result.base() == solution);
    }
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_is_heap_async(
    ExPolicy&& policy, IteratorTag, DataType, bool test_for_is_heap = true)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<DataType> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    auto heap_end_iter = std::next(std::begin(c), dis(gen));
    std::make_heap(std::begin(c), heap_end_iter);

    if (test_for_is_heap)
    {
        auto f = hpx::is_heap(
            policy, iterator(std::begin(c)), iterator(std::end(c)));
        bool result = f.get();
        bool solution = std::is_heap(std::begin(c), std::end(c));

        HPX_TEST_EQ(result, solution);
    }
    else
    {
        auto f = hpx::is_heap_until(
            policy, iterator(std::begin(c)), iterator(std::end(c)));
        iterator result = f.get();
        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result.base() == solution);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_is_heap_exception(IteratorTag, bool test_for_is_heap = true)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());
    std::make_heap(std::begin(c), std::end(c));

    bool caught_exception = false;
    try
    {
        if (test_for_is_heap)
        {
            hpx::is_heap(
                iterator(std::begin(c)), iterator(std::end(c)), throw_always());
        }
        else
        {
            hpx::is_heap_until(
                iterator(std::begin(c)), iterator(std::end(c)), throw_always());
        }

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
void test_is_heap_exception(
    ExPolicy&& policy, IteratorTag, bool test_for_is_heap = true)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());
    std::make_heap(std::begin(c), std::end(c));

    bool caught_exception = false;
    try
    {
        if (test_for_is_heap)
        {
            hpx::is_heap(policy, iterator(std::begin(c)), iterator(std::end(c)),
                throw_always());
        }
        else
        {
            hpx::is_heap_until(policy, iterator(std::begin(c)),
                iterator(std::end(c)), throw_always());
        }

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
void test_is_heap_exception_async(
    ExPolicy&& policy, IteratorTag, bool test_for_is_heap = true)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());
    std::make_heap(std::begin(c), std::end(c));

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        if (test_for_is_heap)
        {
            auto f = hpx::is_heap(policy, iterator(std::begin(c)),
                iterator(std::end(c)), throw_always());
            returned_from_algorithm = true;
            f.get();
        }
        else
        {
            auto f = hpx::is_heap_until(policy, iterator(std::begin(c)),
                iterator(std::end(c)), throw_always());
            returned_from_algorithm = true;
            f.get();
        }

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
    HPX_TEST(returned_from_algorithm);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_is_heap_bad_alloc(IteratorTag, bool test_for_is_heap = true)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());
    std::make_heap(std::begin(c), std::end(c));

    bool caught_bad_alloc = false;
    try
    {
        if (test_for_is_heap)
        {
            hpx::is_heap(iterator(std::begin(c)), iterator(std::end(c)),
                throw_bad_alloc());
        }
        else
        {
            hpx::is_heap_until(iterator(std::begin(c)), iterator(std::end(c)),
                throw_bad_alloc());
        }

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
void test_is_heap_bad_alloc(
    ExPolicy&& policy, IteratorTag, bool test_for_is_heap = true)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());
    std::make_heap(std::begin(c), std::end(c));

    bool caught_bad_alloc = false;
    try
    {
        if (test_for_is_heap)
        {
            hpx::is_heap(policy, iterator(std::begin(c)), iterator(std::end(c)),
                throw_bad_alloc());
        }
        else
        {
            hpx::is_heap_until(policy, iterator(std::begin(c)),
                iterator(std::end(c)), throw_bad_alloc());
        }

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
void test_is_heap_bad_alloc_async(
    ExPolicy&& policy, IteratorTag, bool test_for_is_heap = true)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen());
    std::make_heap(std::begin(c), std::end(c));

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        if (test_for_is_heap)
        {
            auto f = hpx::is_heap(policy, iterator(std::begin(c)),
                iterator(std::end(c)), throw_bad_alloc());
            returned_from_algorithm = true;
            f.get();
        }
        else
        {
            auto f = hpx::is_heap_until(policy, iterator(std::begin(c)),
                iterator(std::end(c)), throw_bad_alloc());
            returned_from_algorithm = true;
            f.get();
        }

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

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_is_heap(bool test_for_is_heap = true)
{
    using namespace hpx::execution;

    test_is_heap(IteratorTag(), int(), test_for_is_heap);
    test_is_heap(seq, IteratorTag(), int(), test_for_is_heap);
    test_is_heap(par, IteratorTag(), int(), test_for_is_heap);
    test_is_heap(par_unseq, IteratorTag(), int(), test_for_is_heap);

    test_is_heap(IteratorTag(), user_defined_type(), test_for_is_heap);
    test_is_heap(seq, IteratorTag(), user_defined_type(), test_for_is_heap);
    test_is_heap(par, IteratorTag(), user_defined_type(), test_for_is_heap);
    test_is_heap(
        par_unseq, IteratorTag(), user_defined_type(), test_for_is_heap);

    test_is_heap_with_pred(
        IteratorTag(), int(), std::greater<int>(), test_for_is_heap);
    test_is_heap_with_pred(
        seq, IteratorTag(), int(), std::greater<int>(), test_for_is_heap);
    test_is_heap_with_pred(
        par, IteratorTag(), int(), std::greater<int>(), test_for_is_heap);
    test_is_heap_with_pred(
        par_unseq, IteratorTag(), int(), std::greater<int>(), test_for_is_heap);

    test_is_heap_async(seq(task), IteratorTag(), int(), test_for_is_heap);
    test_is_heap_async(par(task), IteratorTag(), int(), test_for_is_heap);

    test_is_heap_async(
        seq(task), IteratorTag(), user_defined_type(), test_for_is_heap);
    test_is_heap_async(
        par(task), IteratorTag(), user_defined_type(), test_for_is_heap);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_is_heap_exception(bool test_for_is_heap = true)
{
    using namespace hpx::execution;

    test_is_heap_exception(IteratorTag(), test_for_is_heap);

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_is_heap_exception(seq, IteratorTag(), test_for_is_heap);
    test_is_heap_exception(par, IteratorTag(), test_for_is_heap);

    test_is_heap_exception_async(seq(task), IteratorTag(), test_for_is_heap);
    test_is_heap_exception_async(par(task), IteratorTag(), test_for_is_heap);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_is_heap_bad_alloc(bool test_for_is_heap = true)
{
    using namespace hpx::execution;

    test_is_heap_bad_alloc(IteratorTag(), test_for_is_heap);

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_is_heap_bad_alloc(seq, IteratorTag(), test_for_is_heap);
    test_is_heap_bad_alloc(par, IteratorTag(), test_for_is_heap);

    test_is_heap_bad_alloc_async(seq(task), IteratorTag(), test_for_is_heap);
    test_is_heap_bad_alloc_async(par(task), IteratorTag(), test_for_is_heap);
}
