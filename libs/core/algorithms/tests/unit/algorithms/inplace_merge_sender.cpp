//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007, 131073})
    {
        auto data = sender_test::values(n);
        auto middle = data.begin() + std::ssize(data) / 2;
        std::sort(data.begin(), middle);
        std::sort(middle, data.end());
        auto original = data;
        auto expected = data;
        std::inplace_merge(expected.begin(),
            expected.begin() + std::ssize(expected) / 2, expected.end());
        auto sender =
            hpx::inplace_merge(policy, data.begin(), middle, data.end());
        HPX_TEST(data == original);
        sender_test::wait(std::move(sender));
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
