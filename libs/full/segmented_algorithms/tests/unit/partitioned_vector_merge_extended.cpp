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
#include <hpx/parallel/segmented_algorithms/detail/capture_dispatch.hpp>

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

struct byte_batch_test_algorithm
{
    using result_type = std::size_t;
};

struct byte_batch_test_chunk
{
    std::size_t input1_size;
    std::size_t input2_size;

    std::vector<int> ranges1;
    std::vector<int> ranges2;

    std::size_t dest;
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

    template <typename TaskPolicy>
    void run_task_empty_side_case(TaskPolicy policy, bool first_is_empty,
        hpx::id_type const& source1_locality,
        hpx::id_type const& source2_locality,
        hpx::id_type const& destination_locality)
    {
        std::vector<int> values;
        values.reserve(512);

        for (int i = 0; i != 512; ++i)
        {
            values.push_back(2 * i);
        }

        std::vector<int> const ignored{10000};

        std::size_t const source1_partitions = first_is_empty ? 1 : 4;

        std::size_t const source2_partitions = first_is_empty ? 4 : 1;

        auto const source1_layout = hpx::container_layout(
            source1_partitions, std::vector<hpx::id_type>{source1_locality});

        auto const source2_layout = hpx::container_layout(
            source2_partitions, std::vector<hpx::id_type>{source2_locality});

        auto const destination_layout = hpx::container_layout(
            4, std::vector<hpx::id_type>{destination_locality});

        hpx::partitioned_vector<int> source1(
            first_is_empty ? ignored.size() : values.size(), source1_layout);

        hpx::partitioned_vector<int> source2(
            first_is_empty ? values.size() : ignored.size(), source2_layout);

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

        auto result_future = hpx::merge(HPX_MOVE(policy), first1, last1, first2,
            last2, destination.begin());

        auto result = result_future.get();

        HPX_TEST(result == destination.end());
        check_values(destination, values);
    }

    void test_parallel_multi_destination_partitions()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<int> input1;
        std::vector<int> input2;
        std::vector<int> expected;

        // input1 = 1, 3, 5, ..., 47
        // input2 = 2, 4, 6, ..., 48
        // expected = 1, 2, 3, ..., 48
        for (int value = 1; value <= 48; ++value)
        {
            expected.push_back(value);

            if (value % 2 == 0)
            {
                input2.push_back(value);
            }
            else
            {
                input1.push_back(value);
            }
        }

        // Input partitions alternate between localities 0 and 1.
        auto const source1_layout = partition_layout(
            6, std::vector<hpx::id_type>{localities[0], localities[1]});

        auto const source2_layout = partition_layout(
            6, std::vector<hpx::id_type>{localities[1], localities[0]});

        // Six output partitions are all located on locality 2.
        // The parallel merge should process these output chunks concurrently.
        auto const destination_layout =
            hpx::container_layout(6, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(hpx::execution::par, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void test_multi_locality_destination_partitions()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<int> input1;
        std::vector<int> input2;
        std::vector<int> expected;

        // input1 = 0, 2, 4, ..., 94
        // input2 = 1, 3, 5, ..., 95
        // expected = 0, 1, 2, ..., 95
        for (int value = 0; value != 96; ++value)
        {
            expected.push_back(value);

            if (value % 2 == 0)
            {
                input1.push_back(value);
            }
            else
            {
                input2.push_back(value);
            }
        }

        // All input1 partitions are on locality 0.
        auto const source1_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[0]});

        // All input2 partitions are on locality 1.
        auto const source2_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[1]});

        // The six destination partitions use the available localities.
        auto const destination_layout = partition_layout(6, localities);

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        // Test the ordinary parallel policy.
        {
            std::vector<int> const initial_values(expected.size(), -1);

            assign_values(destination, initial_values);

            auto result =
                hpx::merge(hpx::execution::par, source1.begin(), source1.end(),
                    source2.begin(), source2.end(), destination.begin());

            HPX_TEST(result == destination.end());
            check_values(destination, expected);
        }

        // Test the parallel task policy with the same distributed output.
        {
            std::vector<int> const initial_values(expected.size(), -1);

            assign_values(destination, initial_values);

            auto result_future =
                hpx::merge(hpx::execution::par(hpx::execution::task),
                    source1.begin(), source1.end(), source2.begin(),
                    source2.end(), destination.begin());

            auto result = result_future.get();

            HPX_TEST(result == destination.end());
            check_values(destination, expected);
        }
    }

    void test_sequenced_task_merge()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        constexpr int input_size = 2048;

        std::vector<int> input1;
        std::vector<int> input2;
        std::vector<int> expected;

        input1.reserve(input_size);
        input2.reserve(input_size);
        expected.reserve(2 * static_cast<std::size_t>(input_size));

        for (int i = 0; i != input_size; ++i)
        {
            input1.push_back(2 * i);
            input2.push_back(2 * i + 1);
        }

        for (int i = 0; i != 2 * input_size; ++i)
        {
            expected.push_back(i);
        }

        auto const source1_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(1, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result_future = hpx::merge(
            hpx::execution::seq(hpx::execution::task), source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        // Waiting is necessary before reading the destination.
        auto result = result_future.get();

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void test_parallel_task_merge()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        constexpr int input_size = 2048;

        std::vector<int> input1;
        std::vector<int> input2;
        std::vector<int> expected;

        input1.reserve(input_size);
        input2.reserve(input_size);
        expected.reserve(2 * static_cast<std::size_t>(input_size));

        for (int i = 0; i != input_size; ++i)
        {
            input1.push_back(2 * i);
            input2.push_back(2 * i + 1);
        }

        for (int i = 0; i != 2 * input_size; ++i)
        {
            expected.push_back(i);
        }

        // Every input-1 partition is on locality 0.
        auto const source1_layout =
            hpx::container_layout(8, std::vector<hpx::id_type>{localities[0]});

        // Every input-2 partition is on locality 1.
        auto const source2_layout =
            hpx::container_layout(8, std::vector<hpx::id_type>{localities[1]});

        // Multiple output partitions on locality 2 allow output chunks
        // to execute concurrently.
        auto const destination_layout =
            hpx::container_layout(8, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result_future = hpx::merge(
            hpx::execution::par(hpx::execution::task), source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        auto result = result_future.get();

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void test_disjoint_parallel_chunks()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::vector<int> input1;
        std::vector<int> input2;
        std::vector<int> expected;

        for (int value = 1; value <= 16; ++value)
        {
            input1.push_back(value);
            expected.push_back(value);
        }

        for (int value = 101; value <= 116; ++value)
        {
            input2.push_back(value);
            expected.push_back(value);
        }

        auto const source1_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[0]});

        auto const source2_layout =
            hpx::container_layout(4, std::vector<hpx::id_type>{localities[1]});

        auto const destination_layout =
            hpx::container_layout(8, std::vector<hpx::id_type>{localities[2]});

        hpx::partitioned_vector<int> source1(input1.size(), source1_layout);

        hpx::partitioned_vector<int> source2(input2.size(), source2_layout);

        hpx::partitioned_vector<int> destination(
            expected.size(), destination_layout);

        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(hpx::execution::par, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        check_values(destination, expected);
    }

    void test_task_empty_side_paths()
    {
        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        run_task_empty_side_case(hpx::execution::seq(hpx::execution::task),
            true, localities[0], localities[1], localities[2]);

        run_task_empty_side_case(hpx::execution::seq(hpx::execution::task),
            false, localities[0], localities[1], localities[2]);

        run_task_empty_side_case(hpx::execution::par(hpx::execution::task),
            true, localities[0], localities[1], localities[2]);

        run_task_empty_side_case(hpx::execution::par(hpx::execution::task),
            false, localities[0], localities[1], localities[2]);
    }

    void test_randomized_distributed_merge()
    {
        auto const randomized_start = std::chrono::steady_clock::now();

        char const* const policy_names[] = {
            "seq", "par", "seq(task)", "par(task)"};

        auto const localities = hpx::find_all_localities();

        HPX_TEST(localities.size() >= 3);

        if (localities.size() < 3)
        {
            return;
        }

        std::mt19937 generator(2026);
        std::uniform_int_distribution<int> length_distribution(16, 96);
        std::uniform_int_distribution<int> value_distribution(0, 30);

        constexpr std::size_t test_count = 24;

        for (std::size_t test = 0; test != test_count; ++test)
        {
            std::size_t const size1 =
                static_cast<std::size_t>(length_distribution(generator));

            std::size_t const size2 =
                static_cast<std::size_t>(length_distribution(generator));

            std::vector<int> input1(size1);
            std::vector<int> input2(size2);

            for (int& value : input1)
            {
                value = value_distribution(generator);
            }

            for (int& value : input2)
            {
                value = value_distribution(generator);
            }

            std::sort(input1.begin(), input1.end());
            std::sort(input2.begin(), input2.end());

            std::vector<int> expected(size1 + size2);

            std::merge(input1.begin(), input1.end(), input2.begin(),
                input2.end(), expected.begin());

            std::size_t const partitions1 = 1 + test % 5;
            std::size_t const partitions2 = 1 + (test + 2) % 5;
            std::size_t const destination_partitions = 1 + (test + 3) % 6;

            auto log_phase = [&](char const* phase) {
                double const elapsed = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - randomized_start)
                                           .count();

                std::fprintf(stderr,
                    "[randomized] case=%zu/%zu policy=%s "
                    "sizes=%zu,%zu partitions=%zu,%zu,%zu "
                    "elapsed=%.3fs phase=%s\n",
                    test + 1, test_count, policy_names[test % 4], size1, size2,
                    partitions1, partitions2, destination_partitions, elapsed,
                    phase);

                std::fflush(stderr);
            };

            auto const source1_layout =
                partition_layout(partitions1, localities);

            auto const source2_layout =
                partition_layout(partitions2, localities);

            auto const destination_layout =
                partition_layout(destination_partitions, localities);

            log_phase("construct and initialize source1");
            hpx::partitioned_vector<int> source1(
                input1.cbegin(), input1.cend(), source1_layout);

            log_phase("construct and initialize source2");
            hpx::partitioned_vector<int> source2(
                input2.cbegin(), input2.cend(), source2_layout);

            log_phase("construct destination");
            hpx::partitioned_vector<int> destination(
                expected.size(), destination_layout);

            log_phase("merge");

            auto result = [&]() {
                switch (test % 4)
                {
                case 0:
                    return hpx::merge(hpx::execution::seq, source1.begin(),
                        source1.end(), source2.begin(), source2.end(),
                        destination.begin());

                case 1:
                    return hpx::merge(hpx::execution::par, source1.begin(),
                        source1.end(), source2.begin(), source2.end(),
                        destination.begin());

                case 2:
                    return hpx::merge(hpx::execution::seq(hpx::execution::task),
                        source1.begin(), source1.end(), source2.begin(),
                        source2.end(), destination.begin())
                        .get();

                default:
                    return hpx::merge(hpx::execution::par(hpx::execution::task),
                        source1.begin(), source1.end(), source2.begin(),
                        source2.end(), destination.begin())
                        .get();
                }
            }();
            log_phase("verify");
            HPX_TEST(result == destination.end());
            check_values(destination, expected);
            log_phase("case complete");
        }
    }

    template <typename Policy>
    void run_tagged_stability_case(Policy policy, bool all_equal)
    {
        auto const locs = hpx::find_all_localities();
        std::vector<hpx::id_type> const owners{
            locs[0], locs[1], locs[2], locs[3]};
        std::vector<stable_value> input1;
        std::vector<stable_value> input2;
        for (int i = 0; i != 16; ++i)
        {
            int const key = all_equal ? 7 : i / 3;
            input1.push_back({key, 1, i});
            input2.push_back({key, 2, i});
        }
        run_merge_case(policy, input1, input2, partition_layout(8, owners),
            partition_layout(8, owners), partition_layout(8, owners));
    }

    void test_tagged_stability_policy_matrix()
    {
        for (bool all_equal : {false, true})
        {
            run_tagged_stability_case(hpx::execution::seq, all_equal);
            run_tagged_stability_case(hpx::execution::par, all_equal);
            run_tagged_stability_case(
                hpx::execution::seq(hpx::execution::task), all_equal);
            run_tagged_stability_case(
                hpx::execution::par(hpx::execution::task), all_equal);
        }
    }

    template <typename Policy>
    void run_numeric_mixed_type_case(Policy policy)
    {
        auto const locs = hpx::find_all_localities();
        std::vector<int> const input1{1, 2, 2, 5, 8, 13};
        std::vector<long long> const input2{0, 2, 3, 8, 21};
        std::vector<long long> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        hpx::partitioned_vector<int> source1(
            input1.size(), partition_layout(2, {locs[0]}));
        hpx::partitioned_vector<long long> source2(
            input2.size(), partition_layout(1, {locs[1]}));
        hpx::partitioned_vector<long long> destination(expected.size(),
            partition_layout(4, {locs[0], locs[1], locs[2], locs[3]}));
        assign_values(source1, input1);
        assign_values(source2, input2);

        // Exercise hpx::merge itself, not the projected ranges overload.
        auto result =
            hpx::merge(policy, source1.begin(), source1.end(), source2.begin(),
                source2.end(), destination.begin(), hpx::ranges::less{});
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

    void test_numeric_mixed_type_policy_matrix()
    {
        run_numeric_mixed_type_case(hpx::execution::seq);
        run_numeric_mixed_type_case(hpx::execution::par);
        run_numeric_mixed_type_case(hpx::execution::seq(hpx::execution::task));
        run_numeric_mixed_type_case(hpx::execution::par(hpx::execution::task));
    }

    template <typename Policy>
    void run_zero_sized_container_cases(Policy policy)
    {
        auto const locs = hpx::find_all_localities();
        auto const layout1 = partition_layout(1, {locs[0]});
        auto const layout2 = partition_layout(1, {locs[1]});
        // Nonempty output spans two localities; empty output has no data.
        auto const layout_out = partition_layout(2, {locs[2], locs[3]});
        std::vector<int> const empty;
        std::vector<int> const values{1, 2, 3, 5, 8, 13};
        run_merge_case(policy, empty, values, layout1, layout2, layout_out);
        run_merge_case(policy, values, empty, layout1, layout2, layout_out);
        run_merge_case(policy, empty, empty, layout1, layout2, layout_out);
    }

    void test_zero_sized_containers_parallel_and_task()
    {
        run_zero_sized_container_cases(hpx::execution::par);
        run_zero_sized_container_cases(
            hpx::execution::seq(hpx::execution::task));
        run_zero_sized_container_cases(
            hpx::execution::par(hpx::execution::task));
    }

    std::vector<int> progression(std::size_t count, int step, int offset)
    {
        std::vector<int> values;
        values.reserve(count);
        for (std::size_t i = 0; i != count; ++i)
        {
            values.push_back(step * static_cast<int>(i) + offset);
        }
        return values;
    }

    void test_four_locality_ownership_patterns()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<hpx::id_type> const all{locs[0], locs[1], locs[2], locs[3]};
        auto const input1 = progression(24, 2, 0);
        auto const input2 = progression(24, 2, 1);

        // Inputs have disjoint two-locality owner sets.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(2, {locs[0], locs[1]}),
            partition_layout(2, {locs[2], locs[3]}), partition_layout(4, all));

        // Neither destination locality owns either input.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(1, {locs[0]}), partition_layout(1, {locs[1]}),
            partition_layout(2, {locs[2], locs[3]}));

        // Both inputs on one locality, distributed output.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(1, {locs[0]}), partition_layout(1, {locs[0]}),
            partition_layout(4, all));

        // Distributed inputs, one destination locality.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(4, all), partition_layout(4, all),
            partition_layout(1, {locs[0]}));

        // Both inputs colocated, but all output is remote.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(1, {locs[0]}), partition_layout(1, {locs[0]}),
            partition_layout(1, {locs[1]}));

        // Sequential counterpart of the existing distributed-output test.
        run_merge_case(hpx::execution::seq, input1, input2,
            partition_layout(1, {locs[0]}), partition_layout(1, {locs[1]}),
            partition_layout(4, all));

        run_merge_case(hpx::execution::par(hpx::execution::task), input1,
            input2, partition_layout(4, all), partition_layout(4, all),
            partition_layout(4, all));
    }

    template <typename T>
    void check_partition_owners(hpx::partitioned_vector<T>& values,
        std::vector<hpx::id_type> const& owners)
    {
        using traits = hpx::traits::segmented_iterator_traits<
            typename hpx::partitioned_vector<T>::iterator>;
        auto const sizes = values.get_partition_sizes();
        HPX_TEST_EQ(sizes.size(), owners.size());
        if (sizes.size() != owners.size())
        {
            return;
        }
        auto segment = traits::segment(values.begin());
        for (std::size_t i = 0; i != owners.size(); ++i, ++segment)
        {
            auto const owner = hpx::get_colocation_id(
                hpx::launch::sync, traits::get_id(segment));
            HPX_TEST(owner == owners[i]);
        }
    }

    void test_verified_noncontiguous_partitions()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<hpx::id_type> const owners8{locs[0], locs[1], locs[2],
            locs[3], locs[0], locs[1], locs[2], locs[3]};
        std::vector<hpx::id_type> const owners4{
            locs[0], locs[1], locs[2], locs[3]};

        // Each complete input is sorted. The original unsorted examples are
        // deliberately not copied: merge does not sort individual partitions.
        auto const input1 = progression(24, 2, 0);
        auto const input2 = progression(8, 4, 1);
        std::vector<int> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        hpx::partitioned_vector<int> source1(
            input1.size(), hpx::container_layout(8, owners8));
        hpx::partitioned_vector<int> source2(
            input2.size(), hpx::container_layout(4, owners4));
        hpx::partitioned_vector<int> destination(
            expected.size(), hpx::container_layout(8, owners8));
        check_partition_owners(source1, owners8);
        check_partition_owners(source2, owners4);
        check_partition_owners(destination, owners8);
        assign_values(source1, input1);
        assign_values(source2, input2);

        auto result = hpx::merge(hpx::execution::seq, source1.begin(),
            source1.end(), source2.begin(), source2.end(), destination.begin());
        HPX_TEST(result == destination.end());
        check_values(destination, expected);

        assign_values(destination, std::vector<int>(expected.size(), -1));
        auto future = hpx::merge(hpx::execution::par(hpx::execution::task),
            source1.begin(), source1.end(), source2.begin(), source2.end(),
            destination.begin());
        HPX_TEST(future.get() == destination.end());
        check_values(destination, expected);
    }

    void test_many_small_partitions_and_skew()
    {
        auto const locs = hpx::find_all_localities();
        std::vector<hpx::id_type> const owners{
            locs[0], locs[1], locs[2], locs[3]};
        auto const layout8 = partition_layout(8, owners);
        auto const layout4 = partition_layout(4, owners);

        run_merge_case(hpx::execution::seq, progression(24, 2, 0),
            progression(24, 2, 1), layout8, layout8, layout8);
        run_merge_case(hpx::execution::seq, progression(120, 3, 0),
            progression(80, 3, 1), layout8, layout8, layout8);
        run_merge_case(hpx::execution::seq, progression(32, 1, 0),
            std::vector<int>{5, 15, 25, 35}, layout8, layout4, layout4);
    }

    void test_capture_byte_batch_grouping()
    {
        using policy_type = std::decay_t<decltype(hpx::execution::seq)>;

        using receiver_type = hpx::parallel::detail::batch_receiver<int, double,
            byte_batch_test_chunk, byte_batch_test_algorithm, policy_type,
            std::true_type>;

        constexpr std::size_t max_bytes =
            hpx::parallel::detail::max_capture_batch_bytes;

        constexpr std::size_t quarter_int_count = (max_bytes / 4) / sizeof(int);

        constexpr std::size_t quarter_double_count =
            (max_bytes / 4) / sizeof(double);

        constexpr std::size_t half_int_count = (max_bytes / 2) / sizeof(int);

        std::vector<byte_batch_test_chunk> chunks;

        // Estimated payload: one half of the limit.
        chunks.push_back(byte_batch_test_chunk{
            quarter_int_count, quarter_double_count, {}, {}, 10});

        // Estimated payload: another half of the limit.
        // This chunk fits in the first batch because
        // the combined size is exactly the limit.
        chunks.push_back(byte_batch_test_chunk{half_int_count, 0, {}, {}, 20});

        // This additional value exceeds the first
        //batch's remaining capacity
        // and must therefore begin a second batch.
        chunks.push_back(byte_batch_test_chunk{0, 1, {}, {}, 30});

        auto batches = receiver_type::make_byte_batches(HPX_MOVE(chunks));

        HPX_TEST_EQ(batches.size(), std::size_t{2});

        HPX_TEST_EQ(batches[0].size(), std::size_t{2});
        HPX_TEST_EQ(batches[1].size(), std::size_t{1});

        // Verify that batching preserves the original chunk order.
        HPX_TEST_EQ(batches[0][0].dest, std::size_t{10});
        HPX_TEST_EQ(batches[0][1].dest, std::size_t{20});
        HPX_TEST_EQ(batches[1][0].dest, std::size_t{30});

        constexpr std::size_t max_elements =
            hpx::parallel::detail::max_capture_batch_elements<int, double>();
        // Verify the planner uses the larger input element size.
        HPX_TEST_EQ(max_elements, max_bytes / sizeof(double));
    }
}    // namespace

int main()
{
    auto const localities = hpx::find_all_localities();
    HPX_TEST(localities.size() >= 4);
    if (localities.size() < 4)
    {
        return hpx::util::report_errors();
    }

    run_test("test_parallel_multi_destination_partitions",
        test_parallel_multi_destination_partitions);
    run_test("test_multi_locality_destination_partitions",
        test_multi_locality_destination_partitions);
    run_test("test_sequenced_task_merge", test_sequenced_task_merge);
    run_test("test_parallel_task_merge", test_parallel_task_merge);
    run_test("test_disjoint_parallel_chunks", test_disjoint_parallel_chunks);
    run_test("test_task_empty_side_paths", test_task_empty_side_paths);
    run_test(
        "test_randomized_distributed_merge", test_randomized_distributed_merge);
    run_test("test_tagged_stability_policy_matrix",
        test_tagged_stability_policy_matrix);
    run_test("test_numeric_mixed_type_policy_matrix",
        test_numeric_mixed_type_policy_matrix);
    run_test("test_zero_sized_containers_parallel_and_task",
        test_zero_sized_containers_parallel_and_task);
    run_test("test_four_locality_ownership_patterns",
        test_four_locality_ownership_patterns);
    run_test("test_verified_noncontiguous_partitions",
        test_verified_noncontiguous_partitions);
    run_test("test_many_small_partitions_and_skew",
        test_many_small_partitions_and_skew);
    run_test(
        "test_capture_byte_batch_grouping", test_capture_byte_batch_grouping);

    return hpx::util::report_errors();
}

#endif
