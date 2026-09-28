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
#include <atomic>
#include <cstddef>
#include <forward_list>
#include <functional>
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

    template <typename Policy>
    void test(Policy policy)
    {
        std::vector<record> input{{0}, {1}, {1}, {1}, {2}, {1}, {1}};
        std::vector<int> needle{1, 1};
        auto found = value(hpx::ranges::search(
            policy, input, needle, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.begin() + 1);
        HPX_TEST(found.end() == input.begin() + 3);
        needle.clear();
        found = value(hpx::ranges::search(
            policy, input, needle, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.begin());
        HPX_TEST(found.empty());
        needle.push_back(9);
        found = value(hpx::ranges::search(
            policy, input, needle, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.end());
        HPX_TEST(found.empty());
        auto dangling =
            value(hpx::ranges::search(policy, std::vector<int>{1, 2}, needle));
        static_assert(
            std::is_same_v<decltype(dangling), std::ranges::dangling>);

        found = value(hpx::ranges::search_n(
            policy, input, 3, {1}, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.begin() + 1);
        HPX_TEST(found.size() == 3);
        found = value(hpx::ranges::search_n(policy, input.begin(), input.end(),
            2, 1, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.begin() + 1);
        HPX_TEST(found.size() == 2);
        found = value(hpx::ranges::search_n(
            policy, input, -1, 1, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.begin());
        HPX_TEST(found.empty());
        found = value(hpx::ranges::search_n(
            policy, input, 8, 1, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.end());
        HPX_TEST(found.empty());
        found = value(hpx::ranges::search_n(
            policy, input, 4, 1, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.end());
        HPX_TEST(found.empty());
        input.clear();
        found = value(hpx::ranges::search_n(
            policy, input, 0, 1, std::ranges::equal_to{}, &record::number));
        HPX_TEST(found.begin() == input.end());
        HPX_TEST(found.empty());

        using search = decltype(hpx::ranges::search);
        static_assert(!std::is_invocable_v<search, Policy,
            std::forward_list<int>&, std::vector<int>&>);
        using search_n = decltype(hpx::ranges::search_n);
        static_assert(!std::is_invocable_v<search_n, Policy,
            std::forward_list<int>&, int, int>);
    }

    void test_counted_subsequence_extension()
    {
        std::vector<int> input{1, 2, 3, 2, 3};
        std::vector<int> needle{2, 3};
        auto verify_result = [&](auto count, auto expected) {
            HPX_TEST(hpx::ranges::search_n(input, count, needle) == expected);
            HPX_TEST(hpx::ranges::search_n(input.begin(), count, needle.begin(),
                         needle.end()) == expected);
            auto with_policy = [&](auto policy) {
                HPX_TEST(value(hpx::ranges::search_n(
                             policy, input, count, needle)) == expected);
                HPX_TEST(value(hpx::ranges::search_n(policy, input.begin(),
                             count, needle.begin(), needle.end())) == expected);
            };
            using namespace hpx::execution;
            with_policy(seq);
            with_policy(par);
            with_policy(seq(task));
            with_policy(par(task));
        };
        verify_result(input.size(), input.begin() + 1);
        verify_result(
            std::integral_constant<std::size_t, 5>{}, input.begin() + 1);
        verify_result(
            2, input.begin());    // A match outside the counted prefix.
        needle = {9};
        verify_result(input.size(), input.begin());
        needle.clear();
        verify_result(input.size(), input.begin());
        verify_result(0, input.begin());
        needle = {2, 3};
        verify_result(0, input.begin());
    }

    template <typename Policy>
    void test_counted_search_returns_before_completion(Policy policy)
    {
        std::vector<int> input{1, 2, 3, 4};
        std::vector<int> needle{2, 3};
        hpx::promise<void> release;
        auto gate = release.get_future().share();
        auto project = [gate](int element) {
            gate.get();
            return element;
        };
        auto result = hpx::ranges::search_n(policy, input, input.size(), needle,
            std::ranges::equal_to{}, project);
        release.set_value();
        HPX_TEST(result.get() == input.begin() + 1);
    }

    void test_partition_boundaries()
    {
        auto policy = hpx::execution::par.with(
            hpx::execution::experimental::static_chunk_size(4));
        std::vector<int> input(100, 0);
        std::fill(input.begin() + 3, input.begin() + 14, 1);
        std::atomic<std::size_t> comparisons{0};
        auto pred = [&comparisons](int a, int b) {
            ++comparisons;
            return a == b;
        };
        auto found = hpx::ranges::search_n(policy, input, 9, 1, pred);
        HPX_TEST(found.begin() == input.begin() + 3);
        HPX_TEST(found.end() == input.begin() + 12);
        HPX_TEST_LTE(comparisons.load(), input.size());
        comparisons = 0;
        found = hpx::ranges::search_n(policy, input, 12, 1, pred);
        HPX_TEST(found.begin() == input.end());
        HPX_TEST(found.empty());
        HPX_TEST_LTE(comparisons.load(), input.size());
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
    test_partition_boundaries();
    test_counted_subsequence_extension();
    test_counted_search_returns_before_completion(seq(task));
    test_counted_search_returns_before_completion(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
