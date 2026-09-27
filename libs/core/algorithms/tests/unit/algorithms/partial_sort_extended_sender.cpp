//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007, 131073})
    {
        for (std::size_t position : {std::size_t(0), n / 2, n})
        {
            auto data = sender_test::values(n);
            auto original = data;
            auto expected = data;
            std::sort(expected.begin(), expected.end());
            auto middle = data.begin() + position;
            auto sender =
                hpx::partial_sort(policy, data.begin(), middle, data.end());
            HPX_TEST(data == original);
            sender_test::wait(std::move(sender));
            HPX_TEST(std::equal(data.begin(), middle, expected.begin()));
            std::sort(data.begin(), data.end());
            HPX_TEST(data == expected);
        }
    }
    // A sorted prefix alone does not permit the algorithm to return early:
    // the suffix can contain smaller elements that belong in the prefix.
    std::vector<int> data(4096);
    auto middle = data.begin() + 2048;
    std::iota(data.begin(), middle, 0);
    std::iota(middle, data.end(), -2048);
    auto expected = data;
    std::sort(expected.begin(), expected.end());
    sender_test::wait(
        hpx::partial_sort(policy, data.begin(), middle, data.end()));
    HPX_TEST(std::equal(data.begin(), middle, expected.begin()));
    std::sort(data.begin(), data.end());
    HPX_TEST(data == expected);
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
