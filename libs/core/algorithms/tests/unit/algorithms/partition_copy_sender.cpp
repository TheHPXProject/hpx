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
        std::vector<int> yes(n, -1), no(n, -1), expected_yes(n, -1),
            expected_no(n, -1);
        auto pred = [](int value) { return value % 3 == 0; };
        auto expected = std::partition_copy(data.begin(), data.end(),
            expected_yes.begin(), expected_no.begin(), pred);
        auto result = sender_test::wait(hpx::partition_copy(
            policy, data.begin(), data.end(), yes.begin(), no.begin(), pred));
        auto actual = hpx::get<0>(*result);
        HPX_TEST(actual.first - yes.begin() ==
            expected.first - expected_yes.begin());
        HPX_TEST(actual.second - no.begin() ==
            expected.second - expected_no.begin());
        HPX_TEST(yes == expected_yes);
        HPX_TEST(no == expected_no);
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
