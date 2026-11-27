//  Copyright (c) 2017-2018 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
std::mt19937 _gen(0);

inline void inplace_merge_seed(unsigned int seed)
{
    _gen.seed(seed);
}

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
    {
        std::uniform_int_distribution<> dist(
            0, static_cast<int>(name_list.size() - 1));
        name = name_list[dist(_gen)];
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

    user_defined_type operator+(int val) const
    {
        user_defined_type t(*this);
        t.val += val;
        return t;
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
    random_fill(std::size_t rand_base, std::size_t range)
      : gen(_gen())
      , base(static_cast<unsigned int>(rand_base))
      , dist(-static_cast<int>(range / 2), static_cast<int>(range / 2))
    {
    }

    int operator()()
    {
        // add a random offset to rand_base modulo 2^32: rand_base +/- the
        // range may not fit into an int, which would make the range of the
        // distribution invalid
        return static_cast<int>(base + static_cast<unsigned int>(dist(gen)));
    }

    std::mt19937 gen;
    unsigned int base = 0;
    std::uniform_int_distribution<> dist;
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType, typename Comp>
void test_inplace_merge(IteratorTag, DataType, Comp comp, int rand_base)
{
    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle, comp);
    std::sort(res_middle, res_last, comp);

    sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    hpx::inplace_merge(
        iterator(res_first), iterator(res_middle), iterator(res_last), comp);
    std::inplace_merge(sol_first, sol_middle, sol_last, comp);

    bool equality = test::equal(res_first, res_last, sol_first, sol_last);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Comp>
void test_inplace_merge(
    ExPolicy&& policy, IteratorTag, DataType, Comp comp, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle, comp);
    std::sort(res_middle, res_last, comp);

    sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    hpx::inplace_merge(policy, iterator(res_first), iterator(res_middle),
        iterator(res_last), comp);
    std::inplace_merge(sol_first, sol_middle, sol_last, comp);

    bool equality = test::equal(res_first, res_last, sol_first, sol_last);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Comp>
void test_inplace_merge_async(
    ExPolicy&& policy, IteratorTag, DataType, Comp comp, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle, comp);
    std::sort(res_middle, res_last, comp);

    sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    auto f = hpx::inplace_merge(policy, iterator(res_first),
        iterator(res_middle), iterator(res_last), comp);
    f.get();
    std::inplace_merge(sol_first, sol_middle, sol_last, comp);

    bool equality = test::equal(res_first, res_last, sol_first, sol_last);

    HPX_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_inplace_merge_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<int> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill());
    std::generate(res_middle, res_last, random_fill());
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    bool caught_exception = false;
    try
    {
        hpx::inplace_merge(iterator(res_first), iterator(res_middle),
            iterator(res_last), throw_always());

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
void test_inplace_merge_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<int> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill());
    std::generate(res_middle, res_last, random_fill());
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    bool caught_exception = false;
    try
    {
        hpx::inplace_merge(policy, iterator(res_first), iterator(res_middle),
            iterator(res_last), throw_always());

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
void test_inplace_merge_exception_async(ExPolicy&& policy, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<int> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill());
    std::generate(res_middle, res_last, random_fill());
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::inplace_merge(policy, iterator(res_first),
            iterator(res_middle), iterator(res_last), throw_always());
        returned_from_algorithm = true;
        f.get();

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
void test_inplace_merge_bad_alloc(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<int> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill());
    std::generate(res_middle, res_last, random_fill());
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    bool caught_bad_alloc = false;
    try
    {
        hpx::inplace_merge(iterator(res_first), iterator(res_middle),
            iterator(res_last), throw_bad_alloc());

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
void test_inplace_merge_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<int> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill());
    std::generate(res_middle, res_last, random_fill());
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    bool caught_bad_alloc = false;
    try
    {
        hpx::inplace_merge(policy, iterator(res_first), iterator(res_middle),
            iterator(res_last), throw_bad_alloc());

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
void test_inplace_merge_bad_alloc_async(ExPolicy&& policy, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<int> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill());
    std::generate(res_middle, res_last, random_fill());
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::inplace_merge(policy, iterator(res_first),
            iterator(res_middle), iterator(res_last), throw_bad_alloc());
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
template <typename IteratorTag, typename DataType>
void test_inplace_merge_etc(IteratorTag, DataType, unsigned int rand_base)
{
    typedef typename std::vector<DataType>::iterator base_iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    // Test default comparison.
    {
        typedef test::test_iterator<base_iterator, IteratorTag> iterator;

        hpx::inplace_merge(
            iterator(res_first), iterator(res_middle), iterator(res_last));
        std::inplace_merge(sol_first, sol_middle, sol_last);

        bool equality = test::equal(res_first, res_last, sol_first, sol_last);

        HPX_TEST(equality);
    }
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_inplace_merge_etc(
    ExPolicy&& policy, IteratorTag, DataType, unsigned int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    // Test default comparison.
    {
        typedef test::test_iterator<base_iterator, IteratorTag> iterator;

        hpx::inplace_merge(policy, iterator(res_first), iterator(res_middle),
            iterator(res_last));
        std::inplace_merge(sol_first, sol_middle, sol_last);

        bool equality = test::equal(res_first, res_last, sol_first, sol_last);

        HPX_TEST(equality);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_inplace_merge()
{
    using namespace hpx::execution;

    unsigned int rand_base = _gen();

    ////////// Test cases for 'int' type.
    test_inplace_merge(
        IteratorTag(), int(),
        [](unsigned int const a, unsigned int const b) -> bool {
            return a < b;
        },
        rand_base);
    test_inplace_merge(
        seq, IteratorTag(), int(),
        [](unsigned int const a, unsigned int const b) -> bool {
            return a < b;
        },
        rand_base);
    test_inplace_merge(
        par, IteratorTag(), int(),
        [](unsigned int const a, unsigned int const b) -> bool {
            return a < b;
        },
        rand_base);
    test_inplace_merge(
        par_unseq, IteratorTag(), int(),
        [](unsigned int const a, unsigned int const b) -> bool {
            return a > b;
        },
        rand_base);

    ////////// Test cases for user defined type.
    test_inplace_merge(
        IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a < b;
        },
        rand_base);
    test_inplace_merge(
        seq, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a < b;
        },
        rand_base);
    test_inplace_merge(
        par, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a > b;
        },
        rand_base);
    test_inplace_merge(
        par_unseq, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a < b;
        },
        rand_base);

    ////////// Asynchronous test cases for 'int' type.
    test_inplace_merge_async(
        seq(task), IteratorTag(), int(),
        [](unsigned int const a, unsigned int const b) -> bool {
            return a > b;
        },
        rand_base);
    test_inplace_merge_async(
        par(task), IteratorTag(), int(),
        [](unsigned int const a, unsigned int const b) -> bool {
            return a > b;
        },
        rand_base);

    ////////// Asynchronous test cases for user defined type.
    test_inplace_merge_async(
        seq(task), IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a < b;
        },
        rand_base);
    test_inplace_merge_async(
        par(task), IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a < b;
        },
        rand_base);

    ////////// Another test cases for justifying the implementation.
    test_inplace_merge_etc(IteratorTag(), user_defined_type(), rand_base);
    test_inplace_merge_etc(seq, IteratorTag(), user_defined_type(), rand_base);
    test_inplace_merge_etc(par, IteratorTag(), user_defined_type(), rand_base);
    test_inplace_merge_etc(
        par_unseq, IteratorTag(), user_defined_type(), rand_base);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_inplace_merge_exception()
{
    using namespace hpx::execution;

    test_inplace_merge_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_inplace_merge_exception(seq, IteratorTag());
    test_inplace_merge_exception(par, IteratorTag());

    test_inplace_merge_exception_async(seq(task), IteratorTag());
    test_inplace_merge_exception_async(par(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_inplace_merge_bad_alloc()
{
    using namespace hpx::execution;

    test_inplace_merge_bad_alloc(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_inplace_merge_bad_alloc(seq, IteratorTag());
    test_inplace_merge_bad_alloc(par, IteratorTag());

    test_inplace_merge_bad_alloc_async(seq(task), IteratorTag());
    test_inplace_merge_bad_alloc_async(par(task), IteratorTag());
}
