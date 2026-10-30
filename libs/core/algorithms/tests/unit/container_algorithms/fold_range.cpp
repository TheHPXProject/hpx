//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Mamidi Surya Teja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

// Manual implementation of fold algorithms for verification
template <class I, class S, class T, class Op>
T manual_fold_left(I first, S last, T init, Op op)
{
    for (; first != last; ++first)
    {
        init = op(std::move(init), *first);
    }
    return init;
}

template <class I, class S, class Op>
auto manual_fold_left_first(I first, S last, Op op)
{
    using U = decltype(op(*first, *first));
    if (first == last)
    {
        return hpx::optional<U>();
    }

    U result = *first;
    ++first;
    for (; first != last; ++first)
    {
        result = op(std::move(result), *first);
    }
    return hpx::optional<U>(std::move(result));
}

template <class I, class S, class T, class Op>
T manual_fold_right(I first, S last, T init, Op op)
{
    using rev_iter = std::reverse_iterator<I>;
    I last_it = first;
    while (last_it != last)
        ++last_it;

    // Reverse iterate: fold_right accumulates from the right
    // op(*it, init)
    for (auto it = rev_iter(last_it); it != rev_iter(first); ++it)
    {
        init = op(*it, std::move(init));
    }
    return init;
}

template <class I, class S, class Op>
auto manual_fold_right_last(I first, S last, Op op)
{
    using U = decltype(op(*first, *first));
    if (first == last)
    {
        return hpx::optional<U>();
    }

    using rev_iter = std::reverse_iterator<I>;
    I last_it = first;
    while (last_it != last)
        ++last_it;

    auto it = rev_iter(last_it);
    U result = *it;
    ++it;

    for (; it != rev_iter(first); ++it)
    {
        result = op(*it, std::move(result));
    }
    return hpx::optional<U>(std::move(result));
}

void test_fold_left_iter()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result = hpx::ranges::fold_left(
        c.begin(), c.end(), std::size_t(0), std::plus<>{});
    auto manual_result =
        manual_fold_left(c.begin(), c.end(), std::size_t(0), std::plus<>{});
    HPX_TEST_EQ(hpx_result, manual_result);

    std::vector<std::size_t> small_c = test::random_repeat(10, std::size_t(5));
    hpx_result = hpx::ranges::fold_left(
        small_c.begin(), small_c.end(), std::size_t(1), std::multiplies<>{});
    manual_result = manual_fold_left(
        small_c.begin(), small_c.end(), std::size_t(1), std::multiplies<>{});
    HPX_TEST_EQ(hpx_result, manual_result);
}

void test_fold_left_range()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result = hpx::ranges::fold_left(c, std::size_t(0), std::plus<>{});
    auto manual_result =
        manual_fold_left(c.begin(), c.end(), std::size_t(0), std::plus<>{});
    HPX_TEST_EQ(hpx_result, manual_result);
}

void test_fold_left_empty()
{
    std::vector<std::size_t> c;

    auto hpx_result = hpx::ranges::fold_left(c, std::size_t(42), std::plus<>{});
    HPX_TEST_EQ(hpx_result, std::size_t(42));
}

void test_fold_left_first_iter()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result =
        hpx::ranges::fold_left_first(c.begin(), c.end(), std::plus<>{});
    auto manual_result =
        manual_fold_left_first(c.begin(), c.end(), std::plus<>{});

    HPX_TEST(hpx_result.has_value());
    HPX_TEST(manual_result.has_value());
    if (hpx_result && manual_result)
    {
        HPX_TEST_EQ(*hpx_result, *manual_result);
    }

    std::vector<std::size_t> small_c = test::random_repeat(10, std::size_t(5));
    auto hpx_mult = hpx::ranges::fold_left_first(
        small_c.begin(), small_c.end(), std::multiplies<>{});
    auto manual_mult = manual_fold_left_first(
        small_c.begin(), small_c.end(), std::multiplies<>{});

    HPX_TEST(hpx_mult.has_value());
    HPX_TEST(manual_mult.has_value());
    if (hpx_mult && manual_mult)
    {
        HPX_TEST_EQ(*hpx_mult, *manual_mult);
    }
}

void test_fold_left_first_range()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result = hpx::ranges::fold_left_first(c, std::plus<>{});
    auto manual_result =
        manual_fold_left_first(c.begin(), c.end(), std::plus<>{});

    HPX_TEST(hpx_result.has_value());
    HPX_TEST(manual_result.has_value());
    if (hpx_result && manual_result)
    {
        HPX_TEST_EQ(*hpx_result, *manual_result);
    }
}

void test_fold_left_first_empty()
{
    std::vector<std::size_t> c;

    auto hpx_result = hpx::ranges::fold_left_first(c, std::plus<>{});
    HPX_TEST(!hpx_result.has_value());
}

void test_fold_right_iter()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result = hpx::ranges::fold_right(
        c.begin(), c.end(), std::size_t(0), std::plus<>{});
    auto manual_result =
        manual_fold_right(c.begin(), c.end(), std::size_t(0), std::plus<>{});
    HPX_TEST_EQ(hpx_result, manual_result);

    std::vector<int> small_c = {1, 2, 3, 4, 5};
    auto hpx_sub = hpx::ranges::fold_right(
        small_c.begin(), small_c.end(), 0, std::minus<>{});
    auto manual_sub =
        manual_fold_right(small_c.begin(), small_c.end(), 0, std::minus<>{});
    HPX_TEST_EQ(hpx_sub, manual_sub);
}

void test_fold_right_range()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result = hpx::ranges::fold_right(c, std::size_t(0), std::plus<>{});
    auto manual_result =
        manual_fold_right(c.begin(), c.end(), std::size_t(0), std::plus<>{});
    HPX_TEST_EQ(hpx_result, manual_result);
}

void test_fold_right_empty()
{
    std::vector<std::size_t> c;

    auto hpx_result =
        hpx::ranges::fold_right(c, std::size_t(42), std::plus<>{});
    HPX_TEST_EQ(hpx_result, std::size_t(42));
}

void test_fold_right_last_iter()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result =
        hpx::ranges::fold_right_last(c.begin(), c.end(), std::plus<>{});
    auto manual_result =
        manual_fold_right_last(c.begin(), c.end(), std::plus<>{});

    HPX_TEST(hpx_result.has_value());
    HPX_TEST(manual_result.has_value());
    if (hpx_result && manual_result)
    {
        HPX_TEST_EQ(*hpx_result, *manual_result);
    }

    std::vector<std::size_t> small_c = test::random_repeat(10, std::size_t(5));
    auto hpx_mult = hpx::ranges::fold_right_last(
        small_c.begin(), small_c.end(), std::multiplies<>{});
    auto manual_mult = manual_fold_right_last(
        small_c.begin(), small_c.end(), std::multiplies<>{});

    HPX_TEST(hpx_mult.has_value());
    HPX_TEST(manual_mult.has_value());
    if (hpx_mult && manual_mult)
    {
        HPX_TEST_EQ(*hpx_mult, *manual_mult);
    }
}

void test_fold_right_last_range()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto hpx_result = hpx::ranges::fold_right_last(c, std::plus<>{});
    auto manual_result =
        manual_fold_right_last(c.begin(), c.end(), std::plus<>{});

    HPX_TEST(hpx_result.has_value());
    HPX_TEST(manual_result.has_value());
    if (hpx_result && manual_result)
    {
        HPX_TEST_EQ(*hpx_result, *manual_result);
    }
}

void test_fold_right_last_empty()
{
    std::vector<std::size_t> c;

    auto hpx_result = hpx::ranges::fold_right_last(c, std::plus<>{});
    HPX_TEST(!hpx_result.has_value());
}

void test_fold_left_with_iter_iter()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto [hpx_iter, hpx_value] = hpx::ranges::fold_left_with_iter(
        c.begin(), c.end(), std::size_t(0), std::plus<>{});
    auto manual_result =
        manual_fold_left(c.begin(), c.end(), std::size_t(0), std::plus<>{});

    HPX_TEST(hpx_iter == c.end());
    HPX_TEST_EQ(hpx_value, manual_result);
}

void test_fold_left_with_iter_range()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto [hpx_iter, hpx_value] =
        hpx::ranges::fold_left_with_iter(c, std::size_t(0), std::plus<>{});
    auto manual_result =
        manual_fold_left(c.begin(), c.end(), std::size_t(0), std::plus<>{});

    HPX_TEST(hpx_iter == c.end());
    HPX_TEST_EQ(hpx_value, manual_result);
}

void test_fold_left_with_iter_empty()
{
    std::vector<std::size_t> c;

    auto [hpx_iter, hpx_value] =
        hpx::ranges::fold_left_with_iter(c, std::size_t(42), std::plus<>{});

    HPX_TEST(hpx_iter == c.end());
    HPX_TEST_EQ(hpx_value, std::size_t(42));
}

void test_fold_left_first_with_iter_iter()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto [hpx_iter, hpx_value] = hpx::ranges::fold_left_first_with_iter(
        c.begin(), c.end(), std::plus<>{});
    auto manual_result =
        manual_fold_left_first(c.begin(), c.end(), std::plus<>{});

    HPX_TEST(hpx_iter == c.end());
    HPX_TEST(hpx_value.has_value());
    HPX_TEST(manual_result.has_value());
    if (hpx_value && manual_result)
    {
        HPX_TEST_EQ(*hpx_value, *manual_result);
    }
}

void test_fold_left_first_with_iter_range()
{
    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    auto [hpx_iter, hpx_value] =
        hpx::ranges::fold_left_first_with_iter(c, std::plus<>{});
    auto manual_result =
        manual_fold_left_first(c.begin(), c.end(), std::plus<>{});

    HPX_TEST(hpx_iter == c.end());
    HPX_TEST(hpx_value.has_value());
    HPX_TEST(manual_result.has_value());
    if (hpx_value && manual_result)
    {
        HPX_TEST_EQ(*hpx_value, *manual_result);
    }
}

void test_fold_left_first_with_iter_empty()
{
    std::vector<std::size_t> c;

    auto [hpx_iter, hpx_value] =
        hpx::ranges::fold_left_first_with_iter(c, std::plus<>{});

    HPX_TEST(hpx_iter == c.end());
    HPX_TEST(!hpx_value.has_value());
}

// A type that is constructible from int const& (the iterator reference type
// for vector<int>) but does NOT have an implicit conversion from int (the
// iter_value_t). This distinguishes the tightened constraint
//   constructible_from<U, iter_reference_t<Iter>>
// from the old, weaker
//   constructible_from<iter_value_t<Iter>, iter_reference_t<Iter>>.
struct ref_constructible_accumulator
{
    int value;

    // Constructible from a const lvalue reference - matches iter_reference_t
    ref_constructible_accumulator(int const& v)
      : value(v)
    {
    }
    ref_constructible_accumulator(
        ref_constructible_accumulator const&) = default;
    ref_constructible_accumulator(ref_constructible_accumulator&&) = default;
    ref_constructible_accumulator& operator=(
        ref_constructible_accumulator const&) = default;
    ref_constructible_accumulator& operator=(
        ref_constructible_accumulator&&) = default;
};

struct copyable_asymmetric_accumulator
{
    int value;

    copyable_asymmetric_accumulator(int v)
      : value(v)
    {
    }
    copyable_asymmetric_accumulator(
        copyable_asymmetric_accumulator const&) = default;
    copyable_asymmetric_accumulator(
        copyable_asymmetric_accumulator&&) = default;
    copyable_asymmetric_accumulator& operator=(
        copyable_asymmetric_accumulator const&) = default;
    copyable_asymmetric_accumulator& operator=(
        copyable_asymmetric_accumulator&&) = default;
};

// ---------------------------------------------------------------------------
// Constraint verification (static_assert)
//
// The requires()-clause on fold_left_first / fold_right_last now enforces
//   std::constructible_from<U, iter_reference_t<Iter>>
// where U = decay_t<invoke_result_t<F&, iter_value_t<Iter>,
//                                       iter_reference_t<Iter>>>.
//
// For vector<int>::iterator:
//   iter_value_t      = int
//   iter_reference_t  = int const& (conceptually; actually int& for vector)
//
// ref_constructible_accumulator is constructible from int const& directly,
// so the constraint is satisfied. Verify this statically.
// ---------------------------------------------------------------------------
namespace constraint_checks {

    using iter_ref_t = int const&;    // representative iter_reference_t
    using vec_iter_ref_t =
        std::iter_reference_t<std::vector<int>::iterator>;    // int&

    // U = ref_constructible_accumulator (produced by the fold callable below)
    // The constraint: constructible_from<U, iter_ref_t> must hold.
    static_assert(
        std::constructible_from<ref_constructible_accumulator, iter_ref_t>,
        "fold_left_first constraint: U must be constructible from "
        "iter_reference_t<Iter>");

    static_assert(
        std::constructible_from<ref_constructible_accumulator, vec_iter_ref_t>,
        "fold_left_first constraint: U must be constructible from "
        "iter_reference_t<std::vector<int>::iterator>");

    // Symmetrically for fold_right_last (same iterator/reference types here).
    static_assert(
        std::constructible_from<ref_constructible_accumulator, iter_ref_t>,
        "fold_right_last constraint: U must be constructible from "
        "iter_reference_t<Iter>");

    static_assert(
        std::constructible_from<ref_constructible_accumulator, vec_iter_ref_t>,
        "fold_right_last constraint: U must be constructible from "
        "iter_reference_t<std::vector<int>::iterator>");

}    // namespace constraint_checks

void test_fold_left_first_asymmetric()
{
    std::vector<int> c = {1, 2, 3, 4, 5};
    std::vector<int> expected_c = {1, 2, 3, 4, 5};

    auto custom_op = [](copyable_asymmetric_accumulator acc, int elem) {
        acc.value += elem;
        return acc;
    };

    auto hpx_result = hpx::ranges::fold_left_first(c, custom_op);
    HPX_TEST(hpx_result.has_value());
    if (hpx_result)
    {
        HPX_TEST_EQ(hpx_result->value, 15);
    }

    HPX_TEST(std::equal(c.begin(), c.end(), expected_c.begin()));
}

void test_fold_right_last_asymmetric()
{
    std::vector<int> c = {1, 2, 3, 4, 5};
    std::vector<int> expected_c = {1, 2, 3, 4, 5};

    auto custom_op = [](int elem, copyable_asymmetric_accumulator acc) {
        acc.value += elem;
        return acc;
    };

    auto hpx_result = hpx::ranges::fold_right_last(c, custom_op);
    HPX_TEST(hpx_result.has_value());
    if (hpx_result)
    {
        HPX_TEST_EQ(hpx_result->value, 15);
    }

    HPX_TEST(std::equal(c.begin(), c.end(), expected_c.begin()));
}

// Exercises the tightened constructible_from<U, iter_reference_t<Iter>>
// constraint with ref_constructible_accumulator as the accumulator type U.
//
// For fold_left_first, U = decay_t<invoke_result_t<F&, iter_value_t<I>,
// iter_reference_t<I>>>. The constraint additionally requires:
//   constructible_from<U, iter_reference_t<I>>
// so that the initial seed U result{*first} is valid.
//
// To satisfy is_indirect_binary_left_foldable the operator must also be
// invocable with (U, iter_reference_t<I>), i.e. both the first call
// F(iter_value_t, iter_reference_t) and subsequent calls F(U, iter_ref)
// must compile. A generic lambda handles both overloads transparently.
void test_fold_left_first_ref_constructible_constraint()
{
    std::vector<int> c = {3, 1, 4, 1, 5};

    // Use a struct with two overloads to handle both call sites:
    // First call: F(int, int&)  -> rca  (satisfies invocable<F&, int, int&>)
    // Later calls: F(rca, int&) -> rca  (satisfies invocable<F&, rca, int&>)
    // U = ref_constructible_accumulator.
    // constructible_from<rca, int&> is satisfied by rca(int const&).
    struct fold_op
    {
        ref_constructible_accumulator operator()(
            int seed, int const& elem) const
        {
            return ref_constructible_accumulator(seed + elem);
        }
        ref_constructible_accumulator operator()(
            ref_constructible_accumulator acc, int const& elem) const
        {
            return ref_constructible_accumulator(acc.value + elem);
        }
    };

    auto result = hpx::ranges::fold_left_first(c, fold_op{});
    HPX_TEST(result.has_value());
    // seed = c[0] = 3, then op(3, 1)=rca{4}, op(rca{4},4)=rca{8},
    // op(rca{8},1)=rca{9}, op(rca{9},5)=rca{14}
    if (result)
    {
        HPX_TEST_EQ(result->value, 14);
    }
}

// Exercises the tightened constructible_from<U, iter_reference_t<Iter>>
// constraint on fold_right_last. U = decay_t<invoke_result_t<F&,
// iter_reference_t<I>, iter_value_t<I>>>. The constraint requires:
//   constructible_from<U, iter_reference_t<I>>
// so that U result{*--it} is valid.
void test_fold_right_last_ref_constructible_constraint()
{
    std::vector<int> c = {3, 1, 4, 1, 5};

    // Generic op: (int const&, auto) -> ref_constructible_accumulator.
    // First call: F(int&, int) -> rca   (satisfies invocable<F&, int&, int>)
    // Later calls: F(int&, rca) -> rca  (satisfies invocable<F&, int&, rca>)
    struct fold_op
    {
        ref_constructible_accumulator operator()(
            int const& elem, int seed) const
        {
            return ref_constructible_accumulator(elem + seed);
        }
        ref_constructible_accumulator operator()(
            int const& elem, ref_constructible_accumulator acc) const
        {
            return ref_constructible_accumulator(elem + acc.value);
        }
    };

    auto result = hpx::ranges::fold_right_last(c, fold_op{});
    HPX_TEST(result.has_value());
    // fold_right_last on {3,1,4,1,5}:
    // seed=c[4]=5, op(1,5)=rca{6}, op(4,rca{6})=rca{10},
    // op(1,rca{10})=rca{11}, op(3,rca{11})=rca{14}
    if (result)
    {
        HPX_TEST_EQ(result->value, 14);
    }
}

void test_fold_custom_op()
{
    std::vector<std::size_t> c = test::random_repeat(1007, std::size_t(100));

    auto custom_op = [](std::size_t a, std::size_t b) { return a + b * 2; };

    auto hpx_result = hpx::ranges::fold_left(c, std::size_t(0), custom_op);
    auto manual_result =
        manual_fold_left(c.begin(), c.end(), std::size_t(0), custom_op);
    HPX_TEST_EQ(hpx_result, manual_result);
}

int hpx_main()
{
    test_fold_left_iter();
    test_fold_left_range();
    test_fold_left_empty();

    test_fold_left_first_iter();
    test_fold_left_first_range();
    test_fold_left_first_empty();

    test_fold_right_iter();
    test_fold_right_range();
    test_fold_right_empty();

    test_fold_right_last_iter();
    test_fold_right_last_range();
    test_fold_right_last_empty();

    test_fold_left_with_iter_iter();
    test_fold_left_with_iter_range();
    test_fold_left_with_iter_empty();

    test_fold_left_first_with_iter_iter();
    test_fold_left_first_with_iter_range();
    test_fold_left_first_with_iter_empty();

    test_fold_left_first_asymmetric();
    test_fold_right_last_asymmetric();

    test_fold_left_first_ref_constructible_constraint();
    test_fold_right_last_ref_constructible_constraint();

    test_fold_custom_op();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
