//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 1007, 131073})
    {
        auto data = sender_test::values(n);
        auto original = data;
        auto expected = data;
        std::sort(expected.begin(), expected.end());
        auto sender = hpx::stable_sort(policy, data.begin(), data.end());
        HPX_TEST(data == original);
        sender_test::wait(std::move(sender));
        HPX_TEST(data == expected);
        sender_test::wait(sender_test::ex::just(
                              data.begin(), data.end(), std::greater<int>{}) |
            hpx::stable_sort(policy));
        std::reverse(expected.begin(), expected.end());
        HPX_TEST(data == expected);
    }
    // Equal keys must retain their original order across recursive merges.
    std::vector<std::pair<int, std::size_t>> records;
    for (std::size_t i = 0; i != 131073; ++i)
        records.emplace_back(static_cast<int>(i % 23), i);
    auto original_records = records;
    auto expected = records;
    auto less_key = [](auto const& left, auto const& right) {
        return left.first < right.first;
    };
    std::stable_sort(expected.begin(), expected.end(), less_key);
    sender_test::wait(
        hpx::stable_sort(policy, records.begin(), records.end(), less_key));
    HPX_TEST(records == expected);
    records = original_records;
    auto ranged = sender_test::wait(hpx::ranges::stable_sort(policy, records,
        std::less<int>{}, &std::pair<int, std::size_t>::first));
    HPX_TEST(hpx::get<0>(*ranged) == records.end());
    HPX_TEST(records == expected);
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
