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
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// These types and their serialization are available in every locality's
// executable. Keep component registration at global scope.
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

bool operator==(
    merge_regression_value const& lhs, merge_regression_value const& rhs)
{
    return lhs.primary == rhs.primary && lhs.secondary == rhs.secondary &&
        lhs.source == rhs.source && lhs.ordinal == rhs.ordinal;
}

HPX_REGISTER_PARTITIONED_VECTOR(merge_regression_value);

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

HPX_REGISTER_PARTITIONED_VECTOR(regression_left_value);
HPX_REGISTER_PARTITIONED_VECTOR(regression_right_value);
HPX_REGISTER_PARTITIONED_VECTOR(regression_output_value);

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

    void check(int value) const
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
        point.check((std::max) (lhs, rhs));
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
        point.check(value);
        return value;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & point;
    }
};

namespace {

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

    bool contains_only_expected_errors(std::exception_ptr const& error)
    {
        try
        {
            std::rethrow_exception(error);
        }
        catch (hpx::exception_list const& errors)
        {
            if (errors.size() == 0)
            {
                return false;
            }
            for (auto const& nested : errors)
            {
                if (!contains_only_expected_errors(nested))
                {
                    return false;
                }
            }
            return true;
        }
        catch (std::runtime_error const& error)
        {
            return std::string(error.what())
                       .find("segmented-merge-regression-error") !=
                std::string::npos;
        }
        catch (...)
        {
            return false;
        }
    }

    template <typename Policy, typename Start>
    void expect_failure(Policy const&, Start&& start, bool allocation_failure,
        bool check_exception_contents)
    {
        bool returned_from_call = false;
        bool caught = false;
        try
        {
            auto result = start();
            returned_from_call = true;
            if constexpr (hpx::is_async_execution_policy_v<
                              std::decay_t<Policy>>)
            {
                HPX_TEST(result.valid());
                result.get();
            }
            else
            {
                (void) result;
            }
        }
        catch (std::bad_alloc const&)
        {
            caught = true;
            HPX_TEST(allocation_failure);
        }
        catch (hpx::exception_list const& errors)
        {
            caught = true;
            HPX_TEST(!allocation_failure);
            HPX_TEST(errors.size() != 0);

            if (check_exception_contents)
            {
                HPX_TEST(
                    contains_only_expected_errors(std::current_exception()));
            }
        }
        catch (std::exception const& error)
        {
            caught = true;
            std::fprintf(stderr, "[regression] unexpected exception: %s\n",
                error.what());
            HPX_TEST(false);
        }
        catch (...)
        {
            caught = true;
            HPX_TEST(false);
        }
        HPX_TEST(caught);
        if constexpr (hpx::is_async_execution_policy_v<std::decay_t<Policy>>)
        {
            // A user-function failure belongs in the future, not in the
            // initial seq(task)/par(task) call. No timing/!is_ready assumption.
            HPX_TEST(returned_from_call);
        }
    }

    struct exception_fixture
    {
        std::vector<hpx::id_type> localities = hpx::find_all_localities();
        hpx::id_type remote_locality;
        std::vector<int> input1;
        std::vector<int> input2;
        hpx::partitioned_vector<int> source1;
        hpx::partitioned_vector<int> source2;
        hpx::partitioned_vector<int> destination;

        exception_fixture()
        {
            for (auto const& locality : localities)
            {
                if (locality != hpx::find_here())
                {
                    remote_locality = locality;
                }
            }
            for (int i = 0; i != 16; ++i)
            {
                input1.push_back(2 * i);
                input2.push_back(2 * i + 1);
            }
            source1 =
                make_vector(input1, {8, 8}, {localities[0], localities[1]});
            source2 =
                make_vector(input2, {8, 8}, {localities[1], localities[0]});
            destination = make_vector(std::vector<int>(32, -1), {8, 8, 8, 8},
                {remote_locality, remote_locality, remote_locality,
                    remote_locality});
            HPX_TEST(hpx::find_here() != remote_locality);
        }

        template <typename Policy>
        void run(Policy policy, bool remote, bool projection,
            bool allocation_failure, int threshold)
        {
            std::fprintf(stderr,
                "[regression] exception-site=%s functor=%s "
                "kind=%s threshold=%d\n",
                remote ? "destination" : "coordinator",
                projection ? "projection" : "comparator",
                allocation_failure ? "bad_alloc" : "runtime_error", threshold);
            std::fflush(stderr);

            regression_throw_point point;
            point.locality = remote ? remote_locality : hpx::find_here();
            point.allocation_failure = allocation_failure;
            point.threshold = threshold;
            if (projection)
            {
                expect_failure(
                    policy,
                    [&] {
                        return hpx::ranges::merge(policy, source1.cbegin(),
                            source1.cend(), source2.cbegin(), source2.cend(),
                            destination.begin(), regression_int_less{},
                            regression_throwing_projection{point},
                            regression_throwing_projection{point});
                    },
                    allocation_failure, !remote);
            }
            else
            {
                expect_failure(
                    policy,
                    [&] {
                        return hpx::merge(policy, source1.cbegin(),
                            source1.cend(), source2.cbegin(), source2.cend(),
                            destination.begin(),
                            regression_throwing_compare{point});
                    },
                    allocation_failure, !remote);
            }
            // Output after failure is unspecified: do not require rollback.
            // Each task future is consumed before these containers are reused.
        }

        void check_inputs() const
        {
            check_values(source1, input1);
            check_values(source2, input2);
        }
    };

    void test_diagonal_comparator_exceptions()
    {
        exception_fixture fixture;
        for_each_policy([&](auto policy) {
            fixture.run(policy, false, false, false, 0);
            // k=8 succeeds; k=16 fails. Earlier chunks may already be launched.
            fixture.run(policy, false, false, false, 12);
        });
        fixture.check_inputs();
    }

    void test_diagonal_projection_exceptions()
    {
        exception_fixture fixture;
        for_each_policy(
            [&](auto policy) { fixture.run(policy, false, true, false, 12); });
        fixture.check_inputs();
    }

    void test_remote_functor_exceptions()
    {
        exception_fixture fixture;
        for_each_policy([&](auto policy) {
            // Caller-side comparisons succeed. Fail in a remote output chunk.
            fixture.run(policy, true, false, false, 12);
            fixture.run(policy, true, true, false, 12);
        });
        fixture.check_inputs();
    }

    void test_bad_alloc_propagation()
    {
        exception_fixture fixture;
        for_each_policy([&](auto policy) {
            for (bool remote : {false, true})
            {
                fixture.run(policy, remote, false, true, 12);
                fixture.run(policy, remote, true, true, 12);
            }
        });
        fixture.check_inputs();
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

    void run_test(char const* name, void (*test)())
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
}    // namespace

int main()
{
    auto const localities = hpx::find_all_localities();
    HPX_TEST(localities.size() >= 3);
    if (localities.size() < 3)
    {
        std::fprintf(
            stderr, "This test requires at least three HPX localities.\n");
        return hpx::util::report_errors();
    }

    run_test("test_const_subranges_and_boundary_guards",
        test_const_subranges_and_boundary_guards);
    run_test("test_empty_end_ranges_do_not_compare_or_project",
        test_empty_end_ranges_do_not_compare_or_project);
    run_test(
        "test_two_key_descending_stability", test_two_key_descending_stability);
    run_test("test_comparator_equivalence_across_tiny_partitions",
        test_comparator_equivalence_across_tiny_partitions);
    run_test("test_distinct_stateful_projections",
        test_distinct_stateful_projections);
    run_test("test_heterogeneous_projected_policy_matrix",
        test_heterogeneous_projected_policy_matrix);
    run_test("test_diagonal_comparator_exceptions",
        test_diagonal_comparator_exceptions);
    run_test("test_diagonal_projection_exceptions",
        test_diagonal_projection_exceptions);
    run_test("test_remote_functor_exceptions", test_remote_functor_exceptions);
    run_test("test_bad_alloc_propagation", test_bad_alloc_propagation);
    run_test("test_two_task_merges_before_waiting",
        test_two_task_merges_before_waiting);

    return hpx::util::report_errors();
}

#endif
