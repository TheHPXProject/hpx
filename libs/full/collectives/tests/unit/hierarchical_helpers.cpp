//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using hpx::collectives::detail::checked_data_size_product;
using hpx::collectives::detail::checked_data_size_sum;
using hpx::collectives::detail::classify_site;
using hpx::collectives::detail::get_top_level_group_count;
using hpx::collectives::detail::get_top_level_group_left;
using hpx::collectives::detail::get_top_level_group_size;
using hpx::collectives::detail::is_ragged_rows_v;
using hpx::collectives::detail::is_top_level_rep;
using hpx::collectives::detail::is_uniform_rows_v;
using hpx::collectives::detail::ragged_rows;
using hpx::collectives::detail::uniform_rows;

std::size_t expected_group_count(std::size_t const n, std::size_t const arity)
{
    return arity > n ? n : arity;
}

std::size_t expected_group_size(
    std::size_t const group, std::size_t const n, std::size_t const arity)
{
    std::size_t const count = expected_group_count(n, arity);
    std::size_t const division_steps = n / count;
    std::size_t const remainder = n % count;

    return division_steps + (group < remainder ? 1 : 0);
}

std::size_t expected_group_left(
    std::size_t const group, std::size_t const n, std::size_t const arity)
{
    std::size_t const count = expected_group_count(n, arity);
    std::size_t const division_steps = n / count;
    std::size_t const remainder = n % count;

    return group * division_steps + (group < remainder ? group : remainder);
}

std::size_t expected_group_right(
    std::size_t const group, std::size_t const n, std::size_t const arity)
{
    return expected_group_left(group, n, arity) +
        expected_group_size(group, n, arity) - 1;
}

std::ptrdiff_t expected_classification(
    std::size_t const site, std::size_t const n, std::size_t const arity)
{
    if (site >= n)
    {
        return -1;
    }

    std::size_t const count = expected_group_count(n, arity);
    for (std::size_t group = 0; group != count; ++group)
    {
        if (site >= expected_group_left(group, n, arity) &&
            site <= expected_group_right(group, n, arity))
        {
            return static_cast<std::ptrdiff_t>(group);
        }
    }

    return -1;
}

void check_group(std::size_t const group, std::size_t const n,
    std::size_t const arity, std::size_t const left, std::size_t const right,
    std::size_t const size)
{
    HPX_TEST_EQ(get_top_level_group_left(group, n, arity), left);
    HPX_TEST_EQ(get_top_level_group_size(group, n, arity), size);
    HPX_TEST_EQ(left + size - 1, right);
}

struct move_only_value
{
    explicit move_only_value(int const value)
      : value(value)
    {
    }

    move_only_value(move_only_value const&) = delete;
    move_only_value& operator=(move_only_value const&) = delete;

    move_only_value(move_only_value&& other) noexcept
      : value(other.value)
    {
        other.value = -1;
    }

    move_only_value& operator=(move_only_value&&) = delete;

    int value;
};

void test_balanced_arity2()
{
    // N=8, arity=2 -> two groups of 4: [0,3] and [4,7]
    HPX_TEST_EQ(get_top_level_group_count(8, 2), static_cast<std::size_t>(2));

    check_group(0, 8, 2, 0, 3, 4);
    check_group(1, 8, 2, 4, 7, 4);
}

void test_balanced_arity4()
{
    // N=8, arity=4 -> four groups of 2: [0,1], [2,3], [4,5], [6,7]
    HPX_TEST_EQ(get_top_level_group_count(8, 4), static_cast<std::size_t>(4));

    for (std::size_t i = 0; i != 4; ++i)
    {
        check_group(i, 8, 4, i * 2, i * 2 + 1, 2);
    }
}

void test_unbalanced_n11_arity4()
{
    // N=11, arity=4 -> 11/4=2 rem 3
    // groups: [0,2](3), [3,5](3), [6,8](3), [9,10](2)
    HPX_TEST_EQ(get_top_level_group_count(11, 4), static_cast<std::size_t>(4));

    check_group(0, 11, 4, 0, 2, 3);
    check_group(1, 11, 4, 3, 5, 3);
    check_group(2, 11, 4, 6, 8, 3);
    check_group(3, 11, 4, 9, 10, 2);
}

void test_unbalanced_n7_arity3()
{
    // N=7, arity=3 -> 7/3=2 rem 1
    // groups: [0,2](3), [3,4](2), [5,6](2)
    HPX_TEST_EQ(get_top_level_group_count(7, 3), static_cast<std::size_t>(3));

    check_group(0, 7, 3, 0, 2, 3);
    check_group(1, 7, 3, 3, 4, 2);
    check_group(2, 7, 3, 5, 6, 2);
}

void test_arity_exceeds_n()
{
    // N=3, arity=8 -> clamp arity to 3, three groups of 1
    HPX_TEST_EQ(get_top_level_group_count(3, 8), static_cast<std::size_t>(3));

    for (std::size_t i = 0; i != 3; ++i)
    {
        check_group(i, 3, 8, i, i, 1);
    }
}

void test_single_site()
{
    // N=1, arity=2 -> one group of 1
    HPX_TEST_EQ(get_top_level_group_count(1, 2), static_cast<std::size_t>(1));

    check_group(0, 1, 2, 0, 0, 1);
}

void test_coverage_property()
{
    // For various N and arity, verify that the groups cover [0, N) exactly.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            std::size_t const count = get_top_level_group_count(n, arity);
            std::size_t total_size = 0;

            for (std::size_t group = 0; group != count; ++group)
            {
                std::size_t const left =
                    get_top_level_group_left(group, n, arity);
                std::size_t const size =
                    get_top_level_group_size(group, n, arity);

                total_size += size;

                if (group == 0)
                {
                    HPX_TEST_EQ(left, static_cast<std::size_t>(0));
                }
                else
                {
                    std::size_t const prev_left =
                        get_top_level_group_left(group - 1, n, arity);
                    std::size_t const prev_size =
                        get_top_level_group_size(group - 1, n, arity);
                    HPX_TEST_EQ(left, prev_left + prev_size);
                }

                if (group == count - 1)
                {
                    HPX_TEST_EQ(left + size - 1, n - 1);
                }
            }

            HPX_TEST_EQ(total_size, n);
        }
    }
}

void test_classify_site_basic()
{
    HPX_TEST_EQ(classify_site(0, 11, 4), static_cast<std::ptrdiff_t>(0));
    HPX_TEST_EQ(classify_site(1, 11, 4), static_cast<std::ptrdiff_t>(0));
    HPX_TEST_EQ(classify_site(2, 11, 4), static_cast<std::ptrdiff_t>(0));
    HPX_TEST_EQ(classify_site(3, 11, 4), static_cast<std::ptrdiff_t>(1));
    HPX_TEST_EQ(classify_site(5, 11, 4), static_cast<std::ptrdiff_t>(1));
    HPX_TEST_EQ(classify_site(6, 11, 4), static_cast<std::ptrdiff_t>(2));
    HPX_TEST_EQ(classify_site(9, 11, 4), static_cast<std::ptrdiff_t>(3));
    HPX_TEST_EQ(classify_site(10, 11, 4), static_cast<std::ptrdiff_t>(3));
    HPX_TEST_EQ(classify_site(11, 11, 4), static_cast<std::ptrdiff_t>(-1));
}

void test_classify_site_exhaustive()
{
    // Every site in [0, N) must classify to exactly one group.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            for (std::size_t site = 0; site != n; ++site)
            {
                HPX_TEST_EQ(classify_site(site, n, arity),
                    expected_classification(site, n, arity));
            }
        }
    }
}

void test_matches_recursive_fill()
{
    // Verify that the top-level helpers produce the same partition as the top
    // frame of recursively_fill_communicators.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            std::size_t const count = get_top_level_group_count(n, arity);
            HPX_TEST_EQ(count, expected_group_count(n, arity));

            for (std::size_t group = 0; group != count; ++group)
            {
                HPX_TEST_EQ(get_top_level_group_left(group, n, arity),
                    expected_group_left(group, n, arity));
                HPX_TEST_EQ(get_top_level_group_size(group, n, arity),
                    expected_group_size(group, n, arity));
            }
        }
    }
}

void test_is_top_level_rep_basic()
{
    // N=8, arity=2 -> groups [0,3] and [4,7]. Reps are 0 and 4.
    HPX_TEST(is_top_level_rep(0, 8, 2));
    HPX_TEST(!is_top_level_rep(1, 8, 2));
    HPX_TEST(!is_top_level_rep(3, 8, 2));
    HPX_TEST(is_top_level_rep(4, 8, 2));
    HPX_TEST(!is_top_level_rep(5, 8, 2));
    HPX_TEST(!is_top_level_rep(7, 8, 2));

    // N=11, arity=4 -> groups [0,2], [3,5], [6,8], [9,10]. Reps: 0,3,6,9.
    HPX_TEST(is_top_level_rep(0, 11, 4));
    HPX_TEST(!is_top_level_rep(1, 11, 4));
    HPX_TEST(!is_top_level_rep(2, 11, 4));
    HPX_TEST(is_top_level_rep(3, 11, 4));
    HPX_TEST(is_top_level_rep(6, 11, 4));
    HPX_TEST(is_top_level_rep(9, 11, 4));
    HPX_TEST(!is_top_level_rep(10, 11, 4));
}

void test_is_top_level_rep_exhaustive()
{
    // For various N and arity, exactly the leftmost site in each group
    // should be a rep, and no other site.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            for (std::size_t site = 0; site != n; ++site)
            {
                std::ptrdiff_t const group =
                    expected_classification(site, n, arity);
                bool const expected = group != -1 &&
                    site ==
                        expected_group_left(
                            static_cast<std::size_t>(group), n, arity);

                HPX_TEST_EQ(is_top_level_rep(site, n, arity), expected);
            }
        }
    }
}

void test_is_uniform_rows_trait()
{
    static_assert(is_uniform_rows_v<uniform_rows<int>>);
    static_assert(is_uniform_rows_v<uniform_rows<int> const>);
    static_assert(is_uniform_rows_v<uniform_rows<int>&>);
    static_assert(is_uniform_rows_v<uniform_rows<int> const&>);
    static_assert(!is_uniform_rows_v<int>);
    static_assert(!is_uniform_rows_v<std::vector<int>>);
}

void test_is_ragged_rows_trait()
{
    static_assert(is_ragged_rows_v<ragged_rows<int>>);
    static_assert(is_ragged_rows_v<ragged_rows<int> const>);
    static_assert(is_ragged_rows_v<ragged_rows<int>&>);
    static_assert(is_ragged_rows_v<ragged_rows<int> const&>);
    static_assert(!is_ragged_rows_v<int>);
    static_assert(!is_ragged_rows_v<std::vector<int>>);
    static_assert(std::is_nothrow_constructible_v<ragged_rows<int>,
        std::vector<int>&&, std::vector<std::size_t>&&>);
}

void test_ragged_rows_extract_columns()
{
    std::vector<ragged_rows<int>> values;
    values.emplace_back(std::vector<int>{3, 4, 103, 104, 203, 204},
        std::vector<std::size_t>{0, 0, 2, 2, 4, 4, 6});
    values.emplace_back(std::vector<int>{300, 301, 302, 400, 401, 402},
        std::vector<std::size_t>{0, 3, 3, 6, 6});

    ragged_rows<int> column_zero(values, 0);
    HPX_TEST_EQ(column_zero.data().size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(column_zero.offsets().size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(column_zero.offsets()[0], static_cast<std::size_t>(0));
    HPX_TEST_EQ(column_zero.offsets()[1], static_cast<std::size_t>(0));
    HPX_TEST_EQ(column_zero.offsets()[2], static_cast<std::size_t>(6));
    for (std::size_t i = 0; i != column_zero.data().size(); ++i)
    {
        HPX_TEST_EQ(column_zero.data()[i],
            static_cast<int>(300 + (i / 3) * 100 + i % 3));
    }

    ragged_rows<int> column_one(values, 1);
    HPX_TEST_EQ(column_one.data().size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(column_one.offsets().size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(column_one.offsets()[0], static_cast<std::size_t>(0));
    HPX_TEST_EQ(column_one.offsets()[1], static_cast<std::size_t>(6));
    HPX_TEST_EQ(column_one.offsets()[2], static_cast<std::size_t>(6));
    for (std::size_t i = 0; i != column_one.data().size(); ++i)
    {
        HPX_TEST_EQ(
            column_one.data()[i], static_cast<int>((i / 2) * 100 + 3 + i % 2));
    }

    std::vector<ragged_rows<move_only_value>> move_only_values;
    std::vector<move_only_value> first;
    first.emplace_back(1);
    first.emplace_back(2);
    move_only_values.emplace_back(
        HPX_MOVE(first), std::vector<std::size_t>{0, 1, 2});
    std::vector<move_only_value> second;
    second.emplace_back(3);
    second.emplace_back(4);
    move_only_values.emplace_back(
        HPX_MOVE(second), std::vector<std::size_t>{0, 1, 2});

    ragged_rows<move_only_value> move_only_column(move_only_values, 1);
    HPX_TEST_EQ(move_only_column.data().size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(move_only_column.data()[0].value, 2);
    HPX_TEST_EQ(move_only_column.data()[1].value, 4);
    auto move_only_data = HPX_MOVE(move_only_column).release_data();
    HPX_TEST_EQ(move_only_data.size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(move_only_data[1].value, 4);
}

void test_ragged_rows_validation()
{
    ragged_rows<int> default_rows;
    ragged_rows<int> valid_empty_rows(
        std::vector<int>{}, std::vector<std::size_t>{0, 0, 0});
    HPX_TEST(default_rows.is_valid());
    HPX_TEST_EQ(default_rows.num_segments(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(default_rows.offsets().size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(default_rows.offsets()[0], static_cast<std::size_t>(0));
    HPX_TEST_EQ(default_rows.offsets()[1], static_cast<std::size_t>(0));
    HPX_TEST_NO_THROW(default_rows.validate("test_ragged_rows_validation"));
    HPX_TEST(valid_empty_rows.is_valid());
    HPX_TEST_NO_THROW(valid_empty_rows.validate("test_ragged_rows_validation"));
    HPX_TEST_NO_THROW(valid_empty_rows.validate_for_exchange(
        2, "test_ragged_rows_validation"));

    auto deserialize_rows = [](std::vector<int> data,
                                std::vector<std::size_t> offsets) {
        std::vector<char> buffer;
        hpx::serialization::output_archive output(buffer);
        output << data << offsets;

        ragged_rows<int> rows;
        hpx::serialization::input_archive input(buffer);
        input >> rows;
        return rows;
    };

    auto empty_offsets = deserialize_rows({}, {});
    auto single_offset = deserialize_rows({}, {0});
    auto nonzero_first = deserialize_rows({1}, {1, 1});
    auto decreasing = deserialize_rows({1, 2}, {0, 2, 1, 2});
    auto wrong_end = deserialize_rows({1, 2}, {0, 1});
    ragged_rows<int> incomplete_exchange(
        std::vector<int>{1}, std::vector<std::size_t>{0, 1});

    HPX_TEST(!empty_offsets.is_valid());
    HPX_TEST(!single_offset.is_valid());
    HPX_TEST(!nonzero_first.is_valid());
    HPX_TEST(!decreasing.is_valid());
    HPX_TEST(!wrong_end.is_valid());
    HPX_TEST_THROW(
        empty_offsets.validate("test_ragged_rows_validation"), hpx::exception);
    HPX_TEST_THROW(
        single_offset.validate("test_ragged_rows_validation"), hpx::exception);
    HPX_TEST_THROW(
        nonzero_first.validate("test_ragged_rows_validation"), hpx::exception);
    HPX_TEST_THROW(
        decreasing.validate("test_ragged_rows_validation"), hpx::exception);
    HPX_TEST_THROW(
        wrong_end.validate("test_ragged_rows_validation"), hpx::exception);
    HPX_TEST_THROW(incomplete_exchange.validate_for_exchange(
                       2, "test_ragged_rows_validation"),
        hpx::exception);
}

void test_uniform_rows_slicing()
{
    uniform_rows<int> first(std::vector<int>{10, 11, 20, 21, 30, 31}, 3);
    auto left = HPX_MOVE(first).extract(0, 2);
    HPX_TEST_EQ(left.data().size(), static_cast<std::size_t>(4));
    HPX_TEST_EQ(left.num_rows(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(left.data()[0], 10);
    HPX_TEST_EQ(left.data()[1], 11);
    HPX_TEST_EQ(left.data()[2], 20);
    HPX_TEST_EQ(left.data()[3], 21);

    uniform_rows<int> second(std::vector<int>{10, 11, 20, 21, 30, 31}, 3);
    auto right = HPX_MOVE(second).extract(1, 2);
    HPX_TEST_EQ(right.data().size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(right.num_rows(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(right.data()[0], 30);
    HPX_TEST_EQ(right.data()[1], 31);

    uniform_rows<int> empty_rows(std::vector<int>{}, 3);
    auto empty = HPX_MOVE(empty_rows).extract(1, 3);
    HPX_TEST(empty.data().empty());
    HPX_TEST_EQ(empty.num_rows(), static_cast<std::size_t>(1));

    std::vector<move_only_value> move_only_values;
    move_only_values.reserve(3);
    move_only_values.emplace_back(10);
    move_only_values.emplace_back(20);
    move_only_values.emplace_back(30);
    uniform_rows<move_only_value> move_only_rows(HPX_MOVE(move_only_values), 3);
    auto middle = HPX_MOVE(move_only_rows).extract(1, 3);
    HPX_TEST_EQ(middle.data().size(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(middle.data()[0].value, 20);
}

void test_uniform_rows_extract_single_slice()
{
    uniform_rows<int> rows(std::vector<int>{1, 2, 3, 4}, 2);
    auto slice = HPX_MOVE(rows).extract(0, 1);
    HPX_TEST_EQ(slice.num_rows(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(slice.data().size(), static_cast<std::size_t>(4));
    HPX_TEST_EQ(slice.data()[0], 1);
    HPX_TEST_EQ(slice.data()[1], 2);
    HPX_TEST_EQ(slice.data()[2], 3);
    HPX_TEST_EQ(slice.data()[3], 4);
}

void test_uniform_rows_merge()
{
    std::vector<uniform_rows<std::string>> values;
    values.emplace_back(std::vector<std::string>{"a", "b", "c", "d"}, 2);
    values.emplace_back(std::vector<std::string>{"e", "f"}, 1);

    uniform_rows<std::string> result(HPX_MOVE(values));
    HPX_TEST_EQ(result.data().size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(result.num_rows(), static_cast<std::size_t>(3));
    for (std::size_t i = 0; i != result.data().size(); ++i)
    {
        HPX_TEST_EQ(
            result.data()[i], std::string(1, static_cast<char>('a' + i)));
    }

    std::vector<uniform_rows<int>> mismatched;
    mismatched.emplace_back(std::vector<int>{1, 2}, 1);
    mismatched.emplace_back(std::vector<int>{3, 4, 5}, 1);
    HPX_TEST_THROW(
        (void) uniform_rows<int>(HPX_MOVE(mismatched)), hpx::exception);

    std::vector<uniform_rows<int>> singleton;
    singleton.emplace_back(std::vector<int>{1, 2, 3, 4}, 2);
    int const* const original_data = singleton.front().data().data();
    uniform_rows<int> singleton_result(HPX_MOVE(singleton));
    HPX_TEST_EQ(singleton_result.data().data(), original_data);
    HPX_TEST_EQ(singleton_result.num_rows(), static_cast<std::size_t>(2));
}

void test_uniform_rows_merge_move_only()
{
    std::vector<uniform_rows<move_only_value>> values;

    std::vector<move_only_value> first_row;
    first_row.emplace_back(1);
    first_row.emplace_back(2);
    values.emplace_back(HPX_MOVE(first_row), 1);

    std::vector<move_only_value> second_row;
    second_row.emplace_back(3);
    second_row.emplace_back(4);
    values.emplace_back(HPX_MOVE(second_row), 1);

    uniform_rows<move_only_value> merged(HPX_MOVE(values));
    HPX_TEST_EQ(merged.data().size(), static_cast<std::size_t>(4));
    HPX_TEST_EQ(merged.num_rows(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(merged.data()[0].value, 1);
    HPX_TEST_EQ(merged.data()[1].value, 2);
    HPX_TEST_EQ(merged.data()[2].value, 3);
    HPX_TEST_EQ(merged.data()[3].value, 4);
}

void test_uniform_rows_empty_merge()
{
    std::vector<uniform_rows<int>> empty_values;
    uniform_rows<int> result(HPX_MOVE(empty_values));
    HPX_TEST_EQ(result.num_rows(), static_cast<std::size_t>(0));
    HPX_TEST(result.data().empty());
}

void test_uniform_rows_vector_bool()
{
    std::vector<uniform_rows<bool>> values;
    values.emplace_back(std::vector<bool>{true, false, true, true}, 2);
    values.emplace_back(std::vector<bool>{false, false}, 1);

    uniform_rows<bool> merged(HPX_MOVE(values));
    HPX_TEST_EQ(merged.data().size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(merged.num_rows(), static_cast<std::size_t>(3));
    HPX_TEST(merged.data()[0]);
    HPX_TEST(!merged.data()[1]);
    HPX_TEST(merged.data()[2]);
    HPX_TEST(merged.data()[3]);
    HPX_TEST(!merged.data()[4]);
    HPX_TEST(!merged.data()[5]);

    auto slice = HPX_MOVE(merged).extract(1, 3);
    HPX_TEST_EQ(slice.data().size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(slice.num_rows(), static_cast<std::size_t>(1));
    HPX_TEST(slice.data()[0]);
    HPX_TEST(slice.data()[1]);

    uniform_rows<bool> value(true);
    HPX_TEST(HPX_MOVE(value).unwrap_value());
}

void test_uniform_rows_validation()
{
    uniform_rows<int> valid_empty_rows(std::vector<int>{}, 3);
    uniform_rows<int> valid_no_rows(std::vector<int>{}, 0);
    uniform_rows<int> valid_rows(std::vector<int>{1, 2, 3, 4}, 2);

    auto deserialize_rows = [](std::vector<int> data, std::size_t num_rows) {
        std::vector<char> buffer;
        hpx::serialization::output_archive output(buffer);
        output << data << num_rows;

        uniform_rows<int> rows;
        hpx::serialization::input_archive input(buffer);
        input >> rows;
        return rows;
    };

    auto rows_with_no_width = deserialize_rows(std::vector<int>{1}, 0);
    auto nonuniform_width = deserialize_rows(std::vector<int>{1, 2, 3}, 2);

    HPX_TEST(valid_empty_rows.is_valid());
    HPX_TEST(valid_no_rows.is_valid());
    HPX_TEST(valid_rows.is_valid());
    HPX_TEST_EQ(valid_empty_rows.row_width(), static_cast<std::size_t>(0));
    HPX_TEST_EQ(valid_rows.row_width(), static_cast<std::size_t>(2));
    HPX_TEST(!rows_with_no_width.is_valid());
    HPX_TEST(!nonuniform_width.is_valid());
    HPX_TEST_NO_THROW(
        valid_empty_rows.validate("test_uniform_rows_validation"));
    HPX_TEST_THROW(rows_with_no_width.validate("test_uniform_rows_validation"),
        hpx::exception);
    HPX_TEST_THROW(nonuniform_width.validate("test_uniform_rows_validation"),
        hpx::exception);
}

void test_uniform_rows_row_width_zero_rows()
{
    uniform_rows<int> zero_rows(std::vector<int>{}, 0);
    HPX_TEST_EQ(zero_rows.row_width(), static_cast<std::size_t>(0));
}

void test_uniform_rows_construction()
{
    using extract_type = decltype(&uniform_rows<int>::extract);
    static_assert(std::is_invocable_v<extract_type, uniform_rows<int>&&,
        std::size_t, std::size_t>);
    static_assert(!std::is_invocable_v<extract_type, uniform_rows<int>&,
        std::size_t, std::size_t>);
    static_assert(std::is_nothrow_constructible_v<uniform_rows<int>,
        std::vector<int>&&, std::size_t>);
    static_assert(
        std::is_nothrow_constructible_v<uniform_rows<int>, std::vector<int>&&>);

    uniform_rows<int> rows(std::vector<int>{1, 2, 3});
    HPX_TEST_EQ(rows.data().size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(rows.num_rows(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(rows.row_width(), static_cast<std::size_t>(1));

    std::string value = "value";
    uniform_rows<std::string> copied_value(value);
    HPX_TEST_EQ(HPX_MOVE(copied_value).unwrap_value(), value);

    uniform_rows<std::string> moved_value(std::string("moved"));
    HPX_TEST_EQ(HPX_MOVE(moved_value).unwrap_value(), std::string("moved"));

    uniform_rows<std::string> row(
        std::vector<std::string>{"first", "second"}, 1);
    auto unwrapped = HPX_MOVE(row).unwrap_row();
    HPX_TEST_EQ(unwrapped.size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(unwrapped[0], std::string("first"));
    HPX_TEST_EQ(unwrapped[1], std::string("second"));

    uniform_rows<int> scalar_rows(std::vector<int>{1, 2, 3});
    auto values = HPX_MOVE(scalar_rows).unwrap_values();
    HPX_TEST_EQ(values.size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(values[0], 1);
    HPX_TEST_EQ(values[1], 2);
    HPX_TEST_EQ(values[2], 3);

    uniform_rows<int> releasable(std::vector<int>{4, 5, 6, 7}, 2);
    auto released = HPX_MOVE(releasable).release_data();
    HPX_TEST_EQ(released.size(), static_cast<std::size_t>(4));
}

void test_data_size_overflow()
{
    constexpr std::size_t max_size = (std::numeric_limits<std::size_t>::max)();

    HPX_TEST_EQ(checked_data_size_sum(max_size - 1, 1), max_size);
    HPX_TEST_EQ(checked_data_size_product(max_size, 1), max_size);
    HPX_TEST_EQ(checked_data_size_product(max_size, 0), std::size_t{0});
    HPX_TEST_THROW((void) checked_data_size_sum(max_size, 1), hpx::exception);
    HPX_TEST_THROW(
        (void) checked_data_size_product(max_size, 2), hpx::exception);
}

void test_uniform_rows_serialization()
{
    uniform_rows<std::string> rows(
        std::vector<std::string>{"zero", "one", "two", "three"}, 2);
    std::vector<char> rows_buffer;
    hpx::serialization::output_archive rows_output(rows_buffer);
    rows_output << rows;

    uniform_rows<std::string> restored_rows;
    hpx::serialization::input_archive rows_input(rows_buffer);
    rows_input >> restored_rows;
    HPX_TEST_EQ(restored_rows.data().size(), rows.data().size());
    for (std::size_t i = 0; i != rows.data().size(); ++i)
    {
        HPX_TEST_EQ(restored_rows.data()[i], rows.data()[i]);
    }
    HPX_TEST_EQ(restored_rows.num_rows(), rows.num_rows());
}

void test_ragged_rows_serialization()
{
    ragged_rows<std::string> empty_rows;
    std::vector<char> empty_buffer;
    hpx::serialization::output_archive empty_output(empty_buffer);
    empty_output << empty_rows;

    ragged_rows<std::string> restored_empty_rows;
    hpx::serialization::input_archive empty_input(empty_buffer);
    empty_input >> restored_empty_rows;
    HPX_TEST(restored_empty_rows.is_valid());
    HPX_TEST_EQ(
        restored_empty_rows.num_segments(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(
        restored_empty_rows.offsets().size(), static_cast<std::size_t>(2));

    ragged_rows<std::string> rows(
        std::vector<std::string>{"zero", "one", "two"},
        std::vector<std::size_t>{0, 2, 3});
    std::vector<char> buffer;
    hpx::serialization::output_archive output(buffer);
    output << rows;

    ragged_rows<std::string> restored;
    hpx::serialization::input_archive input(buffer);
    input >> restored;
    HPX_TEST_EQ(restored.data().size(), rows.data().size());
    HPX_TEST_EQ(restored.offsets().size(), rows.offsets().size());
    for (std::size_t i = 0; i != rows.data().size(); ++i)
    {
        HPX_TEST_EQ(restored.data()[i], rows.data()[i]);
    }
    for (std::size_t i = 0; i != rows.offsets().size(); ++i)
    {
        HPX_TEST_EQ(restored.offsets()[i], rows.offsets()[i]);
    }
}

int hpx_main()
{
    test_balanced_arity2();
    test_balanced_arity4();
    test_unbalanced_n11_arity4();
    test_unbalanced_n7_arity3();
    test_arity_exceeds_n();
    test_single_site();
    test_coverage_property();
    test_classify_site_basic();
    test_classify_site_exhaustive();
    test_matches_recursive_fill();
    test_is_top_level_rep_basic();
    test_is_top_level_rep_exhaustive();
    test_is_ragged_rows_trait();
    test_is_uniform_rows_trait();
    test_ragged_rows_extract_columns();
    test_ragged_rows_validation();
    test_uniform_rows_slicing();
    test_uniform_rows_extract_single_slice();
    test_uniform_rows_merge();
    test_uniform_rows_merge_move_only();
    test_uniform_rows_empty_merge();
    test_uniform_rows_vector_bool();
    test_uniform_rows_validation();
    test_uniform_rows_row_width_zero_rows();
    test_uniform_rows_construction();
    test_data_size_overflow();
    test_ragged_rows_serialization();
    test_uniform_rows_serialization();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#endif
