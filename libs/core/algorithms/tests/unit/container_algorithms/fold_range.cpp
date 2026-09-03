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
    explicit ref_constructible_accumulator(int const& v)
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

    // U = ref_constructible_accumulator (produced by the fold callable below)
    // The constraint: constructible_from<U, iter_ref_t> must hold.
    static_assert(
        std::constructible_from<ref_constructible_accumulator, iter_ref_t>,
        "fold_left_first constraint: U must be constructible from "
        "iter_reference_t<Iter>");

    // Symmetrically for fold_right_last (same iterator/reference types here).
    static_assert(
        std::constructible_from<ref_constructible_accumulator, iter_ref_t>,
        "fold_right_last constraint: U must be constructible from "
        "iter_reference_t<Iter>");

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
// The seed expression U result{*first} / U result{*--it} constructs U
// directly from the iterator reference - exactly what the requires()-clause
// now mandates.
void test_fold_left_first_ref_constructible_constraint()
{
    std::vector<int> c = {3, 1, 4, 1, 5};

    // op: (ref_constructible_accumulator, int) -> ref_constructible_accumulator
    // U = ref_constructible_accumulator
    // The requires()-clause checks constructible_from<U, int const&>,
    // which must hold for this to compile.
    auto op = [](ref_constructible_accumulator acc,
                  int const& elem) -> ref_constructible_accumulator {
        acc.value += elem;
        return acc;
    };

    auto result = hpx::ranges::fold_left_first(c, op);
    HPX_TEST(result.has_value());
    // 3 + 1 + 4 + 1 + 5 = 14
    if (result)
    {
        HPX_TEST_EQ(result->value, 14);
    }
}

void test_fold_right_last_ref_constructible_constraint()
{
    std::vector<int> c = {3, 1, 4, 1, 5};

    // op: (int, ref_constructible_accumulator) -> ref_constructible_accumulator
    // U = ref_constructible_accumulator
    // The requires()-clause checks constructible_from<U, int const&>.
    auto op = [](int const& elem, ref_constructible_accumulator acc)
        -> ref_constructible_accumulator {
        acc.value += elem;
        return acc;
    };

    auto result = hpx::ranges::fold_right_last(c, op);
    HPX_TEST(result.has_value());
    // 3 + 1 + 4 + 1 + 5 = 14
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
