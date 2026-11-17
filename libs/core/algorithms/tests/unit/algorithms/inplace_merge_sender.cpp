//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <ranges>
#include <utility>

struct equality_sentinel
{
    int* end = nullptr;
    std::atomic<int>* comparisons = nullptr;

    friend bool operator==(int* it, equality_sentinel sentinel) noexcept
    {
        if (sentinel.comparisons != nullptr)
            sentinel.comparisons->fetch_add(1, std::memory_order_relaxed);
        return it == sentinel.end;
    }

    friend bool operator==(equality_sentinel sentinel, int* it) noexcept
    {
        if (sentinel.comparisons != nullptr)
            sentinel.comparisons->fetch_add(1, std::memory_order_relaxed);
        return it == sentinel.end;
    }
};

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007, 131073})
    {
        auto data = sender_test::values(n);
        auto const middle_offset = static_cast<std::ptrdiff_t>(n / 2);
        auto middle = data.begin() + middle_offset;
        std::sort(data.begin(), middle);
        std::sort(middle, data.end());
        auto original = data;
        auto expected = data;
        std::inplace_merge(
            expected.begin(), expected.begin() + middle_offset, expected.end());
        auto sender =
            hpx::inplace_merge(policy, data.begin(), middle, data.end());
        HPX_TEST(data == original);
        sender_test::wait(std::move(sender));
        HPX_TEST(data == expected);
    }
}

void test_distinct_sentinel()
{
    using namespace hpx::execution;
    auto async = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));

    int sender_data[]{1, 3, 2, 4};
    std::atomic<int> sender_comparisons{0};
    auto sender = hpx::ranges::inplace_merge(par(task).on(async), sender_data,
        sender_data + 2,
        equality_sentinel{sender_data + 4, &sender_comparisons});
    HPX_TEST_EQ(sender_comparisons.load(), 0);
    auto sender_result = sender_test::wait(std::move(sender));
    HPX_TEST(sender_comparisons.load() > 0);
    HPX_TEST(hpx::get<0>(*sender_result) == sender_data + 4);
    HPX_TEST(std::is_sorted(sender_data, sender_data + 4));

    int range_data[]{1, 3, 2, 4};
    std::atomic<int> range_comparisons{0};
    auto range = std::ranges::subrange(
        range_data, equality_sentinel{range_data + 4, &range_comparisons});
    auto range_sender =
        hpx::ranges::inplace_merge(par(task).on(async), range, range_data + 2);
    HPX_TEST_EQ(range_comparisons.load(), 0);
    auto range_result = sender_test::wait(std::move(range_sender));
    HPX_TEST(range_comparisons.load() > 0);
    HPX_TEST(hpx::get<0>(*range_result) == range_data + 4);
    HPX_TEST(std::is_sorted(range_data, range_data + 4));

    int sequential_data[]{1, 3, 2, 4};
    auto sequential_result = hpx::ranges::inplace_merge(sequential_data,
        sequential_data + 2, equality_sentinel{sequential_data + 4});
    HPX_TEST(sequential_result == sequential_data + 4);
    HPX_TEST(std::is_sorted(sequential_data, sequential_data + 4));

    int sequential_range_data[]{1, 3, 2, 4};
    auto sequential_range = std::ranges::subrange(
        sequential_range_data, equality_sentinel{sequential_range_data + 4});
    auto sequential_range_result =
        hpx::ranges::inplace_merge(sequential_range, sequential_range_data + 2);
    HPX_TEST(sequential_range_result == sequential_range_data + 4);
    HPX_TEST(std::is_sorted(sequential_range_data, sequential_range_data + 4));
}

int hpx_main()
{
    sender_test::policies([](auto policy) { test(policy); });
    test_distinct_sentinel();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
