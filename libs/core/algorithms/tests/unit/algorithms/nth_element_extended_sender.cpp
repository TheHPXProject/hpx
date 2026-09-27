//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007})
    {
        for (std::size_t position : {std::size_t(0), n / 2, n})
        {
            auto data = sender_test::values(n);
            auto original = data;
            auto expected = data;
            std::sort(expected.begin(), expected.end());
            auto nth = data.begin() + position;
            auto sender =
                hpx::nth_element(policy, data.begin(), nth, data.end());
            HPX_TEST(data == original);
            sender_test::wait(std::move(sender));
            if (position < n)
            {
                HPX_TEST_EQ(*nth, expected[position]);
                HPX_TEST(std::all_of(data.begin(), nth,
                    [nth](int value) { return value <= *nth; }));
                HPX_TEST(std::all_of(nth, data.end(),
                    [nth](int value) { return value >= *nth; }));
            }
            std::sort(data.begin(), data.end());
            HPX_TEST(data == expected);
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
