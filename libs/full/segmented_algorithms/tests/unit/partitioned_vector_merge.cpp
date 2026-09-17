//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <numeric>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

struct throwing_compare
{
    int trigger = 0;

    bool operator()(int lhs, int rhs) const
    {
        if (lhs == trigger || rhs == trigger)
        {
            throw std::runtime_error("expected merge comparison failure");
        }

        return lhs < rhs;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & trigger;
    }
};

struct first_input_value
{
    int key = 0;
    int sequence = 0;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & sequence;
    }
};

struct second_input_value
{
    int priority = 0;
    int sequence = 0;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & priority & sequence;
    }
};

struct mixed_output_value
{
    int key = 0;
    int source = 0;
    int sequence = 0;

    mixed_output_value& operator=(first_input_value const& value)
    {
        key = value.key;
        source = 1;
        sequence = value.sequence;
        return *this;
    }

    mixed_output_value& operator=(second_input_value const& value)
    {
        key = value.priority;
        source = 2;
        sequence = value.sequence;
        return *this;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & source & sequence;
    }
};

struct first_key_projection
{
    int operator()(first_input_value const& value) const
    {
        return value.key;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

struct second_key_projection
{
    int operator()(second_input_value const& value) const
    {
        return value.priority;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

HPX_REGISTER_PARTITIONED_VECTOR(first_input_value);
HPX_REGISTER_PARTITIONED_VECTOR(second_input_value);
HPX_REGISTER_PARTITIONED_VECTOR(mixed_output_value);

struct stable_value
{
    int key = 0;
    int source = 0;
    int sequence = 0;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & source & sequence;
    }
};

bool operator<(stable_value const& lhs, stable_value const& rhs)
{
    return lhs.key < rhs.key;
}

HPX_REGISTER_PARTITIONED_VECTOR(stable_value);

struct stateful_compare
{
    bool descending = false;

    bool operator()(int lhs, int rhs) const
    {
        return descending ? lhs > rhs : lhs < rhs;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & descending;
    }
};

struct stateful_key_projection
{
    int multiplier = 1;

    int operator()(stable_value const& value) const
    {
        return multiplier * value.key;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & multiplier;
    }
};

namespace {

    // Transfer whole partitions, rather than issuing one remote action per
    // element. Wait for every operation before releasing its input buffers.
    template <typename T>
    void assign_values(
        hpx::partitioned_vector<T>& destination, std::vector<T> const& source)
    {
        HPX_TEST_EQ(destination.size(), source.size());
        if (destination.size() != source.size())
        {
            return;
        }

        auto const sizes = destination.get_partition_sizes();
        std::vector<hpx::future<void>> operations;
        operations.reserve(sizes.size());
        std::vector<std::size_t> const all_positions;
        auto first = source.begin();
        for (std::size_t part = 0; part != sizes.size(); ++part)
        {
            auto last = std::next(first,
                static_cast<typename std::vector<T>::difference_type>(
                    sizes[part]));
            operations.push_back(destination.set_values(
                part, all_positions, std::vector<T>(first, last)));
            first = last;
        }
        HPX_TEST(first == source.end());
        hpx::wait_all(operations);
        for (auto& operation : operations)
        {
            operation.get();
        }
    }

    template <typename T>
    std::vector<T> collect_values(hpx::partitioned_vector<T> const& source)
    {
        auto const sizes = source.get_partition_sizes();
        std::vector<hpx::future<std::vector<T>>> operations;
        operations.reserve(sizes.size());
        for (std::size_t part = 0; part != sizes.size(); ++part)
        {
            operations.push_back(source.get_values(part));
        }
        hpx::wait_all(operations);

        std::vector<T> values;
        values.reserve(source.size());
        // Preserve global partition order, not completion order.
        for (auto& operation : operations)
        {
            auto partition = operation.get();
            values.insert(values.end(),
                std::make_move_iterator(partition.begin()),
                std::make_move_iterator(partition.end()));
        }
        return values;
    }

    template <typename T>
    void check_values(hpx::partitioned_vector<T> const& actual,
        std::vector<T> const& expected)
    {
        auto const values = collect_values(actual);
        HPX_TEST_EQ(values.size(), expected.size());
        if (values.size() != expected.size())
        {
            return;
        }
        for (std::size_t i = 0; i != expected.size(); ++i)
        {
            HPX_TEST_EQ(values[i], expected[i]);
        }
    }

    void check_stable_values(
        hpx::partitioned_vector<stable_value> const& actual,
        std::vector<stable_value> const& expected)
    {
        auto const values = collect_values(actual);
        HPX_TEST_EQ(values.size(), expected.size());
        if (values.size() != expected.size())
        {
            return;
        }
        for (std::size_t i = 0; i != expected.size(); ++i)
        {
            HPX_TEST_EQ(values[i].key, expected[i].key);
            HPX_TEST_EQ(values[i].source, expected[i].source);
            HPX_TEST_EQ(values[i].sequence, expected[i].sequence);
        }
    }

    void check_values(hpx::partitioned_vector<stable_value> const& actual,
        std::vector<stable_value> const& expected)
    {
        check_stable_values(actual, expected);
    }

    // One owner entry per partition. Repeated owners explicitly create
    // non-adjacent partitions on the same locality; do not assume that a
    // partition count larger than the locality count implies round-robin.
    hpx::container_distribution_policy partition_layout(
        std::size_t partitions, std::vector<hpx::id_type> const& localities)
    {
        std::vector<hpx::id_type> owners;
        owners.reserve(partitions);
        for (std::size_t i = 0; i != partitions; ++i)
        {
            owners.push_back(localities[i % localities.size()]);
        }
        return hpx::container_layout(partitions, HPX_MOVE(owners));
    }

    template <typename Policy, typename T, typename Layout1, typename Layout2,
        typename LayoutOut>
    void run_merge_case(Policy policy, std::vector<T> const& input1,
        std::vector<T> const& input2, Layout1 const& layout1,
        Layout2 const& layout2, LayoutOut const& layout_out)
    {
        HPX_TEST(std::is_sorted(input1.begin(), input1.end()));
        HPX_TEST(std::is_sorted(input2.begin(), input2.end()));

        std::vector<T> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        hpx::partitioned_vector<T> source1(input1.size(), layout1);
        hpx::partitioned_vector<T> source2(input2.size(), layout2);
        hpx::partitioned_vector<T> destination(expected.size(), layout_out);
        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(policy, source1.begin(), source1.end(),
            source2.begin(), source2.end(), destination.begin());
        if constexpr (hpx::is_async_execution_policy_v<Policy>)
        {
            HPX_TEST(result.get() == destination.end());
        }
        else
        {
            HPX_TEST(result == destination.end());
        }
        check_values(destination, expected);
    }

    void run_test(char const* name, void (*test)())
    {
        auto const start = std::chrono::steady_clock::now();
        std::fprintf(stderr, "[merge-test] start %s\n", name);
        std::fflush(stderr);
        test();
        double const elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start)
                                   .count();
        std::fprintf(stderr, "[merge-test] done %s (%.3fs)\n", name, elapsed);
        std::fflush(stderr);
    }

    void check_mixed_values(
        hpx::partitioned_vector<mixed_output_value> const& actual,
        std::vector<mixed_output_value> const& expected)
    {
        auto const values = collect_values(actual);
        HPX_TEST_EQ(values.size(), expected.size());
        if (values.size() != expected.size())
        {
            return;
        }
        for (std::size_t i = 0; i != expected.size(); ++i)
        {
            HPX_TEST_EQ(values[i].key, expected[i].key);
            HPX_TEST_EQ(values[i].source, expected[i].source);
            HPX_TEST_EQ(values[i].sequence, expected[i].sequence);
        }
    }

    void run_cross_locality_capture_case(hpx::id_type const& source1_locality,
        hpx::id_type const& source2_locality,
        hpx::id_type const& destination_locality)
    {
        std::vector<int> const input1{1, 3, 5, 7};
        std::vector<int> const input2{2, 4, 6, 8};
        std::vector<int> const expected{1, 2, 3, 4, 5, 6, 7, 8};

        auto const source1_layout = hpx::container_layout(
            1, std::vector<hpx::id_type>{source1_locality});

        auto const source2_layout = hpx::container_layout(
            1, std::vector<hpx::id_type>{source2_locality});

        auto const destination_layout = hpx::container_layout(
            1, std::vector<hpx::id_type>{destination_locality});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        check_values(source1, input1);
        check_values(source2, input2);

        auto result = hpx::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void run_one_empty_input_case(bool first_is_empty,
        hpx::id_type const& source_locality,
        hpx::id_type const& destination_locality)
    {
        std::vector<int> const values{1, 3, 5, 7, 9};
        std::vector<int> const ignored{100};

        auto const source_layout = hpx::container_layout(
            1, std::vector<hpx::id_type>{source_locality});

        auto const destination_layout = hpx::container_layout(
            1, std::vector<hpx::id_type>{destination_locality});

        hpx::partitioned_vector<int> source1(
            first_is_empty ? ignored.size() : values.size(), source_layout);

        hpx::partitioned_vector<int> source2(
            first_is_empty ? values.size() : ignored.size(), source_layout);

        hpx::partitioned_vector<int> destination(
            values.size(), destination_layout);

        if (first_is_empty)
        {
            assign_values(source1, ignored);
            assign_values(source2, values);
        }
        else
        {
            assign_values(source1, values);
            assign_values(source2, ignored);
        }

        auto first1 = source1.begin();
        auto last1 = first_is_empty ? first1 : source1.end();

        auto first2 = source2.begin();
        auto last2 = first_is_empty ? source2.end() : first2;

        auto result = hpx::merge(hpx::execution::seq, first1, last1, first2,
            last2, destination.begin());

        HPX_TEST(result == destination.end());
        check_values(destination, values);
    }

    void test_capture_second_input()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 2);

        if (localities.size() < 2)
        {
            return;
        }
        // Input 1 and output are on locality 0.
        // Input 2 must be captured from locality 1.
        run_cross_locality_capture_case(
            localities[0], localities[1], localities[0]);
    }

    void test_capture_first_input()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 2);

        if (localities.size() < 2)
        {
            return;
        }
        // Input 2 and output are on locality 1.
        // Input 1 must be captured from locality 0.
        run_cross_locality_capture_case(
            localities[0], localities[1], localities[1]);
    }

    void test_capture_both_inputs()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }
        // input1 is on locality 0
        // input2 is on locality 1
        // Destination is on locality 2.
        run_cross_locality_capture_case(
            localities[0], localities[1], localities[2]);
    }

    void test_multi_partition_capture()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 2);

        if (localities.size() < 2)
        {
            return;
        }
        std::vector<int> const input1{
            1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23};
        std::vector<int> const input2{
            2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24};
        std::vector<int> const expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

        // Four partitions distributed across the available localities.
        auto const source1_layout = partition_layout(4, localities);
        auto const source2_layout = partition_layout(4, localities);

        // output is on one partition on locality 0.
        auto const destination_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[0]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);
        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);
        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void test_empty_input_ranges()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 2);

        if (localities.size() < 2)
        {
            return;
        }

        // Non-empty input is remote from the destination.
        run_one_empty_input_case(true, localities[1], localities[0]);

        run_one_empty_input_case(false, localities[1], localities[0]);
    }

    void test_stable_merge()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<stable_value> const input1{
            {1, 1, 0},
            {2, 1, 1},
            {2, 1, 2},
            {4, 1, 3},
        };

        std::vector<stable_value> const input2{
            {1, 2, 0},
            {2, 2, 1},
            {2, 2, 2},
            {3, 2, 3},
        };

        std::vector<stable_value> const expected{
            {1, 1, 0},    // input 1 comes first for equal key 1
            {1, 2, 0},

            {2, 1, 1},    // both input-1 key-2 values remain ordered
            {2, 1, 2},
            {2, 2, 1},    // followed by input-2 key-2 values
            {2, 2, 2},

            {3, 2, 3},
            {4, 1, 3},
        };

        auto const source1_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<stable_value> source1(
            input1.size(), source1_layout);

        hpx::partitioned_vector<stable_value> source2(
            input2.size(), source2_layout);

        hpx::partitioned_vector<stable_value> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        check_stable_values(destination, expected);
    }

    void test_multi_partition_stable_merge()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        // Equal-key groups deliberately cross partition boundaries.
        std::vector<stable_value> const input1{
            {1, 1, 0},
            {2, 1, 1},
            {2, 1, 2},

            {2, 1, 3},
            {2, 1, 4},
            {3, 1, 5},

            {4, 1, 6},
            {4, 1, 7},
            {4, 1, 8},

            {5, 1, 9},
            {6, 1, 10},
            {6, 1, 11},
        };

        std::vector<stable_value> const input2{
            {1, 2, 0},
            {1, 2, 1},
            {2, 2, 2},

            {2, 2, 3},
            {2, 2, 4},
            {2, 2, 5},

            {3, 2, 6},
            {4, 2, 7},
            {4, 2, 8},

            {4, 2, 9},
            {6, 2, 10},
            {7, 2, 11},
        };

        std::vector<stable_value> expected(input1.size() + input2.size());

        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        // Each input has four partitions alternating between
        // localities 0 and 1.
        auto const source1_layout = partition_layout(
            4, std::vector<hpx::id_type>{localities[0], localities[1]});

        auto const source2_layout = partition_layout(
            4, std::vector<hpx::id_type>{localities[1], localities[0]});

        // Both complete input ranges are remote from the destination.
        auto const destination_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<stable_value> source1(
            input1.size(), source1_layout);

        hpx::partitioned_vector<stable_value> source2(
            input2.size(), source2_layout);

        hpx::partitioned_vector<stable_value> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        check_stable_values(destination, expected);
    }

    void test_statefull_comparator()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        // Both inputs are sorted in descending order.
        std::vector<int> const input1{47, 45, 43, 41, 39, 37, 35, 33, 31, 29,
            27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1};

        std::vector<int> const input2{48, 46, 44, 42, 40, 38, 36, 34, 32, 30,
            28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2};

        std::vector<int> expected;
        expected.reserve(input1.size() + input2.size());

        for (int value = 48; value >= 1; --value)
        {
            expected.push_back(value);
        }

        auto const source1_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        stateful_compare compare;
        compare.descending = true;

        auto result =
            hpx::merge(hpx::execution::seq, source1.begin(), source1.end(),
                source2.begin(), source2.end(), destination.begin(), compare);

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void test_ranges_merge_with_projections()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        // Sorted in descending key order.
        std::vector<stable_value> const input1{
            {9, 1, 0},
            {7, 1, 1},
            {7, 1, 2},
            {5, 1, 3},
            {5, 1, 4},
            {3, 1, 5},
            {3, 1, 6},
            {1, 1, 7},
        };

        std::vector<stable_value> const input2{
            {10, 2, 0},
            {8, 2, 1},
            {7, 2, 2},
            {7, 2, 3},
            {6, 2, 4},
            {4, 2, 5},
            {3, 2, 6},
            {2, 2, 7},
        };

        stateful_key_projection projection;
        projection.multiplier = -1;

        auto projected_less = [projection](stable_value const& lhs,
                                  stable_value const& rhs) {
            return projection(lhs) < projection(rhs);
        };

        std::vector<stable_value> expected(input1.size() + input2.size());

        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), projected_less);

        auto const source1_layout =
            hpx::container_layout(3, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(3, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<stable_value> source1(
            input1.size(), source1_layout);

        hpx::partitioned_vector<stable_value> source2(
            input2.size(), source2_layout);

        hpx::partitioned_vector<stable_value> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::ranges::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin(),
            hpx::ranges::less{}, projection, projection);

        HPX_TEST(result.in1 == source1.end());
        HPX_TEST(result.in2 == source2.end());
        HPX_TEST(result.out == destination.end());

        check_stable_values(destination, expected);
    }

    void test_different_input_types()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<first_input_value> const input1{
            {1, 0},
            {3, 1},
            {3, 2},
            {6, 3},
        };

        std::vector<second_input_value> const input2{
            {2, 0},
            {3, 1},
            {3, 2},
            {5, 3},
        };

        std::vector<mixed_output_value> const expected{
            {1, 1, 0},
            {2, 2, 0},

            // Equal keys from input 1 must come first.
            {3, 1, 1},
            {3, 1, 2},

            // Input 2 retains its own ordering.
            {3, 2, 1},
            {3, 2, 2},

            {5, 2, 3},
            {6, 1, 3},
        };

        auto const source1_layout =
            hpx::container_layout(2, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(2, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(3, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<first_input_value> source1(
            input1.size(), source1_layout);

        hpx::partitioned_vector<second_input_value> source2(
            input2.size(), source2_layout);

        hpx::partitioned_vector<mixed_output_value> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::ranges::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin(),
            hpx::ranges::less{}, first_key_projection{},
            second_key_projection{});

        HPX_TEST(result.in1 == source1.end());
        HPX_TEST(result.in2 == source2.end());
        HPX_TEST(result.out == destination.end());

        check_mixed_values(destination, expected);
    }

    void test_partial_subranges()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<int> const complete_input1{
            -100, -99,                 // excluded prefix
            1, 3, 5, 7, 9, 100, 101    // excluded suffix
        };

        std::vector<int> const complete_input2{
            -200,    // excluded prefix
            2, 4, 6, 8, 10,
            200    // excluded suffix
        };

        std::vector<int> const initial_destination(14, -777);

        std::vector<int> const expected_destination{
            -777, -777, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, -777, -777};

        auto const source1_layout = partition_layout(
            4, std::vector<hpx::id_type>{localities[0], localities[1]});

        auto const source2_layout = partition_layout(
            4, std::vector<hpx::id_type>{localities[1], localities[0]});

        auto const destination_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(
            complete_input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(
            complete_input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            initial_destination.size(), destination_layout);

        assign_values(source1, complete_input1);
        assign_values(source2, complete_input2);
        assign_values(destination, initial_destination);

        auto first1 = std::next(source1.begin(), 2);
        auto last1 = std::next(first1, 5);

        auto first2 = std::next(source2.begin(), 1);
        auto last2 = std::next(first2, 5);

        auto output_first = std::next(destination.begin(), 2);
        auto expected_result = std::next(output_first, 10);

        auto result = hpx::merge(
            hpx::execution::seq, first1, last1, first2, last2, output_first);

        HPX_TEST(result == expected_result);
        check_values(destination, expected_destination);
    }

    void test_both_inputs_empty()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<int> const input1{1, 3, 5};
        std::vector<int> const input2{2, 4, 6};
        std::vector<int> const initial_destination{-777, -777, -777, -777};

        auto const source1_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            initial_destination.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);
        assign_values(destination, initial_destination);

        auto first1 = source1.begin();
        auto first2 = source2.begin();

        // Start inside the destination to verify that nothing is written.
        auto output_first = std::next(destination.begin(), 1);

        auto result = hpx::merge(
            hpx::execution::seq, first1, first1, first2, first2, output_first);

        // Merging two empty ranges must return the unchanged output iterator.
        HPX_TEST(result == output_first);

        // No destination element may be modified.
        check_values(destination, initial_destination);
    }

    void test_remote_comparator_exception()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<int> const input1{1, 3, 5, 7, 9, 11, 13, 15};

        std::vector<int> const input2{2, 4, 6, 8, 10, 12, 14, 16};

        auto const source1_layout =
            hpx::container_layout(2, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(2, std::vector<hpx::id_type>{localities[1]});

        // One destination partition makes the complete merge execute remotely
        // without interior diagonal-boundary comparisons on the caller.
        auto const destination_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            input1.size() + input2.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        throwing_compare compare;
        compare.trigger = 13;

        bool caught_exception = false;

        try
        {
            (void) hpx::merge(hpx::execution::seq, source1.begin(),
                source1.end(), source2.begin(), source2.end(),
                destination.begin(), compare);
        }
        catch (hpx::exception_list const& errors)
        {
            caught_exception = true;
            HPX_TEST(errors.size() != 0);
        }
        catch (...)
        {
            // Policy-based HPX algorithms should normalize ordinary user
            // exceptions into hpx::exception_list.
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
    }

    void test_merge_no_policy()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<stable_value> const input1{
            {0, 1, 0}, {1, 1, 1}, {1, 1, 2}, {4, 1, 3}, {10, 1, 4}};
        std::vector<stable_value> const input2{
            {1, 2, 0}, {1, 2, 1}, {2, 2, 2}, {4, 2, 3}, {9, 2, 4}};
        std::vector<stable_value> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        hpx::partitioned_vector<stable_value> source1(
            input1.size(), partition_layout(1, {locs[0]}));
        hpx::partitioned_vector<stable_value> source2(
            input2.size(), partition_layout(1, {locs[1]}));
        hpx::partitioned_vector<stable_value> destination(
            expected.size(), partition_layout(2, {locs[2]}));
        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result =
            hpx::merge(source1.begin(), source1.end(), source2.begin(),
                source2.end(), destination.begin(), hpx::ranges::less{});
        HPX_TEST(result == destination.end());
        check_stable_values(destination, expected);
    }

    void test_zero_sized_containers()
    {
        auto const locs = hpx::find_all_localities();
        auto const layout1 = partition_layout(1, {locs[0]});
        auto const layout2 = partition_layout(1, {locs[1]});
        auto const layout_out = partition_layout(1, {locs[2]});
        std::vector<int> const empty;
        std::vector<int> const values{1, 2, 3, 5, 8};

        // These are zero-sized containers, not empty subranges.
        run_merge_case(
            hpx::execution::seq, empty, values, layout1, layout2, layout_out);
        run_merge_case(
            hpx::execution::seq, values, empty, layout1, layout2, layout_out);
        run_merge_case(
            hpx::execution::seq, empty, empty, layout1, layout2, layout_out);
    }

    void test_single_element_inputs()
    {
        auto const locs = hpx::find_all_localities();
        auto const layout1 = partition_layout(1, {locs[0]});
        auto const layout2 = partition_layout(1, {locs[1]});
        auto const layout_out = partition_layout(1, {locs[2]});

        run_merge_case(hpx::execution::seq, std::vector<int>{3},
            std::vector<int>{1}, layout1, layout2, layout_out);
        run_merge_case(hpx::execution::seq, std::vector<int>{1},
            std::vector<int>{3}, layout1, layout2, layout_out);
        run_merge_case(hpx::execution::seq,
            std::vector<stable_value>{{5, 1, 0}},
            std::vector<stable_value>{{5, 2, 0}}, layout1, layout2, layout_out);
    }

    void test_all_equal_keys()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<stable_value> input1;
        std::vector<stable_value> input2;
        for (int i = 0; i != 6; ++i)
        {
            input1.push_back({7, 1, i});
        }
        for (int i = 0; i != 4; ++i)
        {
            input2.push_back({7, 2, i});
        }
        // Both source identity and within-source order must survive.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(2, {locs[0], locs[1]}),
            partition_layout(2, {locs[1], locs[0]}),
            partition_layout(2, {locs[2]}));
    }

    void test_nonoverlapping_sequential_ranges()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<int> const low{1, 2, 3, 4};
        std::vector<int> const high{10, 11, 12, 13};
        auto const layout1 = partition_layout(2, {locs[0]});
        auto const layout2 = partition_layout(2, {locs[1]});
        auto const layout_out = partition_layout(4, {locs[2]});

        run_merge_case(
            hpx::execution::seq, low, high, layout1, layout2, layout_out);
        run_merge_case(
            hpx::execution::seq, high, low, layout1, layout2, layout_out);
    }

    void test_skewed_sequential_ranges()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<int> large(20);
        std::iota(large.begin(), large.end(), 0);
        std::vector<int> const small{5, 15};
        run_merge_case(hpx::execution::seq, large, small,
            partition_layout(4, {locs[0], locs[1]}),
            partition_layout(1, {locs[1]}), partition_layout(2, {locs[2]}));
        run_merge_case(hpx::execution::seq, small, large,
            partition_layout(1, {locs[0]}),
            partition_layout(4, {locs[1], locs[0]}),
            partition_layout(2, {locs[2]}));
    }
}    // namespace

int main()
{
    auto const localities = hpx::find_all_localities();
    HPX_TEST(localities.size() >= 3);
    if (localities.size() < 3)
    {
        return hpx::util::report_errors();
    }

    run_test("test_capture_second_input", test_capture_second_input);
    run_test("test_capture_first_input", test_capture_first_input);
    run_test("test_capture_both_inputs", test_capture_both_inputs);
    run_test("test_multi_partition_capture", test_multi_partition_capture);
    run_test("test_empty_input_ranges", test_empty_input_ranges);
    run_test("test_stable_merge", test_stable_merge);
    run_test(
        "test_multi_partition_stable_merge", test_multi_partition_stable_merge);
    run_test("test_statefull_comparator", test_statefull_comparator);
    run_test("test_ranges_merge_with_projections",
        test_ranges_merge_with_projections);
    run_test("test_different_input_types", test_different_input_types);
    run_test("test_partial_subranges", test_partial_subranges);
    run_test("test_both_inputs_empty", test_both_inputs_empty);
    run_test(
        "test_remote_comparator_exception", test_remote_comparator_exception);
    run_test("test_merge_no_policy", test_merge_no_policy);
    run_test("test_zero_sized_containers", test_zero_sized_containers);
    run_test("test_single_element_inputs", test_single_element_inputs);
    run_test("test_all_equal_keys", test_all_equal_keys);
    run_test("test_nonoverlapping_sequential_ranges",
        test_nonoverlapping_sequential_ranges);
    run_test("test_skewed_sequential_ranges", test_skewed_sequential_ranges);

    return hpx::util::report_errors();
}

#endif
