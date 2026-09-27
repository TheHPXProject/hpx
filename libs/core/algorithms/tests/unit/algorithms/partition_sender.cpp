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
        auto data = sender_test::values(n);
        auto original = data;
        auto expected = data;
        auto pred = [](int value) { return value % 3 == 0; };
        auto expected_end =
            std::partition(expected.begin(), expected.end(), pred);
        auto sender = hpx::partition(policy, data.begin(), data.end(), pred);
        HPX_TEST(data == original);
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(hpx::get<0>(*result) - data.begin() ==
            expected_end - expected.begin());
        HPX_TEST(std::is_partitioned(data.begin(), data.end(), pred));
        std::sort(data.begin(), data.end());
        std::sort(expected.begin(), expected.end());
        HPX_TEST(data == expected);
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
