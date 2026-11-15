//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

struct move_sensitive_pred
{
    std::shared_ptr<int> state = std::make_shared<int>(1);

    bool operator()(int lhs, int rhs) const
    {
        return state != nullptr && lhs == rhs;
    }
};

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007})
    {
        std::vector<int> data(n, 1);
        auto const first_offset = static_cast<std::ptrdiff_t>(n / 3);
        auto const last_offset = static_cast<std::ptrdiff_t>(2 * n / 3);
        std::fill(data.begin() + first_offset, data.begin() + last_offset, 7);
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

    std::vector<int> multi_chunk(10007, 1);
    multi_chunk[multi_chunk.size() - 2] = 7;
    multi_chunk.back() = 7;
    auto result = sender_test::wait(hpx::search_n(policy, multi_chunk.begin(),
        multi_chunk.end(), 2, 7, move_sensitive_pred{}));
    HPX_TEST(hpx::get<0>(*result) == multi_chunk.end() - 2);
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
