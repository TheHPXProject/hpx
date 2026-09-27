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
        std::vector<int> data(n);
        int i = 0;
        std::generate(data.begin(), data.end(), [&i] { return i++ / 3; });
        std::vector<int> output(n, -1), expected(n, -1);
        auto end = std::unique_copy(data.begin(), data.end(), expected.begin());
        auto sender =
            hpx::unique_copy(policy, data.begin(), data.end(), output.begin());
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(
            hpx::get<0>(*result) - output.begin() == end - expected.begin());
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
