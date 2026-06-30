//  Copyright (c) 2017-2018 Taeguk Kwon
//  Copyright (c)      2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////

struct throw_always
{
    template <typename T>
    bool operator()(T const&, T const&) const
    {
        throw std::runtime_error("test");
    }
};

struct throw_bad_alloc
{
    template <typename T>
    bool operator()(T const&, T const&) const
    {
        throw std::bad_alloc();
    }
};

struct user_defined_type
{
    user_defined_type() = default;
    user_defined_type(int rand_no)
      : val(rand_no)
      , name(name_list[std::rand() % name_list.size()])
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

    bool operator>(user_defined_type const& t) const
    {
        if (this->name > t.name)
            return true;
        else if (this->name < t.name)
            return false;
        else
            return this->val > t.val;
    }

    bool operator==(user_defined_type const& t) const
    {
        return this->name == t.name && this->val == t.val;
    }

    bool operator!=(user_defined_type const& t) const
    {
        return this->name != t.name || this->val != t.val;
    }

    bool operator==(int rand_no) const
    {
        return this->val == rand_no;
    }

    bool operator!=(int rand_no) const
    {
        return this->val != rand_no;
    }

    static std::vector<std::string> const name_list;

    int val;
    std::string name;
};

std::vector<std::string> const user_defined_type::name_list{
    "ABB", "ABC", "ACB", "BASE", "CAA", "CAAA", "CAAB"};

struct random_fill
{
    random_fill() = default;
    random_fill(int rand_base, int range)
      : gen(std::rand())
      , dist(rand_base - range / 2, rand_base + range / 2)
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType, typename Pred>
void test_unique(IteratorTag, DataType, Pred pred, int rand_base)
{
    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto result =
        hpx::unique(iterator(std::begin(c)), iterator(std::end(c)), pred);
    auto solution = std::unique(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_unique(
    ExPolicy policy, IteratorTag, DataType, Pred pred, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto result = hpx::unique(
        policy, iterator(std::begin(c)), iterator(std::end(c)), pred);
    auto solution = std::unique(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_unique_async(
    ExPolicy policy, IteratorTag, DataType, Pred pred, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto f = hpx::unique(
        policy, iterator(std::begin(c)), iterator(std::end(c)), pred);
    auto result = f.get();
    auto solution = std::unique(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_unique_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    try
    {
        auto result = hpx::unique(policy, iterator(std::begin(c)),
            iterator(std::end(c)), throw_always());

        HPX_UNUSED(result);
        HPX_TEST(false);
    }
    catch (hpx::exception_list const&)
    {
        caught_exception = true;
        //test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_unique_exception_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::unique(policy, iterator(std::begin(c)),
            iterator(std::end(c)), throw_always());
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (hpx::exception_list const&)
    {
        caught_exception = true;
        //test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
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
void test_unique_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    try
    {
        auto result = hpx::unique(policy, iterator(std::begin(c)),
            iterator(std::end(c)), throw_bad_alloc());

        HPX_UNUSED(result);
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
void test_unique_bad_alloc_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::unique(policy, iterator(std::begin(c)),
            iterator(std::end(c)), throw_bad_alloc());
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

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_unique_etc(ExPolicy policy, IteratorTag, DataType, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d, org;
    std::generate(
        std::begin(c), std::end(c), random_fill(rand_base, size / 10));
    org = d = c;

    // Test default predicate.
    {
        using iterator = test::test_iterator<base_iterator, IteratorTag>;

        auto result =
            hpx::unique(policy, iterator(std::begin(c)), iterator(std::end(c)));
        auto solution = std::unique(std::begin(d), std::end(d));

        bool equality =
            test::equal(std::begin(c), result.base(), std::begin(d), solution);

        HPX_TEST(equality);
    }

    // Test sequential_unique with input_iterator_tag.
    {
        typedef test::test_iterator<base_iterator, std::input_iterator_tag>
            iterator;

        c = d = org;

        auto result = hpx::parallel::detail::sequential_unique(
            iterator(std::begin(c)), iterator(std::end(c)),
            [](DataType const& a, DataType const& b) -> bool { return a == b; },
            [](DataType& t) -> DataType& { return t; });
        auto solution = std::unique(std::begin(d), std::end(d));

        bool equality =
            test::equal(std::begin(c), result.base(), std::begin(d), solution);

        HPX_TEST(equality);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique()
{
    using namespace hpx::execution;

    int rand_base = std::rand();

    ////////// Test cases for 'int' type.
    test_unique(
        IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);
    test_unique(
        seq, IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);
    test_unique(
        par, IteratorTag(), int(),
        [rand_base](int const a, int const b) -> bool {
            return a == b && b == rand_base;
        },
        rand_base);
    test_unique(
        par_unseq, IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);

    ////////// Test cases for user defined type.
    test_unique(
        IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique(
        seq, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique(
        par, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique(
        par_unseq, IteratorTag(), user_defined_type(),
        [rand_base](user_defined_type const& a, user_defined_type const& b)
            -> bool { return a == b && b == rand_base; },
        rand_base);

    ////////// Asynchronous test cases for 'int' type.
    test_unique_async(
        seq(task), IteratorTag(), int(),
        [rand_base](int const a, int const b) -> bool {
            return a == b && b == rand_base;
        },
        rand_base);
    test_unique_async(
        par(task), IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);

    ////////// Asynchronous test cases for user defined type.
    test_unique_async(
        seq(task), IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_async(
        par(task), IteratorTag(), user_defined_type(),
        [rand_base](user_defined_type const& a, user_defined_type const& b)
            -> bool { return a == rand_base && b == rand_base; },
        rand_base);

    ////////// Corner test cases.
    test_unique(
        par, IteratorTag(), int(),
        [](int const, int const) -> bool { return true; }, rand_base);
    test_unique(
        par_unseq, IteratorTag(), user_defined_type(),
        [](user_defined_type const&, user_defined_type const&) -> bool {
            return false;
        },
        rand_base);

    ////////// Another test cases for justifying the implementation.
    test_unique_etc(seq, IteratorTag(), user_defined_type(), rand_base);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_unique_exception(seq, IteratorTag());
    test_unique_exception(par, IteratorTag());

    test_unique_exception_async(seq(task), IteratorTag());
    test_unique_exception_async(par(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_unique_bad_alloc(seq, IteratorTag());
    test_unique_bad_alloc(par, IteratorTag());

    test_unique_bad_alloc_async(seq(task), IteratorTag());
    test_unique_bad_alloc_async(par(task), IteratorTag());
}

////////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_unique_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    int const rand_base = std::rand();
    auto pred = [](std::size_t const a, std::size_t const b) -> bool {
        return a == b;
    };
    std::size_t const size = 10007;
    std::vector<std::size_t> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)), pred) |
            hpx::unique(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        auto solution = std::unique(std::begin(d), std::end(d), pred);

        bool equality =
            test::equal(std::begin(c), result.base(), std::begin(d), solution);

        HPX_TEST(equality);
    }

    {
        // edge case: empty range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)), pred) |
            hpx::unique(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        auto solution = std::unique(std::begin(d), std::begin(d), pred);

        bool equality =
            test::equal(std::begin(c), result.base(), std::begin(d), solution);

        HPX_TEST(equality);
    }

    {
        // edge case: one element

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(++std::begin(c)), pred) |
            hpx::unique(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        auto solution = std::unique(std::begin(d), ++std::begin(d), pred);

        bool equality =
            test::equal(std::begin(c), result.base(), std::begin(d), solution);

        HPX_TEST(equality);
    }
}
