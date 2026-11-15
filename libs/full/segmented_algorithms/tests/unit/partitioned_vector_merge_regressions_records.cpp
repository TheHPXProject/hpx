//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Segmented merge regressions on record types: stability, comparator
// equivalence, concurrent task merges and heap-owned values.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>

#include <cstddef>
#include <string>
#include <vector>

#include "partitioned_vector_merge_regressions.hpp"

HPX_REGISTER_PARTITIONED_VECTOR(merge_regression_value);
HPX_REGISTER_PARTITIONED_VECTOR(regression_heap_value);

using namespace merge_regression;

namespace {

    template <typename Compare>
    void run_tagged_ordering_case(std::vector<merge_regression_value> input1,
        std::vector<merge_regression_value> input2, Compare compare,
        bool one_element_partitions)
    {
        using value_type = merge_regression_value;
        auto const loc = hpx::find_all_localities();
        // These stable sorts establish the merge precondition without hiding
        // the original order of equivalent records within either input.
        std::stable_sort(input1.begin(), input1.end(), compare);
        std::stable_sort(input2.begin(), input2.end(), compare);
        std::vector<value_type> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), compare);
        std::vector<value_type> const guards(
            expected.size(), value_type{-999, -999, -1, -1});

        auto source1 =
            make_vector(input1, {2, input1.size() - 2}, {loc[0], loc[1]});
        auto source2 =
            make_vector(input2, {3, input2.size() - 3}, {loc[1], loc[2]});
        std::vector<std::size_t> sizes = one_element_partitions ?
            std::vector<std::size_t>(expected.size(), 1) :
            std::vector<std::size_t>{1, 3, 7, 5, 8};
        std::vector<hpx::id_type> owners;
        for (std::size_t i = 0; i != sizes.size(); ++i)
        {
            owners.push_back(loc[(i + 2) % 3]);
        }
        auto destination = make_vector(guards, sizes, owners);

        for_each_policy([&](auto policy) {
            assign_values(destination, guards);
            auto result = finish_result(policy,
                hpx::merge(policy, source1.cbegin(), source1.cend(),
                    source2.cbegin(), source2.cend(), destination.begin(),
                    compare));
            HPX_TEST(result == destination.end());
            check_values(destination, expected);
        });
        check_values(source1, input1);
        check_values(source2, input2);
    }

    void test_two_key_descending_stability()
    {
        std::vector<merge_regression_value> input1;
        std::vector<merge_regression_value> input2;
        for (int i = 0; i != 12; ++i)
        {
            input1.push_back({(i / 2) % 3, (i / 4) % 2, 1, i});
            input2.push_back({(i / 3) % 3, (i / 2) % 2, 2, i});
        }
        // Key 2 breaks key-1 ties. When BOTH keys tie, merge must be stable.
        run_tagged_ordering_case(HPX_MOVE(input1), HPX_MOVE(input2),
            regression_two_key_less{true}, false);
    }

    void test_comparator_equivalence_across_tiny_partitions()
    {
        // Distinct values are equivalent under key / 10. Comparing the raw
        // key instead, or arbitrarily ordering ties, gives the wrong output.
        std::vector<merge_regression_value> input1{
            {12, 0, 1, 0}, {11, 0, 1, 1}, {19, 0, 1, 2}, {25, 0, 1, 3}};
        std::vector<merge_regression_value> input2{
            {18, 0, 2, 0}, {10, 0, 2, 1}, {17, 0, 2, 2}, {21, 0, 2, 3}};
        run_tagged_ordering_case(HPX_MOVE(input1), HPX_MOVE(input2),
            regression_bucket_less{10}, true);
    }

    void test_two_task_merges_before_waiting()
    {
        using value_type = merge_regression_value;
        auto const loc = hpx::find_all_localities();
        std::vector<value_type> input1;
        std::vector<value_type> input2;
        for (int i = 0; i != 32; ++i)
        {
            input1.push_back({i / 4, i % 2, 1, i});
            input2.push_back({i / 4, i % 2, 2, i});
        }
        regression_two_key_less const compare{true};
        std::stable_sort(input1.begin(), input1.end(), compare);
        std::stable_sort(input2.begin(), input2.end(), compare);
        std::vector<value_type> expected(64);
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), compare);

        auto source1 =
            make_vector(input1, {7, 9, 16}, {loc[0], loc[1], loc[2]});
        auto source2 =
            make_vector(input2, {10, 14, 8}, {loc[2], loc[0], loc[1]});
        std::vector<value_type> const guards(
            64, value_type{-999, -999, -1, -1});
        auto destination1 =
            make_vector(guards, {9, 23, 32}, {loc[2], loc[0], loc[1]});
        auto destination2 =
            make_vector(guards, {17, 15, 32}, {loc[1], loc[2], loc[0]});

        // The comparator's stack object is destroyed when this helper returns.
        // Each operation must retain its own copy. Sources and destinations
        // remain alive until BOTH operations finish; the outputs are disjoint.
        auto launch = [&](auto policy, auto& destination) {
            regression_two_key_less local_compare{true};
            return hpx::merge(policy, source1.cbegin(), source1.cend(),
                source2.cbegin(), source2.cend(), destination.begin(),
                local_compare);
        };
        auto first =
            launch(hpx::execution::seq(hpx::execution::task), destination1);
        auto second =
            launch(hpx::execution::par(hpx::execution::task), destination2);

        hpx::wait_all(first, second);
        HPX_TEST(first.get() == destination1.end());
        HPX_TEST(second.get() == destination2.end());
        check_values(destination1, expected);
        check_values(destination2, expected);
        check_values(source1, input1);
        check_values(source2, input2);
    }

    void test_heap_owned_values()
    {
        auto const localities = hpx::find_all_localities();

        std::vector<regression_heap_value> input1;
        std::vector<regression_heap_value> input2;

        constexpr int key_count = 16;
        constexpr int values_per_key = 2;
        constexpr std::size_t characters_per_key = 8192;

        for (int key = 0; key != key_count; ++key)
        {
            std::string payload(
                characters_per_key, static_cast<char>('a' + key));

            for (int ordinal = 0; ordinal != values_per_key; ++ordinal)
            {
                input1.push_back({payload, 1, ordinal});
                input2.push_back({payload, 2, ordinal});
            }
        }

        std::vector<regression_heap_value> expected(
            input1.size() + input2.size());

        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        auto source1 = make_vector(
            input1, {7, 9, 16}, {localities[0], localities[1], localities[2]});

        auto source2 = make_vector(
            input2, {8, 11, 13}, {localities[2], localities[0], localities[1]});

        std::vector<regression_heap_value> const initial(
            expected.size(), regression_heap_value{"guard", -1, -1});

        auto destination = make_vector(initial, {9, 13, 17, 25},
            {localities[2], localities[0], localities[1], localities[2]});

        for_each_policy([&](auto policy) {
            assign_values(destination, initial);

            auto result = finish_result(policy,
                hpx::merge(policy, source1.cbegin(), source1.cend(),
                    source2.cbegin(), source2.cend(), destination.begin()));

            HPX_TEST(result == destination.end());
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

    run_test(
        "test_two_key_descending_stability", test_two_key_descending_stability);
    run_test("test_comparator_equivalence_across_tiny_partitions",
        test_comparator_equivalence_across_tiny_partitions);
    run_test("test_two_task_merges_before_waiting",
        test_two_task_merges_before_waiting);
    run_test("test_heap_owned_values", test_heap_owned_values);

    return hpx::util::report_errors();
}

#endif
