//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

template <typename Policy>
void test(Policy policy)
{
    auto test_empty_signed_bounds = [policy](int first, int last) {
        int calls = 0;
        int induction_value = 17;
        int reduction_value = 42;
        auto sender = hpx::experimental::for_loop(policy, first, last,
            hpx::experimental::induction(induction_value),
            hpx::experimental::reduction_plus(reduction_value),
            [&calls](int, int, int&) { ++calls; });
        sender_test::wait(std::move(sender));

        HPX_TEST_EQ(calls, 0);
        HPX_TEST_EQ(induction_value, 17);
        HPX_TEST_EQ(reduction_value, 42);
    };

    test_empty_signed_bounds(5, 5);
    test_empty_signed_bounds(10, 5);

    for (std::size_t n : {0, 1, 17, 10007})
    {
        std::vector<int> data(n, 0);
        auto sender = hpx::experimental::for_loop(
            policy, data.begin(), data.end(), [](auto it) { *it = 42; });
        HPX_TEST(std::all_of(
            data.begin(), data.end(), [](int value) { return value == 0; }));
        sender_test::wait(std::move(sender));
        HPX_TEST(std::all_of(
            data.begin(), data.end(), [](int value) { return value == 42; }));
    }

    namespace ex = hpx::execution::experimental;
    std::vector<int> data(17, 0);
    auto range_sender = hpx::ranges::experimental::for_loop(
        policy, data, [](auto it) { *it = 1; });
    static_assert(ex::is_sender_v<decltype(range_sender)>);
    sender_test::wait(std::move(range_sender));
    HPX_TEST(std::all_of(
        data.begin(), data.end(), [](int value) { return value == 1; }));

    std::fill(data.begin(), data.end(), 0);
    auto iterator_sender = hpx::ranges::experimental::for_loop(
        policy, data.begin(), data.end(), [](auto it) { *it = 2; });
    static_assert(ex::is_sender_v<decltype(iterator_sender)>);
    sender_test::wait(std::move(iterator_sender));
    HPX_TEST(std::all_of(
        data.begin(), data.end(), [](int value) { return value == 2; }));

    std::fill(data.begin(), data.end(), 0);
    auto strided_range_sender = hpx::ranges::experimental::for_loop_strided(
        policy, data, 3, [](auto it) { *it = 3; });
    static_assert(ex::is_sender_v<decltype(strided_range_sender)>);
    sender_test::wait(std::move(strided_range_sender));
    for (std::size_t i = 0; i != data.size(); ++i)
    {
        HPX_TEST_EQ(data[i], i % 3 == 0 ? 3 : 0);
    }

    std::fill(data.begin(), data.end(), 0);
    auto strided_iterator_sender = hpx::ranges::experimental::for_loop_strided(
        policy, data.begin(), data.end(), 4, [](auto it) { *it = 4; });
    static_assert(ex::is_sender_v<decltype(strided_iterator_sender)>);
    sender_test::wait(std::move(strided_iterator_sender));
    for (std::size_t i = 0; i != data.size(); ++i)
    {
        HPX_TEST_EQ(data[i], i % 4 == 0 ? 4 : 0);
    }
}

int hpx_main()
{
    auto test_empty_signed_bounds = [](int first, int last) {
        int calls = 0;
        int induction_value = 17;
        int reduction_value = 42;
        hpx::experimental::for_loop(hpx::execution::par, first, last,
            hpx::experimental::induction(induction_value),
            hpx::experimental::reduction_plus(reduction_value),
            [&calls](int, int, int&) { ++calls; });

        HPX_TEST_EQ(calls, 0);
        HPX_TEST_EQ(induction_value, 17);
        HPX_TEST_EQ(reduction_value, 42);
    };

    test_empty_signed_bounds(5, 5);
    test_empty_signed_bounds(10, 5);
    sender_test::policies([](auto policy) { test(policy); });
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
