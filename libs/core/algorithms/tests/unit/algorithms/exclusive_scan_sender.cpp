//  Copyright (c) 2026 Pratyksh Gupta
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
        std::vector<int> output(n, -1), expected(n, -1);
        std::exclusive_scan(
            data.begin(), data.end(), expected.begin(), 17, std::plus<int>{});
        auto sender = hpx::exclusive_scan(policy, data.begin(), data.end(),
            output.begin(), 17, std::plus<int>{});
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(hpx::get<0>(*result) == output.end());
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
