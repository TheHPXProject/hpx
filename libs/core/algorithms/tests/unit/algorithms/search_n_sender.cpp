//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

struct record
{
    int key;
    bool query = false;
};

struct moving_projection
{
    std::unique_ptr<int> key = std::make_unique<int>(7);

    moving_projection() = default;

    moving_projection(moving_projection const& other)
      : key(std::make_unique<int>(*other.key))
    {
    }

    moving_projection(moving_projection&&) noexcept = default;

    int const& operator()(record const& value) const
    {
        return value.query ? *key : value.key;
    }
};

struct record_with_unique_key
{
    std::unique_ptr<int> key;

    explicit record_with_unique_key(int value)
      : key(std::make_unique<int>(value))
    {
    }

    record_with_unique_key(record_with_unique_key const& other)
      : key(std::make_unique<int>(*other.key))
    {
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

                auto iterator_result = sender_test::wait(hpx::ranges::search_n(
                    policy, data.begin(), data.end(), count, value));
                HPX_TEST(hpx::get<0>(*iterator_result) == expected);

                auto range_result = sender_test::wait(
                    hpx::ranges::search_n(policy, data, count, value));
                HPX_TEST(hpx::get<0>(*range_result) == expected);
                HPX_TEST(hpx::ranges::search_n(data.begin(), data.end(), count,
                             value) == expected);
                HPX_TEST(hpx::ranges::search_n(data, count, value) == expected);
            }
        }
    }

    std::vector<int> short_values{1, 1, 2};
    auto const huge_count = (std::numeric_limits<std::size_t>::max)();
    HPX_TEST(hpx::ranges::search_n(short_values, huge_count, 1) ==
        short_values.end());
    auto huge_result = sender_test::wait(
        hpx::ranges::search_n(policy, short_values, huge_count, 1));
    HPX_TEST(hpx::get<0>(*huge_result) == short_values.end());

    std::vector<record> records{{1}, {7}, {7}, {2}};
    HPX_TEST(hpx::ranges::search_n(records, 2, record{7},
                 hpx::ranges::equal_to{}, &record::key) == records.begin() + 1);
    auto projected_result = sender_test::wait(hpx::ranges::search_n(
        policy, records, 2, record{7}, hpx::ranges::equal_to{}, &record::key));
    HPX_TEST(hpx::get<0>(*projected_result) == records.begin() + 1);
    auto moving_projection_sender = hpx::ranges::search_n(policy, records, 2,
        record{0, true}, hpx::ranges::equal_to{}, moving_projection{});
    auto moving_projection_result =
        sender_test::wait(std::move(moving_projection_sender));
    HPX_TEST(hpx::get<0>(*moving_projection_result) == records.begin() + 1);

    std::vector<record_with_unique_key> unique_key_records;
    for (int key : {1, 7, 7, 2})
        unique_key_records.emplace_back(key);
    record_with_unique_key value(7);
    auto compare_keys = [](auto const& left, auto const& right) {
        return *left == *right;
    };
    HPX_TEST(
        hpx::ranges::search_n(unique_key_records, 2, value, compare_keys,
            &record_with_unique_key::key) == unique_key_records.begin() + 1);
    auto unique_key_result =
        sender_test::wait(hpx::ranges::search_n(policy, unique_key_records, 2,
            value, compare_keys, &record_with_unique_key::key));
    HPX_TEST(hpx::get<0>(*unique_key_result) == unique_key_records.begin() + 1);
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
