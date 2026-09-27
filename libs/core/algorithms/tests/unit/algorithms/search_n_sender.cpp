//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007})
    {
        std::vector<int> data(n, 1);
        std::fill(data.begin() + n / 3, data.begin() + 2 * n / 3, 7);
        for (std::size_t count :
            {std::size_t(0), std::size_t(1), std::size_t(3), n + 1})
        {
            for (int value : {1, 7, 42})
            {
                auto expected =
                    std::search_n(data.begin(), data.end(), count, value);
                auto result = sender_test::wait(hpx::search_n(
                    policy, data.begin(), data.end(), count, value));
                HPX_TEST(hpx::get<0>(*result) == expected);
            }
        }
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
