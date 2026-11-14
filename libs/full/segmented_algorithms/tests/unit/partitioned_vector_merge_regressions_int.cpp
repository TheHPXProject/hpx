//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Segmented merge regressions on partitioned_vector<int>: subranges,
// empty ranges, output partition boundaries and diagonal search state.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>

#include "partitioned_vector_merge_regressions.hpp"

using namespace merge_regression;

namespace {

    void test_const_subranges_and_boundary_guards()
    {
        auto const loc = hpx::find_all_localities();
        int const smallest = (std::numeric_limits<int>::min)();
        int const largest = (std::numeric_limits<int>::max)();
        std::vector<int> const input1{smallest, smallest + 1, -10, -5, -3, 0, 2,
            4, 6, 8, largest - 1, largest};
        std::vector<int> const input2{
            smallest, -9, -4, -1, 1, 3, 5, 7, 9, largest};
        std::vector<int> const guards(28, 123456);
        auto source1 = make_vector(input1, {3, 5, 4}, {loc[0], loc[1], loc[2]});
        auto source2 = make_vector(input2, {4, 1, 5}, {loc[1], loc[2], loc[0]});
        auto destination = make_vector(
            guards, {5, 7, 4, 12}, {loc[2], loc[0], loc[1], loc[2]});

        struct bounds
        {
            std::ptrdiff_t first1, last1, first2, last2, output;
        };
        bounds const cases[] = {
            {1, 8, 2, 7, 4},    // Ends exactly at output partition boundary 16.
            {3, 11, 4, 10,
                5},    // Starts at boundaries, ends inside a partition.
            {0, 12, 0, 10,
                2},             // Full const inputs, including INT_MIN/INT_MAX.
            {8, 9, 5, 6, 26}    // Single elements; result == destination.end().
        };

        for_each_policy([&](auto policy) {
            for (auto const& b : cases)
            {
                assign_values(destination, guards);
                auto expected = guards;
                std::merge(input1.begin() + b.first1, input1.begin() + b.last1,
                    input2.begin() + b.first2, input2.begin() + b.last2,
                    expected.begin() + b.output);

                auto first1 = source1.cbegin() + b.first1;
                auto last1 = source1.cbegin() + b.last1;
                auto first2 = source2.cbegin() + b.first2;
                auto last2 = source2.cbegin() + b.last2;
                auto output = destination.begin() + b.output;
                auto const length = b.last1 - b.first1 + b.last2 - b.first2;
                auto result = finish_result(policy,
                    hpx::ranges::merge(policy, first1, last1, first2, last2,
                        output, regression_int_less{}));

                HPX_TEST(result.in1 == last1);
                HPX_TEST(result.in2 == last2);
                HPX_TEST(result.out == output + length);
                check_values(destination, expected);
            }
        });
        check_values(source1, input1);
        check_values(source2, input2);
    }

    void test_empty_end_ranges_do_not_compare_or_project()
    {
        auto const loc = hpx::find_all_localities();
        std::vector<int> const input1{1, 3, 5, 7, 9, 11};
        std::vector<int> const input2{2, 4, 6, 8, 10, 12};
        std::vector<int> const guards(9, -999);
        auto source1 = make_vector(input1, {2, 4}, {loc[0], loc[1]});
        auto source2 = make_vector(input2, {3, 3}, {loc[1], loc[0]});
        auto destination =
            make_vector(guards, {2, 3, 4}, {loc[2], loc[0], loc[1]});

        regression_throw_point forbidden;
        forbidden.everywhere = true;
        regression_throwing_compare compare{forbidden};
        regression_throwing_projection projection{forbidden};

        for_each_policy([&](auto policy) {
            for (int empty_case = 0; empty_case != 3; ++empty_case)
            {
                assign_values(destination, guards);
                bool const empty1 = empty_case != 1;
                bool const empty2 = empty_case != 0;
                auto first1 = empty1 ? source1.cend() : source1.cbegin();
                auto first2 = empty2 ? source2.cend() : source2.cbegin();
                auto output = empty_case == 2 ? destination.end() :
                                                destination.begin() + 2;
                auto expected = guards;
                std::ptrdiff_t const count = empty_case == 2 ? 0 : 6;
                if (empty_case != 2)
                {
                    auto const& input = empty1 ? input2 : input1;
                    std::copy(input.begin(), input.end(), expected.begin() + 2);
                }

                // Copy-only paths must not evaluate either projection or the
                // comparator. With both inputs empty, output == end is valid.
                auto result = finish_result(policy,
                    hpx::ranges::merge(policy, first1, source1.cend(), first2,
                        source2.cend(), output, compare, projection,
                        projection));
                HPX_TEST(result.in1 == source1.cend());
                HPX_TEST(result.in2 == source2.cend());
                HPX_TEST(result.out == output + count);
                check_values(destination, expected);
            }
        });
        check_values(source1, input1);
        check_values(source2, input2);
    }

    void test_output_partition_boundary_result()
    {
        auto const localities = hpx::find_all_localities();

        std::vector<int> const input1{1, 3, 5, 7};
        std::vector<int> const input2{2, 4, 6, 8};

        constexpr std::size_t merged_size = 8;

        std::vector<int> expected(12, -1);

        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        auto source1 =
            make_vector(input1, {2, 2}, {localities[0], localities[1]});

        auto source2 =
            make_vector(input2, {2, 2}, {localities[1], localities[0]});

        // The merge fills exactly the first destination partition. The second
        // partition remains as a guard range.
        std::vector<int> const initial(12, -1);

        auto destination = make_vector(
            initial, {merged_size, 4}, {localities[2], localities[0]});

        for_each_policy([&](auto policy) {
            assign_values(destination, initial);

            using iterator = decltype(destination.begin());
            using difference_type =
                typename std::iterator_traits<iterator>::difference_type;

            auto const expected_end = std::next(
                destination.begin(), static_cast<difference_type>(merged_size));

            auto result = finish_result(policy,
                hpx::merge(policy, source1.cbegin(), source1.cend(),
                    source2.cbegin(), source2.cend(), destination.begin()));

            HPX_TEST(result == expected_end);
            check_values(destination, expected);
        });

        check_values(source1, input1);
        check_values(source2, input2);
    }

    void test_invalid_diagonal_state_rejected()
    {
        using namespace hpx::parallel::detail;

        // This state is initially valid. The deliberately invalid comparator
        // makes the update move a_high below a_low.
        diagonal_search_state state{2,    // k
            1,                            // a_low
            2,                            // a_high
            1,                            // a
            1,                            // b
            false};

        diagonal_probe_values<int, int> values;

        values.a_previous = std::make_shared<int>(1);
        values.a_current = std::make_shared<int>(1);
        values.b_previous = std::make_shared<int>(1);
        values.b_current = std::make_shared<int>(1);

        auto invalid_compare = [](int, int) { return true; };

        bool caught = false;

        try
        {
            update_diagonal_state(state, 3, 3, values, invalid_compare);
        }
        catch (hpx::exception const& error)
        {
            caught = true;
            HPX_TEST(error.get_error() == hpx::error::bad_parameter);
        }
        catch (...)
        {
            caught = true;
            HPX_TEST(false);
        }

        HPX_TEST(caught);
    }

    void test_invalid_projected_value_count_rejected()
    {
        using namespace hpx::parallel::detail;

        for (std::size_t count : {std::size_t{0}, std::size_t{2}})
        {
            projected_value_result<int> result;
            result.values.assign(count, 42);

            std::vector<projected_value_result<int>> results;
            results.push_back(HPX_MOVE(result));

            std::vector<diagonal_probe_values<int, int>> values(1);

            bool caught = false;

            try
            {
                store_input1_probe_results(HPX_MOVE(results), values);
            }
            catch (hpx::exception const& error)
            {
                caught = true;
                HPX_TEST(error.get_error() == hpx::error::invalid_status);
            }
            catch (...)
            {
                caught = true;
                HPX_TEST(false);
            }

            HPX_TEST(caught);
        }
    }
}    // namespace

int main()
{
    if (!has_three_localities())
    {
        return hpx::util::report_errors();
    }

    run_test("test_const_subranges_and_boundary_guards",
        test_const_subranges_and_boundary_guards);
    run_test("test_empty_end_ranges_do_not_compare_or_project",
        test_empty_end_ranges_do_not_compare_or_project);
    run_test("test_output_partition_boundary_result",
        test_output_partition_boundary_result);
    run_test("test_invalid_diagonal_state_rejected",
        test_invalid_diagonal_state_rejected);
    run_test("test_invalid_projected_value_count_rejected",
        test_invalid_projected_value_count_rejected);

    return hpx::util::report_errors();
}

#endif
