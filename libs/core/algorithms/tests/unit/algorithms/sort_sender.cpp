//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 1007, 131073})
    {
        auto data = sender_test::values(n);
        auto original = data;
        auto expected = data;
        std::sort(expected.begin(), expected.end());
        auto sender = hpx::sort(policy, data.begin(), data.end());
        HPX_TEST(data == original);
        sender_test::wait(std::move(sender));
        HPX_TEST(data == expected);
        auto ranged = sender_test::wait(hpx::ranges::sort(policy, data));
        HPX_TEST(hpx::get<0>(*ranged) == data.end());
        HPX_TEST(data == expected);

        sender_test::wait(sender_test::ex::just(
                              data.begin(), data.end(), std::greater<int>{}) |
            hpx::sort(policy));
        std::reverse(expected.begin(), expected.end());
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
