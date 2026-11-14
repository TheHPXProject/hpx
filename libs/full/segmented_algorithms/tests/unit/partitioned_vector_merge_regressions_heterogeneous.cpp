//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Segmented merge regression with different input and output value types
// and a different projection for each input.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>

#include "partitioned_vector_merge_regressions.hpp"

HPX_REGISTER_PARTITIONED_VECTOR(regression_left_value);
HPX_REGISTER_PARTITIONED_VECTOR(regression_right_value);
HPX_REGISTER_PARTITIONED_VECTOR(regression_output_value);

using namespace merge_regression;

namespace {

    void test_heterogeneous_projected_policy_matrix()
    {
        auto const loc = hpx::find_all_localities();
        std::vector<regression_left_value> const input1{
            {101, 0}, {103, 1}, {103, 2}, {107, 3}};
        std::vector<regression_right_value> const input2{
            {202, 0}, {203, 1}, {203, 2}, {208, 3}, {209, 4}};
        regression_left_projection const projection1{-100};
        regression_right_projection const projection2{-200};
        auto project = [&](auto const& value) {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                              regression_left_value>)
            {
                return projection1(value);
            }
            else
            {
                return projection2(value);
            }
        };
        auto oracle_compare = [&](auto const& a, auto const& b) {
            return project(a) < project(b);
        };
        std::vector<regression_output_value> expected(9);
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), oracle_compare);
        std::vector<regression_output_value> const guards(
            9, regression_output_value{-999, -1, -1});
        auto source1 = make_vector(input1, {1, 3}, {loc[0], loc[1]});
        auto source2 = make_vector(input2, {2, 3}, {loc[2], loc[0]});
        auto destination =
            make_vector(guards, {2, 4, 3}, {loc[1], loc[2], loc[0]});

        for_each_policy([&](auto policy) {
            assign_values(destination, guards);
            auto result = finish_result(policy,
                hpx::ranges::merge(policy, source1.cbegin(), source1.cend(),
                    source2.cbegin(), source2.cend(), destination.begin(),
                    hpx::ranges::less{}, projection1, projection2));
            HPX_TEST(result.in1 == source1.cend());
            HPX_TEST(result.in2 == source2.cend());
            HPX_TEST(result.out == destination.end());
            // Compare raw output payloads, not just projected keys. Projection
            // selects ordering; it must not replace the copied input values.
            check_values(destination, expected);
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

    run_test("test_heterogeneous_projected_policy_matrix",
        test_heterogeneous_projected_policy_matrix);

    return hpx::util::report_errors();
}

#endif
