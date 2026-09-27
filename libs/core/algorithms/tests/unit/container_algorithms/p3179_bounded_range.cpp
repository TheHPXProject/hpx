//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <forward_list>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    template <typename I>
    struct sentinel
    {
        I end;

        friend bool operator==(I it, sentinel s)
        {
            return it == s.end;
        }
    };

    template <typename I>
    struct sized_sentinel : sentinel<I>
    {
        friend std::iter_difference_t<I> operator-(sized_sentinel s, I it)
        {
            return s.end - it;
        }

        friend std::iter_difference_t<I> operator-(I it, sized_sentinel s)
        {
            return it - s.end;
        }
    };

    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else if constexpr (hpx::execution::experimental::is_sender_v<
                               std::decay_t<T>>)
        {
            auto completed = hpx::this_thread::experimental::sync_wait(
                HPX_FORWARD(T, result));
            HPX_TEST(completed.has_value());
            return hpx::get<0>(HPX_MOVE(completed.value()));
        }
        else
            return HPX_FORWARD(T, result);
    }

    template <typename Policy>
    void test_transfer(Policy policy)
    {
        using vector = std::vector<int>;
        using iterator = vector::iterator;
        vector input{1, 2, 3, 4, 5};
        vector output(3, -1);
        using algorithm = decltype(hpx::ranges::copy);
        static_assert(!std::is_invocable_v<algorithm, Policy,
            std::forward_list<int>&, vector&>);
        static_assert(!std::is_invocable_v<algorithm, Policy, iterator,
            sentinel<iterator>, iterator, iterator>);
        static_assert(!std::is_invocable_v<algorithm, Policy, iterator,
            iterator, iterator, sentinel<iterator>>);
        static_assert(
            !std::is_invocable_v<algorithm, Policy, vector&, vector const&>);

        auto result = value(hpx::ranges::copy(policy, input, output));
        HPX_TEST(result.in == input.begin() + 3);
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 3}));
        auto const last = sized_sentinel<iterator>{{input.end()}};
        auto const out_last = sized_sentinel<iterator>{{output.end()}};
        result = value(hpx::ranges::copy(
            policy, input.begin(), last, output.begin(), out_last));
        HPX_TEST(result.in == input.begin() + 3);
        result = value(hpx::ranges::copy_n(
            policy, input.begin(), 5, output.begin(), out_last));
        HPX_TEST(result.in == input.begin() + 3);
        result = value(hpx::ranges::copy_n(
            policy, input.begin(), -1, output.begin(), out_last));
        HPX_TEST(result.in == input.begin());
        HPX_TEST(result.out == output.begin());

        // Sized ranges need not have sized sentinels.
        auto source = std::ranges::subrange<iterator, sentinel<iterator>,
            std::ranges::subrange_kind::sized>(
            input.begin(), {input.end()}, input.size());
        auto destination = std::ranges::subrange<iterator, sentinel<iterator>,
            std::ranges::subrange_kind::sized>(
            output.begin(), {output.end()}, output.size());
        result = value(hpx::ranges::copy(policy, source, destination));
        HPX_TEST(result.in == input.begin() + 3);
        auto dangling = value(hpx::ranges::copy(policy, vector{7, 8}, output));
        static_assert(
            std::is_same_v<decltype(dangling.in), std::ranges::dangling>);
        HPX_TEST(dangling.out == output.begin() + 2);
        auto dangling_out = value(hpx::ranges::copy(policy, input, vector(2)));
        static_assert(
            std::is_same_v<decltype(dangling_out.out), std::ranges::dangling>);
        HPX_TEST(dangling_out.in == input.begin() + 2);

        vector empty;
        result = value(hpx::ranges::copy(policy, input, empty));
        HPX_TEST(result.in == input.begin());
        HPX_TEST(result.out == empty.end());
        result = value(hpx::ranges::copy(policy, empty, output));
        HPX_TEST(result.in == empty.end());
        HPX_TEST(result.out == output.begin());

        std::vector<std::unique_ptr<int>> movable;
        movable.push_back(std::make_unique<int>(42));
        movable.push_back(std::make_unique<int>(43));
        std::vector<std::unique_ptr<int>> moved(1);
        auto m = value(hpx::ranges::move(policy, movable, moved));
        HPX_TEST(m.in == movable.begin() + 1);
        HPX_TEST(m.out == moved.end());
        HPX_TEST(!movable[0]);
        HPX_TEST_EQ(*movable[1], 43);
        HPX_TEST_EQ(*moved[0], 42);
    }

    struct record
    {
        int number;
    };

    template <typename Policy>
    void test_transform(Policy policy)
    {
        std::vector<record> input{{1}, {2}, {3}, {4}};
        std::vector<int> output(2);
        auto twice = [](int n) { return 2 * n; };
        auto unary = value(hpx::ranges::transform(
            policy, input, output, twice, &record::number));
        HPX_TEST(unary.in == input.begin() + 2);
        HPX_TEST(unary.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{2, 4}));
        unary = value(hpx::ranges::transform(policy, input.begin(), input.end(),
            output.begin(), output.end(), twice, &record::number));
        HPX_TEST(unary.in == input.begin() + 2);
        std::vector<int> second{10};
        auto binary = value(hpx::ranges::transform(
            policy, input, second, output, std::plus<>{}, &record::number));
        HPX_TEST(binary.in1 == input.begin() + 1);
        HPX_TEST(binary.in2 == second.end());
        HPX_TEST(binary.out == output.begin() + 1);
        HPX_TEST_EQ(output[0], 11);
        HPX_TEST_EQ(output[1], 4);
        binary = value(hpx::ranges::transform(policy, input.begin(),
            input.end(), second.begin(), second.end(), output.begin(),
            output.end(), std::plus<>{}, &record::number));
        HPX_TEST(binary.in1 == input.begin() + 1);
        HPX_TEST(binary.in2 == second.end());
    }

    template <typename Policy>
    void test_filtered(Policy policy)
    {
        std::vector<int> input{0, 1, 0, 2, 0, 3, 0};
        std::vector<int> output(2);
        auto selected = [](int n) { return n != 0; };
        auto result =
            value(hpx::ranges::copy_if(policy, input, output, selected));
        HPX_TEST(result.in == input.begin() + 5);
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2}));
        result = value(hpx::ranges::remove_copy_if(
            policy, input, output, [](int n) { return n == 0; }));
        HPX_TEST(result.in == input.begin() + 5);
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2}));
        output.resize(3);
        result = value(hpx::ranges::copy_if(policy, input, output, selected));
        HPX_TEST(result.in == input.end());
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 3}));
        output.clear();
        result = value(hpx::ranges::copy_if(policy, input, output, selected));
        HPX_TEST(result.in == input.begin() + 1);
        HPX_TEST(result.out == output.end());
        result = value(hpx::ranges::copy_if(
            policy, input, output, [](int) { return false; }));
        HPX_TEST(result.in == input.end());

        input = {1, 1, 2, 2, 2, 3, 3};
        output.resize(2);
        result = value(hpx::ranges::unique_copy(policy, input, output));
        HPX_TEST(result.in == input.begin() + 5);
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2}));
        output.resize(3);
        result = value(hpx::ranges::unique_copy(policy, input, output));
        HPX_TEST(result.in == input.end());
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 3}));
    }

    template <typename Policy>
    void test_partition_and_reorder(Policy policy)
    {
        std::vector<int> input{1, 3, 2, 4, 5, 6, 8};
        std::vector<int> yes(1, -1), no(3, -1);
        auto even = [](int n) { return n % 2 == 0; };
        auto split =
            value(hpx::ranges::partition_copy(policy, input, yes, no, even));
        HPX_TEST(split.in == input.begin() + 3);
        HPX_TEST(split.out1 == yes.end());
        HPX_TEST(split.out2 == no.begin() + 2);
        HPX_TEST_EQ(yes[0], 2);
        HPX_TEST(std::ranges::equal(no, std::array{1, 3, -1}));
        yes.clear();
        split =
            value(hpx::ranges::partition_copy(policy, input, yes, no, even));
        HPX_TEST(split.in == input.begin() + 2);
        HPX_TEST(split.out2 == no.begin() + 2);

        input = {1, 2, 3, 4, 5};
        std::vector<int> output(2);
        auto result = value(hpx::ranges::reverse_copy(policy, input, output));
        HPX_TEST(result.in == input.begin() + 3);
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{5, 4}));
        result = value(
            hpx::ranges::rotate_copy(policy, input, input.begin() + 3, output));
        HPX_TEST(result.in == input.begin());
        HPX_TEST(std::ranges::equal(output, std::array{4, 5}));
        output.resize(4);
        result = value(
            hpx::ranges::rotate_copy(policy, input, input.begin() + 3, output));
        HPX_TEST(result.in == input.begin() + 2);
        HPX_TEST(std::ranges::equal(output, std::array{4, 5, 1, 2}));
        output.resize(5);
        result = value(
            hpx::ranges::rotate_copy(policy, input, input.begin() + 3, output));
        HPX_TEST(result.in == input.begin() + 3);
        HPX_TEST(std::ranges::equal(output, std::array{4, 5, 1, 2, 3}));
        input.clear();
        result = value(
            hpx::ranges::rotate_copy(policy, input, input.begin(), output));
        HPX_TEST(result.in == input.end());
        HPX_TEST(result.out == output.begin());
    }

    struct value_record
    {
        int key;
        friend bool operator==(
            value_record const&, value_record const&) = default;
    };

    template <typename Policy>
    void test_default_values(Policy policy)
    {
        std::vector<value_record> input{{1}, {2}, {1}, {3}, {4}};
        std::vector<value_record> output(2);
        auto result =
            value(hpx::ranges::remove_copy(policy, input, output, {1}));
        HPX_TEST(result.in == input.begin() + 4);
        HPX_TEST_EQ(output[0].key, 2);
        HPX_TEST_EQ(output[1].key, 3);
        result =
            value(hpx::ranges::replace_copy(policy, input, output, {1}, {9}));
        HPX_TEST(result.in == input.begin() + 2);
        HPX_TEST_EQ(output[0].key, 9);
        HPX_TEST_EQ(output[1].key, 2);
        result = value(hpx::ranges::replace_copy_if(policy, input, output,
            [](value_record r) { return r.key == 2; }, {7}));
        HPX_TEST_EQ(output[1].key, 7);
        result = value(hpx::ranges::remove_copy(policy, input.begin(),
            input.end(), output.begin(), output.end(), {1}));
        HPX_TEST(result.in == input.begin() + 4);
        auto end = value(hpx::ranges::fill(policy, output, {6}));
        HPX_TEST(end == output.end());
        HPX_TEST_EQ(output[0].key, 6);
        end = value(hpx::ranges::fill_n(policy, output.begin(), 1, {8}));
        HPX_TEST(end == output.begin() + 1);
        end = value(hpx::ranges::replace(policy, output, {6}, {4}));
        HPX_TEST_EQ(output[1].key, 4);
        end = value(hpx::ranges::replace_if(
            policy, output, [](value_record r) { return r.key == 8; }, {4}));
        HPX_TEST_EQ(output[0].key, 4);
        auto tail = value(hpx::ranges::remove(policy, output, {4}));
        HPX_TEST(tail.begin() == output.begin());
    }

    template <typename Policy>
    void test_lazy_filter(Policy policy)
    {
        std::vector<int> input{1, 2, 3}, output(2);
        std::size_t calls = 0;
        auto work = hpx::ranges::copy_if(policy, input, output, [&calls](int) {
            ++calls;
            return true;
        });
        HPX_TEST_EQ(calls, std::size_t(0));
        auto result = value(HPX_MOVE(work));
        HPX_TEST(result.in == input.begin() + 2);
        HPX_TEST(result.out == output.end());
        HPX_TEST_EQ(calls, input.size());
    }

    template <typename Policy>
    void test_empty_filtered(Policy policy)
    {
        std::vector<int> input, yes(2, -1), no(2, -1);
        auto selected = [](int n) { return n % 2 == 0; };
        auto copied = value(hpx::ranges::copy_if(policy, input, yes, selected));
        HPX_TEST(copied.in == input.begin());
        HPX_TEST(copied.out == yes.begin());
        auto split = value(
            hpx::ranges::partition_copy(policy, input, yes, no, selected));
        HPX_TEST(split.in == input.begin());
        HPX_TEST(split.out1 == yes.begin());
        HPX_TEST(split.out2 == no.begin());
        HPX_TEST_EQ(yes.front(), -1);
        HPX_TEST_EQ(no.front(), -1);
    }

    template <typename Policy>
    void test(Policy policy)
    {
        test_default_values(policy);
        test_transfer(policy);
        test_transform(policy);
        test_filtered(policy);
        test_partition_and_reorder(policy);
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
    using namespace hpx::execution::experimental;
    auto exec = explicit_scheduler_executor(thread_pool_scheduler{});
    auto sender_policy = hpx::execution::par(task).on(exec);
    test_filtered(sender_policy);
    test_partition_and_reorder(sender_policy);
    test_empty_filtered(sender_policy);
    test_lazy_filter(sender_policy);
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
