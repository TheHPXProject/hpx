//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

namespace {
    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else
            return HPX_FORWARD(T, result);
    }

    struct record
    {
        int number;
    };

    struct lvalue_projection
    {
        int& operator()(int& v)
        {
            return v;
        }
    };

    struct nonconst_less
    {
        bool operator()(int lhs, int rhs)
        {
            return lhs < rhs;
        }
    };

    static_assert(std::sortable<std::vector<int>::iterator, nonconst_less,
        lvalue_projection>);

    template <typename Policy>
    void test_lvalue_projection(Policy policy)
    {
        std::vector<int> input(4096);
        auto reset = [&] {
            std::generate(input.begin(), input.end(),
                [n = int(input.size())]() mutable { return --n; });
        };
        auto less = nonconst_less{};
        auto proj = lvalue_projection{};
        reset();
        value(hpx::ranges::sort(policy, input, less, proj));
        HPX_TEST(std::ranges::is_sorted(input));
        reset();
        value(hpx::ranges::stable_sort(policy, input, less, proj));
        HPX_TEST(std::ranges::is_sorted(input));
        reset();
        value(hpx::ranges::partial_sort(
            policy, input, input.begin() + 17, less, proj));
        HPX_TEST(std::is_sorted(input.begin(), input.begin() + 17));
        HPX_TEST_EQ(input[16], 16);
        reset();
        value(hpx::ranges::nth_element(
            policy, input, input.begin() + 17, less, proj));
        HPX_TEST_EQ(input[17], 17);
        reset();
        HPX_TEST(value(hpx::ranges::is_heap(policy, input, less, proj)));
        HPX_TEST(value(hpx::ranges::is_heap_until(policy, input, less, proj)) ==
            input.end());
        input.back() = int(input.size());
        HPX_TEST(!value(hpx::ranges::is_heap(policy, input, less, proj)));
        HPX_TEST(value(hpx::ranges::is_heap_until(policy, input, less, proj)) ==
            input.end() - 1);
    }

    template <typename Policy>
    void test_move_only(Policy policy)
    {
        std::vector<std::unique_ptr<int>> input;
        auto reset = [&] {
            input.clear();
            for (int n = 99; n >= 0; --n)
                input.push_back(std::make_unique<int>(n));
        };
        auto proj = [](auto const& item) { return *item; };
        auto less = std::ranges::less{};
        reset();
        value(hpx::ranges::sort(policy, input, less, proj));
        HPX_TEST(std::ranges::is_sorted(input, less, proj));
        reset();
        value(hpx::ranges::stable_sort(policy, input, less, proj));
        HPX_TEST(std::ranges::is_sorted(input, less, proj));
        reset();
        value(hpx::ranges::partial_sort(
            policy, input, input.begin() + 10, less, proj));
        HPX_TEST_EQ(*input[9], 9);
        HPX_TEST(std::ranges::is_sorted(
            input.begin(), input.begin() + 10, less, proj));
        reset();
        value(hpx::ranges::nth_element(
            policy, input, input.begin() + 10, less, proj));
        HPX_TEST_EQ(*input[10], 10);
        HPX_TEST(value(hpx::ranges::nth_element(
                     policy, input, input.end(), less, proj)) == input.end());
    }

    template <typename Policy>
    void test(Policy policy)
    {
        test_move_only(policy);
        test_lvalue_projection(policy);
        using namespace hpx::ranges;
        auto proj = &record::number;
        auto less = std::ranges::less{};
        std::vector<record> input{{3}, {1}, {2}, {2}};
        HPX_TEST(value(sort(policy, input, less, proj)) == input.end());
        HPX_TEST(std::ranges::is_sorted(input, less, proj));
        HPX_TEST(value(reverse(policy, input)) == input.end());
        HPX_TEST_EQ(input[0].number, 3);
        HPX_TEST(value(stable_sort(policy, input, less, proj)) == input.end());
        HPX_TEST(std::ranges::is_sorted(input, less, proj));
        auto tail = value(unique(policy, input, std::ranges::equal_to{}, proj));
        HPX_TEST(tail.begin() == input.begin() + 3);
        HPX_TEST(tail.end() == input.end());
        input = {{3}, {1}, {2}, {2}};
        tail = value(hpx::ranges::remove(policy, input, 2, proj));
        HPX_TEST(tail.begin() == input.begin() + 2);
        HPX_TEST_EQ(input[0].number, 3);
        HPX_TEST_EQ(input[1].number, 1);
        input = {{3}, {1}, {2}, {2}};
        HPX_TEST(
            value(replace(policy, input, 2, record{5}, proj)) == input.end());
        HPX_TEST_EQ(input[2].number, 5);
        HPX_TEST_EQ(input[3].number, 5);
        auto large = [](int n) { return n > 2; };
        HPX_TEST(value(replace_if(policy, input, large, record{2}, proj)) ==
            input.end());
        HPX_TEST_EQ(input[0].number, 2);
        tail =
            value(remove_if(policy, input, [](int n) { return n == 1; }, proj));
        HPX_TEST(tail.begin() == input.begin() + 3);
        HPX_TEST(tail.end() == input.end());

        input = {{3}, {1}, {2}, {2}};
        tail = value(partition(policy, input, large, proj));
        HPX_TEST(tail.begin() == input.begin() + 1);
        HPX_TEST(tail.end() == input.end());
        input = {{3}, {1}, {2}, {2}};
        tail = value(stable_partition(policy, input, large, proj));
        HPX_TEST(tail.begin() == input.begin() + 1);
        HPX_TEST_EQ(input[1].number, 1);
        HPX_TEST(value(partial_sort(policy, input, input.begin() + 2, less,
                     proj)) == input.end());
        HPX_TEST_EQ(input[0].number, 1);
        HPX_TEST_EQ(input[1].number, 2);
        HPX_TEST(value(nth_element(policy, input, input.begin() + 2, less,
                     proj)) == input.end());
        HPX_TEST_EQ(input[2].number, 2);

        std::vector<int> numbers{1, 2, 3, 4, 5};
        auto shifted = value(shift_left(policy, numbers, 2));
        HPX_TEST(shifted.begin() == numbers.begin());
        HPX_TEST(shifted.end() == numbers.begin() + 3);
        HPX_TEST(std::ranges::equal(shifted, std::array{3, 4, 5}));
        numbers = {1, 2, 3, 4, 5};
        shifted = value(shift_right(policy, numbers, 2));
        HPX_TEST(shifted.begin() == numbers.begin() + 2);
        HPX_TEST(shifted.end() == numbers.end());
        HPX_TEST(std::ranges::equal(shifted, std::array{1, 2, 3}));
        numbers = {1, 2, 3, 4, 5};
        auto rotated = value(rotate(policy, numbers, numbers.begin() + 2));
        HPX_TEST(rotated.begin() == numbers.begin() + 3);
        HPX_TEST(rotated.end() == numbers.end());
        HPX_TEST(std::ranges::equal(numbers, std::array{3, 4, 5, 1, 2}));
        HPX_TEST(value(fill(policy, numbers, 7)) == numbers.end());
        HPX_TEST(std::ranges::all_of(numbers, [](int n) { return n == 7; }));
        HPX_TEST(value(generate(policy, numbers, [] { return 9; })) ==
            numbers.end());
        HPX_TEST(std::ranges::all_of(numbers, [](int n) { return n == 9; }));
        auto dangling = value(sort(policy, std::vector<int>{3, 2, 1}));
        static_assert(
            std::is_same_v<decltype(dangling), std::ranges::dangling>);
        auto dangling_range =
            value(hpx::ranges::remove(policy, std::vector<int>{3, 2}, 2));
        static_assert(
            std::is_same_v<decltype(dangling_range), std::ranges::dangling>);
    }
}    // namespace

int hpx_main()
{
    using namespace hpx::execution;
    test(seq);
    test(par);
    test(par_unseq);
    test(seq(task));
    test(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
