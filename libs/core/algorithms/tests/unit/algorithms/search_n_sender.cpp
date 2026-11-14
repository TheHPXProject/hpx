//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

struct record
{
    int key;
};

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

                auto iterator_result = sender_test::wait(hpx::ranges::search_n(
                    policy, data.begin(), data.end(), count, value));
                HPX_TEST(hpx::get<0>(*iterator_result) == expected);

                auto range_result = sender_test::wait(
                    hpx::ranges::search_n(policy, data, count, value));
                HPX_TEST(hpx::get<0>(*range_result) == expected);
                HPX_TEST(hpx::ranges::search_n(data.begin(), data.end(), count,
                             value) == expected);
                HPX_TEST(hpx::ranges::search_n(data, count, value) == expected);
            }
        }
    }

    std::vector<int> short_values{1, 1, 2};
    auto const huge_count = (std::numeric_limits<std::size_t>::max)();
    HPX_TEST(hpx::ranges::search_n(short_values, huge_count, 1) ==
        short_values.end());
    auto huge_result = sender_test::wait(
        hpx::ranges::search_n(policy, short_values, huge_count, 1));
    HPX_TEST(hpx::get<0>(*huge_result) == short_values.end());

    std::vector<record> records{{1}, {7}, {7}, {2}};
    HPX_TEST(hpx::ranges::search_n(records, 2, record{7},
                 hpx::ranges::equal_to{}, &record::key) == records.begin() + 1);
    auto projected_result = sender_test::wait(hpx::ranges::search_n(
        policy, records, 2, record{7}, hpx::ranges::equal_to{}, &record::key));
    HPX_TEST(hpx::get<0>(*projected_result) == records.begin() + 1);
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
