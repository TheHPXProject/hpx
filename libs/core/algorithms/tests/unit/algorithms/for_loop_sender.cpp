//  Copyright (c) 2026 the-ivii
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
