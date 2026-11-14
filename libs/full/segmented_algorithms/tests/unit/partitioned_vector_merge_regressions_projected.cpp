//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Segmented merge regressions with projections: distinct stateful
// projections and a non-default-constructible projected key.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>

#include "partitioned_vector_merge_regressions.hpp"

HPX_REGISTER_PARTITIONED_VECTOR(merge_regression_value);

using namespace merge_regression;

namespace {

    void test_distinct_stateful_projections()
    {
        using value_type = merge_regression_value;
        auto const loc = hpx::find_all_localities();
        std::vector<value_type> input1;
        std::vector<value_type> input2;
        int const keys[] = {9, 7, 7, 5, 3, 1};
        for (int i = 0; i != 6; ++i)
        {
            input1.push_back({keys[i] + 100, 0, 1, i});
            input2.push_back({keys[i] + 200, 0, 2, i});
        }
        regression_key_projection const projection1{-100};
        regression_key_projection const projection2{-200};
        regression_int_less const compare{true};
        auto oracle_compare = [&](value_type const& a, value_type const& b) {
            auto project = [&](value_type const& v) {
                return v.source == 1 ? projection1(v) : projection2(v);
            };
            return compare(project(a), project(b));
        };
        HPX_TEST(std::is_sorted(input1.begin(), input1.end(), oracle_compare));
        HPX_TEST(std::is_sorted(input2.begin(), input2.end(), oracle_compare));
        std::vector<value_type> expected(12);
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), oracle_compare);
        std::vector<value_type> const guards(
            12, value_type{-999, -999, -1, -1});

        // Last output partition is colocated with BOTH inputs; earlier output
        // partitions require capturing them. The two projection states differ.
        auto source1 = make_vector(input1, {6}, {loc[1]});
        auto source2 = make_vector(input2, {6}, {loc[1]});
        auto destination =
            make_vector(guards, {3, 4, 5}, {loc[2], loc[0], loc[1]});
        auto check_result = [&](auto const& result) {
            HPX_TEST(result.in1 == source1.cend());
            HPX_TEST(result.in2 == source2.cend());
            HPX_TEST(result.out == destination.end());
            check_values(destination, expected);
        };
        for_each_policy([&](auto policy) {
            assign_values(destination, guards);
            check_result(finish_result(policy,
                hpx::ranges::merge(policy, source1.cbegin(), source1.cend(),
                    source2.cbegin(), source2.cend(), destination.begin(),
                    compare, projection1, projection2)));
        });

        // Cover the no-policy RANGES iterator overload as well.
        assign_values(destination, guards);
        check_result(hpx::ranges::merge(source1.cbegin(), source1.cend(),
            source2.cbegin(), source2.cend(), destination.begin(), compare,
            projection1, projection2));
        check_values(source1, input1);
        check_values(source2, input2);
    }

    void test_non_default_constructible_projected_key()
    {
        auto const localities = hpx::find_all_localities();

        std::vector<int> const input1{1, 3, 5, 7, 9, 11};
        std::vector<int> const input2{2, 4, 6, 8, 10, 12};

        std::vector<int> expected(input1.size() + input2.size());

        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        std::vector<int> const initial(expected.size(), -1);

        // Inputs and destination reside on different localities so that projected
        // keys are serialized through the remote diagonal-probe actions.
        auto source1 =
            make_vector(input1, {2, 4}, {localities[0], localities[1]});

        auto source2 =
            make_vector(input2, {3, 3}, {localities[1], localities[0]});

        auto destination = make_vector(
            initial, {3, 4, 5}, {localities[2], localities[0], localities[2]});

        regression_non_default_less compare;
        regression_non_default_projection projection;

        for_each_policy([&](auto policy) {
            assign_values(destination, initial);

            auto result = finish_result(policy,
                hpx::ranges::merge(policy, source1.cbegin(), source1.cend(),
                    source2.cbegin(), source2.cend(), destination.begin(),
                    compare, projection, projection));

            HPX_TEST(result.in1 == source1.cend());
            HPX_TEST(result.in2 == source2.cend());
            HPX_TEST(result.out == destination.end());

            check_values(destination, expected);

            // Also instantiate the algorithm with two empty segmented ranges.
            // No key is produced at runtime, but the projected key type must still
            // be accepted during template instantiation.
            auto empty_result = finish_result(policy,
                hpx::ranges::merge(policy, source1.cend(), source1.cend(),
                    source2.cend(), source2.cend(), destination.end(), compare,
                    projection, projection));

            HPX_TEST(empty_result.in1 == source1.cend());
            HPX_TEST(empty_result.in2 == source2.cend());
            HPX_TEST(empty_result.out == destination.end());
        });

        check_values(source1, input1);
        check_values(source2, input2);
    }
}    // namespace

int main()
{
    if (!has_three_localities())
    {
        return hpx::util::report_errors();
    }

    run_test("test_distinct_stateful_projections",
        test_distinct_stateful_projections);
    run_test("test_non_default_constructible_projected_key",
        test_non_default_constructible_projected_key);

    return hpx::util::report_errors();
}

#endif
