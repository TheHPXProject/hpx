//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 2, 17, 10007})
    {
        std::vector<int> keys(n), values(n, 1), out_keys(n, -1),
            out_values(n, -1);
        int i = 0;
        std::generate(
            keys.begin(), keys.end(), [&i] { return (i++ / 4) % 17; });
        auto sender = hpx::experimental::reduce_by_key(policy, keys.begin(),
            keys.end(), values.begin(), out_keys.begin(), out_values.begin());
        HPX_TEST(std::all_of(out_values.begin(), out_values.end(),
            [](int value) { return value == -1; }));
        auto result = sender_test::wait(std::move(sender));
        auto count = (n + 3) / 4;
        HPX_TEST(hpx::get<0>(*result).in == out_keys.begin() + count);
        HPX_TEST(hpx::get<0>(*result).out == out_values.begin() + count);
        for (std::size_t j = 0; j != count; ++j)
        {
            HPX_TEST_EQ(out_keys[j], static_cast<int>(j % 17));
            HPX_TEST_EQ(out_values[j],
                static_cast<int>((std::min) (std::size_t(4), n - j * 4)));
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
