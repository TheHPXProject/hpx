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
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 _gen(seed);

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
      : val(static_cast<unsigned int>(rand_no))
    {
        std::uniform_int_distribution<> dis(
            0, static_cast<int>(name_list.size() - 1));
        name = name_list[dis(_gen)];
    }

    bool operator<(unsigned int rand_base) const
    {
        static std::string const base_name = "BASE";

        if (this->name < base_name)
            return true;
        else if (this->name > base_name)
            return false;
        else
            return this->val < rand_base;
    }

    bool operator==(user_defined_type const& t) const
    {
        return this->name == t.name && this->val == t.val;
    }

    static std::vector<std::string> const name_list;

    unsigned int val;
    std::string name;
};

std::vector<std::string> const user_defined_type::name_list{
    "ABB", "ABC", "ACB", "BASE", "CAA", "CAAA", "CAAB"};

struct random_fill
{
    random_fill(std::size_t rand_base, std::size_t half_range /* >= 0 */)
      : gen(_gen())
      , base(static_cast<unsigned int>(rand_base))
      , dist(-static_cast<int>(half_range), static_cast<int>(half_range))
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
template <typename IteratorTag, typename DataType, typename Pred>
void test_partition_copy(
    IteratorTag, DataType, Pred pred, unsigned int rand_base)
{
    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d_true_res(size), d_false_res(size),
        d_true_sol(size), d_false_sol(size);
    std::generate(
        std::begin(c), std::end(c), random_fill(rand_base, size / 10));

    auto result = hpx::partition_copy(iterator(std::begin(c)),
        iterator(std::end(c)), iterator(std::begin(d_true_res)),
        iterator(std::begin(d_false_res)), pred);
    auto solution = std::partition_copy(std::begin(c), std::end(c),
        std::begin(d_true_sol), std::begin(d_false_sol), pred);

    bool equality_true = test::equal(std::begin(d_true_res),
        result.first.base(), std::begin(d_true_sol), solution.first);
    bool equality_false = test::equal(std::begin(d_false_res),
        result.second.base(), std::begin(d_false_sol), solution.second);

    HPX_TEST(equality_true);
    HPX_TEST(equality_false);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_partition_copy(
    ExPolicy policy, IteratorTag, DataType, Pred pred, unsigned int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d_true_res(size), d_false_res(size),
        d_true_sol(size), d_false_sol(size);
    std::generate(
        std::begin(c), std::end(c), random_fill(rand_base, size / 10));

    auto result = hpx::partition_copy(policy, iterator(std::begin(c)),
        iterator(std::end(c)), iterator(std::begin(d_true_res)),
        iterator(std::begin(d_false_res)), pred);
    auto solution = std::partition_copy(std::begin(c), std::end(c),
        std::begin(d_true_sol), std::begin(d_false_sol), pred);

    bool equality_true = test::equal(std::begin(d_true_res),
        result.first.base(), std::begin(d_true_sol), solution.first);
    bool equality_false = test::equal(std::begin(d_false_res),
        result.second.base(), std::begin(d_false_sol), solution.second);

    HPX_TEST(equality_true);
    HPX_TEST(equality_false);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_partition_copy_async(
    ExPolicy policy, IteratorTag, DataType, Pred pred, unsigned int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d_true_res(size), d_false_res(size),
        d_true_sol(size), d_false_sol(size);
    std::generate(
        std::begin(c), std::end(c), random_fill(rand_base, size / 10));

    auto f = hpx::partition_copy(policy, iterator(std::begin(c)),
        iterator(std::end(c)), iterator(std::begin(d_true_res)),
        iterator(std::begin(d_false_res)), pred);
    auto result = f.get();
    auto solution = std::partition_copy(std::begin(c), std::end(c),
        std::begin(d_true_sol), std::begin(d_false_sol), pred);

    bool equality_true = test::equal(std::begin(d_true_res),
        result.first.base(), std::begin(d_true_sol), solution.first);
    bool equality_false = test::equal(std::begin(d_false_res),
        result.second.base(), std::begin(d_false_sol), solution.second);

    HPX_TEST(equality_true);
    HPX_TEST(equality_false);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partition_copy_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), d_true(size), d_false(size);
    std::iota(std::begin(c), std::end(c), _gen());

    bool caught_exception = false;
    try
    {
        auto result = hpx::partition_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(d_true)),
            iterator(std::begin(d_false)), throw_always());

        HPX_UNUSED(result);
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
void test_partition_copy_exception_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), d_true(size), d_false(size);
    std::iota(std::begin(c), std::end(c), _gen());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::partition_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(d_true)),
            iterator(std::begin(d_false)), throw_always());
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
template <typename ExPolicy, typename IteratorTag>
void test_partition_copy_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), d_true(size), d_false(size);
    std::iota(std::begin(c), std::end(c), _gen());

    bool caught_bad_alloc = false;
    try
    {
        auto result = hpx::partition_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(d_true)),
            iterator(std::begin(d_false)), throw_bad_alloc());

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
void test_partition_copy_bad_alloc_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), d_true(size), d_false(size);
    std::iota(std::begin(c), std::end(c), _gen());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::partition_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(d_true)),
            iterator(std::begin(d_false)), throw_bad_alloc());
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
void test_partition_copy()
{
    using namespace hpx::execution;

    unsigned int rand_base = _gen();

    ////////// Test cases for 'int' type.
    test_partition_copy(
        IteratorTag(), int(),
        [rand_base](unsigned int const n) -> bool { return n < rand_base; },
        rand_base);
    test_partition_copy(
        seq, IteratorTag(), int(),
        [rand_base](unsigned int const n) -> bool { return n < rand_base; },
        rand_base);
    test_partition_copy(
        par, IteratorTag(), int(),
        [rand_base](unsigned int const n) -> bool { return n <= rand_base; },
        rand_base);
    test_partition_copy(
        par_unseq, IteratorTag(), int(),
        [rand_base](unsigned int const n) -> bool { return n > rand_base; },
        rand_base);

    ////////// Test cases for user defined type.
    test_partition_copy(
        IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& t) -> bool { return t < rand_base; },
        rand_base);
    test_partition_copy(
        seq, IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& t) -> bool { return t < rand_base; },
        rand_base);
    test_partition_copy(
        par, IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& t) -> bool { return !(t < rand_base); },
        rand_base);
    test_partition_copy(
        par_unseq, IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& t) -> bool { return t < rand_base; },
        rand_base);

    ////////// Asynchronous test cases for 'int' type.
    test_partition_copy_async(
        seq(task), IteratorTag(), int(),
        [rand_base](unsigned int const n) -> bool { return n >= rand_base; },
        rand_base);
    test_partition_copy_async(
        par(task), IteratorTag(), int(),
        [rand_base](unsigned int const n) -> bool { return n < rand_base; },
        rand_base);

    ////////// Asynchronous test cases for user defined type.
    test_partition_copy_async(
        seq(task), IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& t) -> bool { return !(t < rand_base); },
        rand_base);
    test_partition_copy_async(
        par(task), IteratorTag(), user_defined_type(),
        [rand_base](
            user_defined_type const& t) -> bool { return t < rand_base; },
        rand_base);

    ////////// Corner test cases.
    test_partition_copy(
        par, IteratorTag(), int(),
        [rand_base](unsigned int const) -> bool {
            HPX_UNUSED(rand_base);
            return true;
        },
        rand_base);
    test_partition_copy(
        par_unseq, IteratorTag(), user_defined_type(),
        [rand_base](user_defined_type const&) -> bool {
            HPX_UNUSED(rand_base);
            return false;
        },
        rand_base);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_partition_copy_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_partition_copy_exception(seq, IteratorTag());
    test_partition_copy_exception(par, IteratorTag());

    test_partition_copy_exception_async(seq(task), IteratorTag());
    test_partition_copy_exception_async(par(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_partition_copy_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_partition_copy_bad_alloc(seq, IteratorTag());
    test_partition_copy_bad_alloc(par, IteratorTag());

    test_partition_copy_bad_alloc_async(seq(task), IteratorTag());
    test_partition_copy_bad_alloc_async(par(task), IteratorTag());
}
