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
    for (std::size_t n : {0, 1, 17, 10007, 131073})
    {
        auto left = sender_test::values(n);
        auto right = sender_test::values(n / 2);
        std::sort(left.begin(), left.end());
        std::sort(right.begin(), right.end());
        std::vector<int> output(left.size() + right.size(), -1);
        auto expected = output;
        std::merge(left.begin(), left.end(), right.begin(), right.end(),
            expected.begin());
        auto sender = hpx::merge(policy, left.begin(), left.end(),
            right.begin(), right.end(), output.begin());
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(hpx::get<0>(*result) == output.end());
        HPX_TEST(output == expected);
        std::fill(output.begin(), output.end(), -1);
        auto ranged = sender_test::wait(
            hpx::ranges::merge(policy, left, right, output.begin()));
        HPX_TEST(hpx::get<0>(*ranged).in1 == left.end());
        HPX_TEST(hpx::get<0>(*ranged).in2 == right.end());
        HPX_TEST(hpx::get<0>(*ranged).out == output.end());
        HPX_TEST(output == expected);
    }
}

int hpx_main()
{
    sender_test::policies([](auto policy) { test(policy); });
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
