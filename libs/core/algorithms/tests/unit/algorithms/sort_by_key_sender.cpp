//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 131073})
    {
        auto keys = sender_test::values(n);
        auto original_keys = keys;
        std::vector<int> ids(n);
        std::iota(ids.begin(), ids.end(), 0);
        auto original_ids = ids;
        auto sender = hpx::experimental::sort_by_key(
            policy, keys.begin(), keys.end(), ids.begin());
        HPX_TEST(keys == original_keys);
        HPX_TEST(ids == original_ids);
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(hpx::get<0>(*result).first == keys.end());
        HPX_TEST(hpx::get<0>(*result).second == ids.end());
        HPX_TEST(std::is_sorted(keys.begin(), keys.end()));
        for (std::size_t i = 0; i != n; ++i)
            HPX_TEST_EQ(keys[i], original_keys[ids[i]]);
        std::sort(ids.begin(), ids.end());
        HPX_TEST(ids == original_ids);
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
