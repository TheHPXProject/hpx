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
void test_unique_copy(IteratorTag, DataType, Pred pred, int rand_base)
{
    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), dest_res(size), dest_sol(size);
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));

    auto result = hpx::unique_copy(iterator(std::begin(c)),
        iterator(std::end(c)), iterator(std::begin(dest_res)), pred);
    auto solution = std::unique_copy(
        std::begin(c), std::end(c), std::begin(dest_sol), pred);

    bool equality = test::equal(
        std::begin(dest_res), result.base(), std::begin(dest_sol), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_unique_copy(
    ExPolicy policy, IteratorTag, DataType, Pred pred, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), dest_res(size), dest_sol(size);
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));

    auto result = hpx::unique_copy(policy, iterator(std::begin(c)),
        iterator(std::end(c)), iterator(std::begin(dest_res)), pred);
    auto solution = std::unique_copy(
        std::begin(c), std::end(c), std::begin(dest_sol), pred);

    bool equality = test::equal(
        std::begin(dest_res), result.base(), std::begin(dest_sol), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename IteratorTag, typename DataType,
    typename Pred>
void test_unique_copy_async(
    ExPolicy policy, IteratorTag, DataType, Pred pred, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), dest_res(size), dest_sol(size);
    std::generate(std::begin(c), std::end(c), random_fill(rand_base, 6));

    auto f = hpx::unique_copy(policy, iterator(std::begin(c)),
        iterator(std::end(c)), iterator(std::begin(dest_res)), pred);
    auto result = f.get();
    auto solution = std::unique_copy(
        std::begin(c), std::end(c), std::begin(dest_sol), pred);

    bool equality = test::equal(
        std::begin(dest_res), result.base(), std::begin(dest_sol), solution);

    HPX_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_unique_copy_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), dest(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    try
    {
        auto result = hpx::unique_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest)), throw_always());

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
void test_unique_copy_exception_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), dest(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::unique_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest)), throw_always());
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
void test_unique_copy_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), dest(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    try
    {
        auto result = hpx::unique_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest)),
            throw_bad_alloc());

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
void test_unique_copy_bad_alloc_async(ExPolicy policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size = 10007;
    std::vector<int> c(size), dest(size);
    std::generate(std::begin(c), std::end(c), random_fill());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::unique_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest)),
            throw_bad_alloc());
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
void test_unique_copy_etc(ExPolicy policy, IteratorTag, DataType, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), dest_res(size), dest_sol(size);
    std::generate(
        std::begin(c), std::end(c), random_fill(rand_base, size / 10));

    // Test default predicate.
    {
        using iterator = test::test_iterator<base_iterator, IteratorTag>;

        auto result = hpx::unique_copy(policy, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest_res)));
        auto solution =
            std::unique_copy(std::begin(c), std::end(c), std::begin(dest_sol));

        bool equality = test::equal(std::begin(dest_res), result.base(),
            std::begin(dest_sol), solution);

        HPX_TEST(equality);
    }

    // Test sequential_unique_copy with input_iterator_tag.
    {
        typedef test::test_iterator<base_iterator, std::input_iterator_tag>
            input_iterator;
        typedef test::test_iterator<base_iterator, std::output_iterator_tag>
            output_iterator;

        auto result = hpx::parallel::detail::sequential_unique_copy(
            input_iterator(std::begin(c)), input_iterator(std::end(c)),
            output_iterator(std::begin(dest_res)),
            [](DataType const& a, DataType const& b) -> bool { return a == b; },
            // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
            [](DataType const& t) -> DataType const& { return t; },
            std::false_type());
        auto solution =
            std::unique_copy(std::begin(c), std::end(c), std::begin(dest_sol));

        bool equality = test::equal(std::begin(dest_res), result.out.base(),
            std::begin(dest_sol), solution);

        HPX_TEST(equality);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique_copy()
{
    using namespace hpx::execution;

    int rand_base = std::rand();

    ////////// Test cases for 'int' type.
    test_unique_copy(
        IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);
    test_unique_copy(
        seq, IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);
    test_unique_copy(
        par, IteratorTag(), int(),
        [rand_base](int const a, int const b) -> bool {
            return a == b && b == rand_base;
        },
        rand_base);
    test_unique_copy(
        par_unseq, IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);

    ////////// Test cases for user defined type.
    test_unique_copy(
        IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_copy(
        seq, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_copy(
        par, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_copy(
        par_unseq, IteratorTag(), user_defined_type(),
        [rand_base](user_defined_type const& a, user_defined_type const& b)
            -> bool { return a == b && b == rand_base; },
        rand_base);

    ////////// Asynchronous test cases for 'int' type.
    test_unique_copy_async(
        seq(task), IteratorTag(), int(),
        [rand_base](int const a, int const b) -> bool {
            return a == b && b == rand_base;
        },
        rand_base);
    test_unique_copy_async(
        par(task), IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);

    ////////// Asynchronous test cases for user defined type.
    test_unique_copy_async(
        seq(task), IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_copy_async(
        par(task), IteratorTag(), user_defined_type(),
        [rand_base](user_defined_type const& a, user_defined_type const& b)
            -> bool { return a == rand_base && b == rand_base; },
        rand_base);

    ////////// Corner test cases.
    test_unique_copy(
        par, IteratorTag(), int(),
        [](int const, int const) -> bool { return true; }, rand_base);
    test_unique_copy(
        par_unseq, IteratorTag(), user_defined_type(),
        [](user_defined_type const&, user_defined_type const&) -> bool {
            return false;
        },
        rand_base);

    ////////// Another test cases for justifying the implementation.
    test_unique_copy_etc(seq, IteratorTag(), user_defined_type(), rand_base);
    test_unique_copy_etc(par, IteratorTag(), user_defined_type(), rand_base);
    test_unique_copy_etc(
        par_unseq, IteratorTag(), user_defined_type(), rand_base);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique_copy_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_unique_copy_exception(seq, IteratorTag());
    test_unique_copy_exception(par, IteratorTag());

    test_unique_copy_exception_async(seq(task), IteratorTag());
    test_unique_copy_exception_async(par(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique_copy_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_unique_copy_bad_alloc(seq, IteratorTag());
    test_unique_copy_bad_alloc(par, IteratorTag());

    test_unique_copy_bad_alloc_async(seq(task), IteratorTag());
    test_unique_copy_bad_alloc_async(par(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
// Verify that the predicate constraint in hpx::unique_copy is checked against
// the INPUT iterator's value type (iter_value_t<InIter>), not the output
// iterator's value type. This directly exercises the corrected requires clause.
//
// All calls use a predicate typed explicitly on int (the input element type),
// confirming the CPO correctly dispatches with a strongly-typed binary predicate
// across all execution policies.
template <typename IteratorTag>
void test_unique_copy_constraint()
{
    using namespace hpx::execution;
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    // Strongly-typed binary predicate on the input element type (int).
    // Before the fix, the parallel overload's requires clause checked
    // is_invocable_v<Pred, iter_value_t<FwdIter1>, iter_value_t<FwdIter2>>,
    // which could silently mis-constrain when InIter and OutIter differ.
    auto typed_pred = [](int const a, int const b) -> bool { return a == b; };

    std::size_t const size = 10007;
    std::vector<int> c(size), dest_res(size), dest_sol(size);
    std::generate(std::begin(c), std::end(c), random_fill(0, 6));

    // --- seq policy ---
    {
        auto result = hpx::unique_copy(seq, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest_res)), typed_pred);
        auto solution = std::unique_copy(std::begin(c), std::end(c),
            std::begin(dest_sol), [](int a, int b) { return a == b; });
        HPX_TEST(test::equal(std::begin(dest_res), result.base(),
            std::begin(dest_sol), solution));
    }

    // --- par policy ---
    {
        auto result = hpx::unique_copy(par, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest_res)), typed_pred);
        auto solution = std::unique_copy(std::begin(c), std::end(c),
            std::begin(dest_sol), [](int a, int b) { return a == b; });
        HPX_TEST(test::equal(std::begin(dest_res), result.base(),
            std::begin(dest_sol), solution));
    }

    // --- par_unseq policy ---
    {
        auto result = hpx::unique_copy(par_unseq, iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest_res)), typed_pred);
        auto solution = std::unique_copy(std::begin(c), std::end(c),
            std::begin(dest_sol), [](int a, int b) { return a == b; });
        HPX_TEST(test::equal(std::begin(dest_res), result.base(),
            std::begin(dest_sol), solution));
    }

    // --- seq(task) async ---
    {
        auto f = hpx::unique_copy(seq(task), iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest_res)), typed_pred);
        auto result = f.get();
        auto solution = std::unique_copy(std::begin(c), std::end(c),
            std::begin(dest_sol), [](int a, int b) { return a == b; });
        HPX_TEST(test::equal(std::begin(dest_res), result.base(),
            std::begin(dest_sol), solution));
    }

    // --- par(task) async ---
    {
        auto f = hpx::unique_copy(par(task), iterator(std::begin(c)),
            iterator(std::end(c)), iterator(std::begin(dest_res)), typed_pred);
        auto result = f.get();
        auto solution = std::unique_copy(std::begin(c), std::end(c),
            std::begin(dest_sol), [](int a, int b) { return a == b; });
        HPX_TEST(test::equal(std::begin(dest_res), result.base(),
            std::begin(dest_sol), solution));
    }

    // --- Edge case: empty range ---
    // Ensures the constraint does not misfire on zero-length inputs.
    {
        std::vector<int> empty_src;
        std::vector<int> empty_dst;
        auto result = hpx::unique_copy(par, empty_src.begin(), empty_src.end(),
            empty_dst.begin(), typed_pred);
        HPX_TEST(result == empty_dst.begin());

        result = hpx::unique_copy(seq, empty_src.begin(), empty_src.end(),
            empty_dst.begin(), typed_pred);
        HPX_TEST(result == empty_dst.begin());
    }

    // --- Edge case: single element ---
    // Must copy the one element verbatim; no predicate calls occur.
    {
        std::vector<int> single_src = {42};
        std::vector<int> single_dst(1, 0);

        auto result = hpx::unique_copy(seq, single_src.begin(),
            single_src.end(), single_dst.begin(), typed_pred);
        HPX_TEST(result == single_dst.begin() + 1);
        HPX_TEST(single_dst[0] == 42);

        result = hpx::unique_copy(par, single_src.begin(), single_src.end(),
            single_dst.begin(), typed_pred);
        HPX_TEST(result == single_dst.begin() + 1);
        HPX_TEST(single_dst[0] == 42);
    }

    // --- Edge case: all elements identical -> only one element in output ---
    {
        std::vector<int> all_same(100, 7);
        std::vector<int> dst_res(100, 0), dst_sol(100, 0);

        auto result = hpx::unique_copy(
            par, all_same.begin(), all_same.end(), dst_res.begin(), typed_pred);
        auto solution = std::unique_copy(all_same.begin(), all_same.end(),
            dst_sol.begin(), [](int a, int b) { return a == b; });

        HPX_TEST(result == dst_res.begin() + 1);
        HPX_TEST(
            test::equal(dst_res.begin(), result, dst_sol.begin(), solution));
    }

    // --- Edge case: no consecutive duplicates -> full copy ---
    {
        std::vector<int> no_dup = {1, 2, 3, 4, 5};
        std::vector<int> dst_res(5, 0), dst_sol(5, 0);

        auto result = hpx::unique_copy(
            par, no_dup.begin(), no_dup.end(), dst_res.begin(), typed_pred);
        auto solution = std::unique_copy(no_dup.begin(), no_dup.end(),
            dst_sol.begin(), [](int a, int b) { return a == b; });

        HPX_TEST(result == dst_res.begin() + 5);
        HPX_TEST(
            test::equal(dst_res.begin(), result, dst_sol.begin(), solution));
    }

    // --- Edge case: two elements, identical ---
    {
        std::vector<int> two_same = {9, 9};
        std::vector<int> dst_res(2, 0), dst_sol(2, 0);

        auto result = hpx::unique_copy(
            par, two_same.begin(), two_same.end(), dst_res.begin(), typed_pred);
        auto solution = std::unique_copy(two_same.begin(), two_same.end(),
            dst_sol.begin(), [](int a, int b) { return a == b; });

        HPX_TEST(result == dst_res.begin() + 1);
        HPX_TEST(
            test::equal(dst_res.begin(), result, dst_sol.begin(), solution));
    }

    // --- Edge case: two elements, distinct ---
    {
        std::vector<int> two_diff = {3, 5};
        std::vector<int> dst_res(2, 0), dst_sol(2, 0);

        auto result = hpx::unique_copy(
            par, two_diff.begin(), two_diff.end(), dst_res.begin(), typed_pred);
        auto solution = std::unique_copy(two_diff.begin(), two_diff.end(),
            dst_sol.begin(), [](int a, int b) { return a == b; });

        HPX_TEST(result == dst_res.begin() + 2);
        HPX_TEST(
            test::equal(dst_res.begin(), result, dst_sol.begin(), solution));
    }
}
