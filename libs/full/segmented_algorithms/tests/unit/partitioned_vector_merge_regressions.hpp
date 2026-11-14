//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shared types and helpers of the partitioned_vector_merge_regressions_*
// tests.

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/include/partitioned_vector.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/algorithms.hpp>
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
#include <exception>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// These types and their serialization are available in every locality's
// executable. Each test registers the partitioned_vector types it uses, at
// global scope.
struct merge_regression_value
{
    int primary = 0;
    int secondary = 0;
    int source = 0;
    int ordinal = 0;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & primary & secondary & source & ordinal;
    }
};

inline bool operator==(
    merge_regression_value const& lhs, merge_regression_value const& rhs)
{
    return lhs.primary == rhs.primary && lhs.secondary == rhs.secondary &&
        lhs.source == rhs.source && lhs.ordinal == rhs.ordinal;
}

struct regression_left_value
{
    int key = 0;
    int ordinal = 0;

    bool operator==(regression_left_value const&) const = default;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & ordinal;
    }
};

struct regression_right_value
{
    long key = 0;
    int ordinal = 0;

    bool operator==(regression_right_value const&) const = default;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & ordinal;
    }
};

struct regression_output_value
{
    long key = 0;
    int source = 0;
    int ordinal = 0;

    regression_output_value& operator=(regression_left_value const& value)
    {
        key = value.key;
        source = 1;
        ordinal = value.ordinal;
        return *this;
    }

    regression_output_value& operator=(regression_right_value const& value)
    {
        key = value.key;
        source = 2;
        ordinal = value.ordinal;
        return *this;
    }

    bool operator==(regression_output_value const&) const = default;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & source & ordinal;
    }
};

struct regression_left_projection
{
    int offset = 0;

    int operator()(regression_left_value const& value) const
    {
        return value.key + offset;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & offset;
    }
};

struct regression_right_projection
{
    long offset = 0;

    long operator()(regression_right_value const& value) const
    {
        return value.key + offset;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & offset;
    }
};

struct regression_two_key_less
{
    bool descending = false;

    bool operator()(merge_regression_value const& lhs,
        merge_regression_value const& rhs) const
    {
        if (lhs.primary != rhs.primary)
        {
            return descending ? lhs.primary > rhs.primary :
                                lhs.primary < rhs.primary;
        }
        return descending ? lhs.secondary > rhs.secondary :
                            lhs.secondary < rhs.secondary;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & descending;
    }
};

struct regression_bucket_less
{
    int width = 1;

    bool operator()(merge_regression_value const& lhs,
        merge_regression_value const& rhs) const
    {
        return lhs.primary / width < rhs.primary / width;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & width;
    }
};

struct regression_int_less
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

struct regression_key_projection
{
    int offset = 0;

    int operator()(merge_regression_value const& value) const
    {
        return value.primary + offset;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & offset;
    }
};

// Select where a failure happens, rather than accidentally testing only a
// comparison in the coordinator's diagonal search. The threshold also lets
// the first output chunk succeed before a later diagonal search fails.
struct regression_throw_point
{
    hpx::id_type locality;
    bool everywhere = false;
    bool allocation_failure = false;
    int threshold = (std::numeric_limits<int>::min)();

    void throw_if_needed(int value) const
    {
        if (value >= threshold && (everywhere || hpx::find_here() == locality))
        {
            if (allocation_failure)
            {
                throw std::bad_alloc();
            }
            throw std::runtime_error("segmented-merge-regression-error");
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & locality & everywhere & allocation_failure & threshold;
    }
};

struct regression_throwing_compare
{
    regression_throw_point point;

    bool operator()(int lhs, int rhs) const
    {
        point.throw_if_needed((std::max) (lhs, rhs));
        return lhs < rhs;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & point;
    }
};

struct regression_throwing_projection
{
    regression_throw_point point;

    int operator()(int value) const
    {
        point.throw_if_needed(value);
        return value;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & point;
    }
};

struct regression_non_default_key
{
    regression_non_default_key() = delete;

    explicit regression_non_default_key(int value)
      : value(value)
    {
    }

    regression_non_default_key(regression_non_default_key const&) = default;
    regression_non_default_key(regression_non_default_key&&) = default;

    regression_non_default_key& operator=(
        regression_non_default_key const&) = default;
    regression_non_default_key& operator=(
        regression_non_default_key&&) = default;

    int value;

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
        // value is serialized as construction data below.
    }
};

static_assert(!std::is_default_constructible_v<regression_non_default_key>);

struct regression_non_default_projection
{
    regression_non_default_key operator()(int value) const
    {
        return regression_non_default_key(value);
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

struct regression_non_default_less
{
    bool operator()(regression_non_default_key const& lhs,
        regression_non_default_key const& rhs) const
    {
        return lhs.value < rhs.value;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

struct regression_heap_value
{
    std::string key;
    int source = 0;
    int ordinal = 0;

    bool operator==(regression_heap_value const& other) const
    {
        return key == other.key && source == other.source &&
            ordinal == other.ordinal;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & key & source & ordinal;
    }
};

inline bool operator<(
    regression_heap_value const& lhs, regression_heap_value const& rhs)
{
    return lhs.key < rhs.key;
}

namespace hpx::serialization {

    template <typename Archive>
    void save_construct_data(
        Archive& ar, regression_non_default_key const* key, unsigned)
    {
        ar << key->value;
    }

    template <typename Archive>
    void load_construct_data(
        Archive& ar, regression_non_default_key* key, unsigned)
    {
        int value;
        ar >> value;

        ::new (key) regression_non_default_key(value);
    }
}    // namespace hpx::serialization

namespace merge_regression {

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
            // No ostream overload is needed for the tagged record type.
            if (!(values[i] == expected[i]))
            {
                std::fprintf(stderr, "[regression] mismatch at index %zu\n", i);
                HPX_TEST(values[i] == expected[i]);
            }
        }
    }

    template <typename T>
    hpx::partitioned_vector<T> make_vector(std::vector<T> const& values,
        std::vector<std::size_t> const& sizes,
        std::vector<hpx::id_type> const& owners)
    {
        // Exactly one owner entry per partition, with no zero-size partitions.
        HPX_TEST_EQ(sizes.size(), owners.size());
        HPX_TEST_EQ(std::accumulate(sizes.begin(), sizes.end(), std::size_t(0)),
            values.size());
        HPX_TEST(std::all_of(sizes.begin(), sizes.end(),
            [](std::size_t size) { return size != 0; }));

        hpx::partitioned_vector<T> result(
            values.size(), hpx::explicit_container_layout(sizes, owners));
        HPX_TEST(result.get_partition_sizes() == sizes);
        assign_values(result, values);
        return result;
    }

    template <typename F>
    void for_each_policy(F&& f)
    {
        auto run = [&](char const* name, auto policy) {
            std::fprintf(stderr, "[regression] policy=%s\n", name);
            std::fflush(stderr);
            f(policy);
        };
        run("seq", hpx::execution::seq);
        run("par", hpx::execution::par);
        run("seq(task)", hpx::execution::seq(hpx::execution::task));
        run("par(task)", hpx::execution::par(hpx::execution::task));
    }

    template <typename Policy, typename Result>
    auto finish_result(Policy const&, Result result)
    {
        if constexpr (hpx::is_async_execution_policy_v<std::decay_t<Policy>>)
        {
            return result.get();
        }
        else
        {
            return result;
        }
    }

    inline void run_test(char const* name, void (*test)())
    {
        auto const start = std::chrono::steady_clock::now();
        std::fprintf(stderr, "[merge-regression] start %s\n", name);
        std::fflush(stderr);
        test();
        double const elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start)
                                   .count();
        std::fprintf(
            stderr, "[merge-regression] done %s (%.3fs)\n", name, elapsed);
        std::fflush(stderr);
    }

    // The tests place partitions on three different localities.
    inline bool has_three_localities()
    {
        auto const localities = hpx::find_all_localities();
        HPX_TEST(localities.size() >= 3);
        if (localities.size() < 3)
        {
            std::fprintf(
                stderr, "This test requires at least three HPX localities.\n");
            return false;
        }
        return true;
    }
}    // namespace merge_regression

#endif
