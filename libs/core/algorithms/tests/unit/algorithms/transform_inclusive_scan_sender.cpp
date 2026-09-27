//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

struct twice
{
    int operator()(int value) const
    {
        return value * 2;
    }
};

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007})
    {
        auto data = sender_test::values(n);
        std::vector<int> output(n, -1), expected(n, -1);
        std::transform_inclusive_scan(data.begin(), data.end(),
            expected.begin(), std::plus<int>{}, twice{}, 17);
        auto sender = hpx::transform_inclusive_scan(policy, data.begin(),
            data.end(), output.begin(), std::plus<int>{}, twice{}, 17);
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(hpx::get<0>(*result) == output.end());
        HPX_TEST(output == expected);

        std::fill(output.begin(), output.end(), -1);
        std::transform_inclusive_scan(data.begin(), data.end(),
            expected.begin(), std::plus<int>{}, twice{});
        auto noinit = hpx::transform_inclusive_scan(policy, data.begin(),
            data.end(), output.begin(), std::plus<int>{}, twice{});
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto noinit_result = sender_test::wait(std::move(noinit));
        HPX_TEST(hpx::get<0>(*noinit_result) == output.end());
        HPX_TEST(output == expected);

        auto inplace = data;
        sender_test::wait(hpx::transform_inclusive_scan(policy, inplace.begin(),
            inplace.end(), inplace.begin(), std::plus<int>{}, twice{}));
        HPX_TEST(inplace == expected);
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
