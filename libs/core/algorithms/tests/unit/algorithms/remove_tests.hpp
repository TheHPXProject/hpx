//  Copyright (c) 2017-2018 Taeguk Kwon
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
unsigned int seed = std::random_device{}();
std::mt19937 g(seed);

struct throw_always
{
    template <typename T>
    bool operator()(T const&) const
    {
        throw std::runtime_error("test");
    }
};

struct throw_bad_alloc
{
    template <typename T>
    bool operator()(T const&) const
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
        std::uniform_int_distribution<> dis(
            0, static_cast<int>(name_list.size() - 1));
        name = name_list[dis(g)];
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

    bool operator==(int rand_no) const
    {
        return this->val == rand_no;
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
    random_fill(std::size_t rand_base, std::size_t half_range /* >= 0 */)
      : gen(g())
      , dist(static_cast<int>(rand_base - half_range),
            static_cast<int>(rand_base + half_range))
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
template <typename IteratorTag, typename DataType, typename ValueType>
void test_remove(IteratorTag, DataType, ValueType value, int rand_base)
{
    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto result =
        hpx::remove(iterator(std::begin(c)), iterator(std::end(c)), value);
    auto solution = std::remove(std::begin(d), std::end(d), value);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename ValueType>
void test_remove(
    ExPolicy policy, IteratorTag, DataType, ValueType value, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto result = hpx::remove(
        policy, iterator(std::begin(c)), iterator(std::end(c)), value);
    auto solution = std::remove(std::begin(d), std::end(d), value);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename ValueType>
void test_remove_async(
    ExPolicy policy, IteratorTag, DataType, ValueType value, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto f = hpx::remove(
        policy, iterator(std::begin(c)), iterator(std::end(c)), value);
    auto result = f.get();
    auto solution = std::remove(std::begin(d), std::end(d), value);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType, typename Pred>
void test_remove_if(IteratorTag, DataType, Pred pred, unsigned int rand_base)
{
    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto result =
        hpx::remove_if(iterator(std::begin(c)), iterator(std::end(c)), pred);
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_remove_if(
    ExPolicy policy, IteratorTag, DataType, Pred pred, unsigned int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto result = hpx::remove_if(
        policy, iterator(std::begin(c)), iterator(std::end(c)), pred);
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_remove_if_async(
    ExPolicy policy, IteratorTag, DataType, Pred pred, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto f = hpx::remove_if(
        policy, iterator(std::begin(c)), iterator(std::end(c)), pred);
    auto result = f.get();
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove_exception(IteratorTag, bool test_for_remove_if = false)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    try
    {
        if (test_for_remove_if)
        {
            hpx::remove_if(decorated_iterator(std::begin(c)),
                decorated_iterator(std::end(c)), throw_always());
        }
        else
        {
            hpx::remove(decorated_iterator(std::begin(c),
                            []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)), int(10));
        }

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
void test_remove_exception(
    ExPolicy policy, IteratorTag, bool test_for_remove_if = false)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    try
    {
        if (test_for_remove_if)
        {
            hpx::remove_if(policy, decorated_iterator(std::begin(c)),
                decorated_iterator(std::end(c)), throw_always());
        }
        else
        {
            hpx::remove(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)), int(10));
        }

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
void test_remove_exception_async(
    ExPolicy policy, IteratorTag, bool test_for_remove_if = false)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f;

        if (test_for_remove_if)
        {
            f = hpx::remove_if(policy, decorated_iterator(std::begin(c)),
                decorated_iterator(std::end(c)), throw_always());
        }
        else
        {
            f = hpx::remove(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)), int(10));
        }

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
template <typename IteratorTag>
void test_remove_bad_alloc(IteratorTag, bool test_for_remove_if = false)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    try
    {
        if (test_for_remove_if)
        {
            hpx::remove_if(decorated_iterator(std::begin(c)),
                decorated_iterator(std::end(c)), throw_bad_alloc());
        }
        else
        {
            hpx::remove(decorated_iterator(
                            std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)), int(10));
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
void test_remove_bad_alloc(
    ExPolicy policy, IteratorTag, bool test_for_remove_if = false)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    try
    {
        if (test_for_remove_if)
        {
            hpx::remove_if(policy, decorated_iterator(std::begin(c)),
                decorated_iterator(std::end(c)), throw_bad_alloc());
        }
        else
        {
            hpx::remove(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)), int(10));
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
void test_remove_bad_alloc_async(
    ExPolicy policy, IteratorTag, bool test_for_remove_if = false)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f;

        if (test_for_remove_if)
        {
            f = hpx::remove_if(policy, decorated_iterator(std::begin(c)),
                decorated_iterator(std::end(c)), throw_bad_alloc());
        }
        else
        {
            f = hpx::remove(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)), int(10));
        }

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
template <typename IteratorTag>
void test_remove(IteratorTag, int rand_base)
{
    using namespace hpx::execution;

    ////////// Test cases for 'int' type.
    test_remove(IteratorTag(), int(), int(rand_base + 2), rand_base);
    test_remove(seq, IteratorTag(), int(), int(rand_base + 1), rand_base);
    test_remove(par, IteratorTag(), int(), int(rand_base), rand_base);
    test_remove(par_unseq, IteratorTag(), int(), int(rand_base - 1), rand_base);

    ////////// Test cases for user defined type.
    test_remove(IteratorTag(), user_defined_type(),
        user_defined_type(rand_base), rand_base);
    test_remove(seq, IteratorTag(), user_defined_type(),
        user_defined_type(rand_base), rand_base);
    test_remove(par, IteratorTag(), user_defined_type(),
        user_defined_type(rand_base + 1), rand_base);

    ////////// Asynchronous test cases for 'int' type.
    test_remove_async(
        seq(task), IteratorTag(), int(), int(rand_base), rand_base);
    test_remove_async(
        par(task), IteratorTag(), int(), int(rand_base - 1), rand_base);

    ////////// Asynchronous test cases for user defined type.
    test_remove_async(seq(task), IteratorTag(), user_defined_type(),
        user_defined_type(rand_base - 1), rand_base);
    test_remove_async(par(task), IteratorTag(), user_defined_type(),
        user_defined_type(rand_base), rand_base);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove_if(IteratorTag, int rand_base)
{
    using namespace hpx::execution;

    ////////// Test cases for 'int' type.
    test_remove_if(
        IteratorTag(), int(),
        [rand_base](int const a) -> bool { return a == rand_base; }, rand_base);
    test_remove_if(
        seq, IteratorTag(), int(),
        [rand_base](int const a) -> bool { return a == rand_base; }, rand_base);
    test_remove_if(
        par, IteratorTag(), int(),
        [rand_base](int const a) -> bool { return !(a == rand_base); },
        rand_base);
    test_remove_if(
        par_unseq, IteratorTag(), int(),
        [rand_base](int const a) -> bool { return a == rand_base; }, rand_base);

    ////////// Test cases for user defined type.
    test_remove_if(
        IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& a) -> bool { return !(a == rand_base); },
        rand_base);
    test_remove_if(
        seq, IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& a) -> bool { return !(a == rand_base); },
        rand_base);
    test_remove_if(
        par, IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& a) -> bool { return a == rand_base; },
        rand_base);
    test_remove_if(
        par_unseq, IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& a) -> bool { return !(a == rand_base); },
        rand_base);

    ////////// Asynchronous test cases for 'int' type.
    test_remove_if_async(
        seq(task), IteratorTag(), int(),
        [rand_base](int const a) -> bool { return !(a == rand_base); },
        rand_base);
    test_remove_if_async(
        par(task), IteratorTag(), int(),
        [rand_base](int const a) -> bool { return a == rand_base; }, rand_base);

    ////////// Asynchronous test cases for user defined type.
    test_remove_if_async(
        seq(task), IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& a) -> bool { return a == rand_base; },
        rand_base);
    test_remove_if_async(
        par(task), IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& a) -> bool { return !(a == rand_base); },
        rand_base);

    ////////// Corner test cases.
    test_remove_if(
        par, IteratorTag(), int(), [](int const) -> bool { return true; },
        rand_base);
    test_remove_if(
        par_unseq, IteratorTag(), user_defined_type(),
        [](user_defined_type const&) -> bool { return false; }, rand_base);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove(bool test_for_remove_if = false)
{
    unsigned int rand_base = g();

    if (test_for_remove_if)
    {
        test_remove_if(IteratorTag(), rand_base);
    }
    else
    {
        test_remove(IteratorTag(), rand_base);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove_exception(bool test_for_remove_if = false)
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_exception(IteratorTag(), test_for_remove_if);
    test_remove_exception(seq, IteratorTag(), test_for_remove_if);
    test_remove_exception(par, IteratorTag(), test_for_remove_if);

    test_remove_exception_async(seq(task), IteratorTag(), test_for_remove_if);
    test_remove_exception_async(par(task), IteratorTag(), test_for_remove_if);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove_bad_alloc(bool test_for_remove_if = false)
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_bad_alloc(IteratorTag(), test_for_remove_if);
    test_remove_bad_alloc(seq, IteratorTag(), test_for_remove_if);
    test_remove_bad_alloc(par, IteratorTag(), test_for_remove_if);

    test_remove_bad_alloc_async(seq(task), IteratorTag(), test_for_remove_if);
    test_remove_bad_alloc_async(par(task), IteratorTag(), test_for_remove_if);
}

////////////////////////////////////////////////////////////////////////////////

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_remove_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::size_t rand_base = g();
    std::size_t value = rand_base + 2;

    std::size_t const size = 10007;
    std::vector<int> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto snd_result = tt::sync_wait(
        ex::just(iterator(std::begin(c)), iterator(std::end(c)), value) |
        hpx::remove(ex_policy.on(exec)));

    auto result = hpx::get<0>(*snd_result);

    auto solution = std::remove(std::begin(d), std::end(d), value);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_remove_if_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::size_t rand_base = g();
    auto pred = [rand_base](int const a) -> bool { return a == rand_base; };

    std::size_t const size = 10007;
    std::vector<int> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));
    d = c;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto snd_result = tt::sync_wait(
        ex::just(iterator(std::begin(c)), iterator(std::end(c)), pred) |
        hpx::remove_if(ex_policy.on(exec)));
    auto result = hpx::get<0>(*snd_result);

    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.base(), std::begin(d), solution);

    HPX_TEST(equality);
}
