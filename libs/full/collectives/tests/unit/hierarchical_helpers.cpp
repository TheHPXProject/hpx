//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/collectives/detail/flattened_data.hpp>
#include <hpx/modules/collectives.hpp>

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

using hpx::collectives::detail::classify_site;
using hpx::collectives::detail::flattened_data;
using hpx::collectives::detail::get_top_level_groups;
using hpx::collectives::detail::is_top_level_rep;
using hpx::collectives::detail::merge_flattened_data;
using hpx::collectives::detail::select_flattened_column;
using hpx::collectives::detail::slice_flattened_data;
using hpx::collectives::detail::top_level_group;

void test_balanced_arity2()
{
    // N=8, arity=2 -> two groups of 4: [0,3] and [4,7]
    auto groups = get_top_level_groups(8, 2);
    HPX_TEST_EQ(groups.size(), static_cast<std::size_t>(2));

    HPX_TEST_EQ(groups[0].left, static_cast<std::size_t>(0));
    HPX_TEST_EQ(groups[0].right, static_cast<std::size_t>(3));
    HPX_TEST_EQ(groups[0].size, static_cast<std::size_t>(4));

    HPX_TEST_EQ(groups[1].left, static_cast<std::size_t>(4));
    HPX_TEST_EQ(groups[1].right, static_cast<std::size_t>(7));
    HPX_TEST_EQ(groups[1].size, static_cast<std::size_t>(4));
}

void test_balanced_arity4()
{
    // N=8, arity=4 -> four groups of 2: [0,1], [2,3], [4,5], [6,7]
    auto groups = get_top_level_groups(8, 4);
    HPX_TEST_EQ(groups.size(), static_cast<std::size_t>(4));

    for (std::size_t i = 0; i != 4; ++i)
    {
        HPX_TEST_EQ(groups[i].left, i * 2);
        HPX_TEST_EQ(groups[i].right, i * 2 + 1);
        HPX_TEST_EQ(groups[i].size, static_cast<std::size_t>(2));
    }
}

void test_unbalanced_n11_arity4()
{
    // N=11, arity=4 -> 11/4=2 rem 3
    // groups: [0,2](3), [3,5](3), [6,8](3), [9,10](2)
    auto groups = get_top_level_groups(11, 4);
    HPX_TEST_EQ(groups.size(), static_cast<std::size_t>(4));

    HPX_TEST_EQ(groups[0].left, static_cast<std::size_t>(0));
    HPX_TEST_EQ(groups[0].right, static_cast<std::size_t>(2));
    HPX_TEST_EQ(groups[0].size, static_cast<std::size_t>(3));

    HPX_TEST_EQ(groups[1].left, static_cast<std::size_t>(3));
    HPX_TEST_EQ(groups[1].right, static_cast<std::size_t>(5));
    HPX_TEST_EQ(groups[1].size, static_cast<std::size_t>(3));

    HPX_TEST_EQ(groups[2].left, static_cast<std::size_t>(6));
    HPX_TEST_EQ(groups[2].right, static_cast<std::size_t>(8));
    HPX_TEST_EQ(groups[2].size, static_cast<std::size_t>(3));

    HPX_TEST_EQ(groups[3].left, static_cast<std::size_t>(9));
    HPX_TEST_EQ(groups[3].right, static_cast<std::size_t>(10));
    HPX_TEST_EQ(groups[3].size, static_cast<std::size_t>(2));
}

void test_unbalanced_n7_arity3()
{
    // N=7, arity=3 -> 7/3=2 rem 1
    // groups: [0,2](3), [3,4](2), [5,6](2)
    auto groups = get_top_level_groups(7, 3);
    HPX_TEST_EQ(groups.size(), static_cast<std::size_t>(3));

    HPX_TEST_EQ(groups[0].left, static_cast<std::size_t>(0));
    HPX_TEST_EQ(groups[0].right, static_cast<std::size_t>(2));
    HPX_TEST_EQ(groups[0].size, static_cast<std::size_t>(3));

    HPX_TEST_EQ(groups[1].left, static_cast<std::size_t>(3));
    HPX_TEST_EQ(groups[1].right, static_cast<std::size_t>(4));
    HPX_TEST_EQ(groups[1].size, static_cast<std::size_t>(2));

    HPX_TEST_EQ(groups[2].left, static_cast<std::size_t>(5));
    HPX_TEST_EQ(groups[2].right, static_cast<std::size_t>(6));
    HPX_TEST_EQ(groups[2].size, static_cast<std::size_t>(2));
}

void test_arity_exceeds_n()
{
    // N=3, arity=8 -> clamp arity to 3, three groups of 1
    auto groups = get_top_level_groups(3, 8);
    HPX_TEST_EQ(groups.size(), static_cast<std::size_t>(3));

    for (std::size_t i = 0; i != 3; ++i)
    {
        HPX_TEST_EQ(groups[i].left, i);
        HPX_TEST_EQ(groups[i].right, i);
        HPX_TEST_EQ(groups[i].size, static_cast<std::size_t>(1));
    }
}

void test_single_site()
{
    // N=1, arity=2 -> one group of 1
    auto groups = get_top_level_groups(1, 2);
    HPX_TEST_EQ(groups.size(), static_cast<std::size_t>(1));

    HPX_TEST_EQ(groups[0].left, static_cast<std::size_t>(0));
    HPX_TEST_EQ(groups[0].right, static_cast<std::size_t>(0));
    HPX_TEST_EQ(groups[0].size, static_cast<std::size_t>(1));
}

void test_coverage_property()
{
    // For various N and arity, verify that the groups cover [0, N) exactly.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            auto groups = get_top_level_groups(n, arity);

            std::size_t total_size = 0;
            for (auto const& g : groups)
            {
                HPX_TEST_EQ(g.size, g.right - g.left + 1);
                total_size += g.size;
            }
            HPX_TEST_EQ(total_size, n);

            HPX_TEST_EQ(groups.front().left, static_cast<std::size_t>(0));
            HPX_TEST_EQ(groups.back().right, n - 1);

            for (std::size_t i = 1; i < groups.size(); ++i)
            {
                HPX_TEST_EQ(groups[i].left, groups[i - 1].right + 1);
            }
        }
    }
}

void test_classify_site_basic()
{
    auto groups = get_top_level_groups(11, 4);

    HPX_TEST_EQ(classify_site(0, groups), static_cast<std::ptrdiff_t>(0));
    HPX_TEST_EQ(classify_site(1, groups), static_cast<std::ptrdiff_t>(0));
    HPX_TEST_EQ(classify_site(2, groups), static_cast<std::ptrdiff_t>(0));
    HPX_TEST_EQ(classify_site(3, groups), static_cast<std::ptrdiff_t>(1));
    HPX_TEST_EQ(classify_site(5, groups), static_cast<std::ptrdiff_t>(1));
    HPX_TEST_EQ(classify_site(6, groups), static_cast<std::ptrdiff_t>(2));
    HPX_TEST_EQ(classify_site(9, groups), static_cast<std::ptrdiff_t>(3));
    HPX_TEST_EQ(classify_site(10, groups), static_cast<std::ptrdiff_t>(3));
}

void test_classify_site_exhaustive()
{
    // Every site in [0, N) must classify to exactly one group.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            auto groups = get_top_level_groups(n, arity);

            for (std::size_t site = 0; site != n; ++site)
            {
                std::ptrdiff_t g = classify_site(site, groups);
                HPX_TEST_NEQ(g, static_cast<std::ptrdiff_t>(-1));
                HPX_TEST(site >= groups[g].left && site <= groups[g].right);
            }
        }
    }
}

void test_matches_recursive_fill()
{
    // Verify that get_top_level_groups produces the same partition as
    // the top frame of recursively_fill_communicators. That function
    // uses: division_steps = (right - left + 1) / arity,
    //       remainder = (right - left + 1) % arity.
    // We replicate that logic and compare.
    for (std::size_t n = 1; n <= 32; ++n)
    {
        for (std::size_t arity = 2; arity <= 8; ++arity)
        {
            auto groups = get_top_level_groups(n, arity);

            std::size_t effective_arity = (arity > n) ? n : arity;
            std::size_t division_steps = n / effective_arity;
            std::size_t remainder = n % effective_arity;

            HPX_TEST_EQ(groups.size(), effective_arity);

            std::size_t offset = 0;
            for (std::size_t i = 0; i != effective_arity; ++i)
            {
                std::size_t expected_size =
                    division_steps + (i < remainder ? 1 : 0);
                HPX_TEST_EQ(groups[i].left, offset);
                HPX_TEST_EQ(groups[i].size, expected_size);
                HPX_TEST_EQ(groups[i].right, offset + expected_size - 1);
                offset += expected_size;
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
            auto groups = get_top_level_groups(n, arity);

            for (std::size_t site = 0; site != n; ++site)
            {
                bool expected = false;
                for (auto const& g : groups)
                {
                    if (site == g.left)
                    {
                        expected = true;
                        break;
                    }
                }
                HPX_TEST_EQ(is_top_level_rep(site, n, arity), expected);
            }
        }
    }
}

void test_flattened_data_slicing()
{
    flattened_data<int> first{{10, 11, 20, 30, 31, 32}, {0, 2, 3, 6}};
    auto left = slice_flattened_data(first, 0, 2);
    HPX_TEST_EQ(left.data.size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(left.offsets.size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(left.data[0], 10);
    HPX_TEST_EQ(left.data[1], 11);
    HPX_TEST_EQ(left.data[2], 20);
    HPX_TEST_EQ(left.offsets[0], std::uint32_t(0));
    HPX_TEST_EQ(left.offsets[1], std::uint32_t(2));
    HPX_TEST_EQ(left.offsets[2], std::uint32_t(3));

    flattened_data<int> second{{10, 11, 20, 30, 31, 32}, {0, 2, 3, 6}};
    auto right = slice_flattened_data(second, 1, 2);
    HPX_TEST_EQ(right.data.size(), static_cast<std::size_t>(3));
    HPX_TEST_EQ(right.offsets.size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(right.data[0], 30);
    HPX_TEST_EQ(right.data[1], 31);
    HPX_TEST_EQ(right.data[2], 32);
    HPX_TEST_EQ(right.offsets[0], std::uint32_t(0));
    HPX_TEST_EQ(right.offsets[1], std::uint32_t(3));

    flattened_data<int> empty_row{{1, 2, 3, 4, 5}, {0, 2, 2, 5}};
    auto empty = slice_flattened_data(empty_row, 1, 3);
    HPX_TEST(empty.data.empty());
    HPX_TEST_EQ(empty.offsets.size(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(empty.offsets[0], std::uint32_t(0));
    HPX_TEST_EQ(empty.offsets[1], std::uint32_t(0));
}

void test_flattened_data_merge()
{
    std::vector<flattened_data<std::string>> values;
    values.push_back({{"a", "b", "c", "d"}, {0, 3, 4}});
    values.push_back({{"e", "f"}, {0, 2}});

    auto result = merge_flattened_data(values);
    HPX_TEST_EQ(result.data.size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(result.offsets.size(), static_cast<std::size_t>(4));
    for (std::size_t i = 0; i != result.data.size(); ++i)
    {
        HPX_TEST_EQ(result.data[i], std::string(1, static_cast<char>('a' + i)));
    }
    HPX_TEST_EQ(result.offsets[0], std::uint32_t(0));
    HPX_TEST_EQ(result.offsets[1], std::uint32_t(3));
    HPX_TEST_EQ(result.offsets[2], std::uint32_t(4));
    HPX_TEST_EQ(result.offsets[3], std::uint32_t(6));
}

void test_flattened_exchange_diagonal()
{
    std::vector<flattened_data<int>> values;
    values.push_back({{3, 4, 103, 104, 203, 204}, {0, 0, 2, 2, 4, 4, 6}});
    values.push_back({{300, 301, 302, 400, 401, 402}, {0, 3, 3, 6, 6}});

    auto for_group_zero = select_flattened_column(values, 0);
    HPX_TEST_EQ(for_group_zero.data.size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(for_group_zero.offsets[0], std::uint32_t(0));
    HPX_TEST_EQ(for_group_zero.offsets[1], std::uint32_t(0));
    HPX_TEST_EQ(for_group_zero.offsets[2], std::uint32_t(6));
    for (std::size_t i = 0; i != for_group_zero.data.size(); ++i)
    {
        HPX_TEST_EQ(for_group_zero.data[i],
            static_cast<int>(300 + (i / 3) * 100 + i % 3));
    }

    std::vector<flattened_data<int>> second_values;
    second_values.push_back(
        {{3, 4, 103, 104, 203, 204}, {0, 0, 2, 2, 4, 4, 6}});
    second_values.push_back({{300, 301, 302, 400, 401, 402}, {0, 3, 3, 6, 6}});
    auto for_group_one = select_flattened_column(second_values, 1);
    HPX_TEST_EQ(for_group_one.data.size(), static_cast<std::size_t>(6));
    HPX_TEST_EQ(for_group_one.offsets[0], std::uint32_t(0));
    HPX_TEST_EQ(for_group_one.offsets[1], std::uint32_t(6));
    HPX_TEST_EQ(for_group_one.offsets[2], std::uint32_t(6));
    for (std::size_t i = 0; i != for_group_one.data.size(); ++i)
    {
        HPX_TEST_EQ(
            for_group_one.data[i], static_cast<int>((i / 2) * 100 + 3 + i % 2));
    }
}

void test_flattened_vector_bool()
{
    std::vector<flattened_data<bool>> values;
    values.push_back({{true, false, true}, {0, 2, 3}});
    values.push_back({{false, false}, {0, 2}});

    auto merged = merge_flattened_data(values);
    HPX_TEST_EQ(merged.data.size(), static_cast<std::size_t>(5));
    HPX_TEST(merged.data[0]);
    HPX_TEST(!merged.data[1]);
    HPX_TEST(merged.data[2]);
    HPX_TEST(!merged.data[3]);
    HPX_TEST(!merged.data[4]);

    auto slice = slice_flattened_data(merged, 1, 3);
    HPX_TEST_EQ(slice.data.size(), static_cast<std::size_t>(1));
    HPX_TEST(slice.data[0]);
}

void test_flattened_data_serialization()
{
    flattened_data<std::string> original{{"zero", "one", "two"}, {0, 2, 3}};

    std::vector<char> buffer;
    hpx::serialization::output_archive output(buffer);
    output << original;

    flattened_data<std::string> restored;
    hpx::serialization::input_archive input(buffer);
    input >> restored;

    HPX_TEST_EQ(restored.data.size(), original.data.size());
    HPX_TEST_EQ(restored.offsets.size(), original.offsets.size());
    for (std::size_t i = 0; i != original.data.size(); ++i)
    {
        HPX_TEST_EQ(restored.data[i], original.data[i]);
    }
    for (std::size_t i = 0; i != original.offsets.size(); ++i)
    {
        HPX_TEST_EQ(restored.offsets[i], original.offsets[i]);
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
    test_flattened_data_slicing();
    test_flattened_data_merge();
    test_flattened_exchange_diagonal();
    test_flattened_vector_bool();
    test_flattened_data_serialization();

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
