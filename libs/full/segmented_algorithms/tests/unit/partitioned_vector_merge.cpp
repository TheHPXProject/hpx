//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

struct bucket_less
{
    int bucket_size = 10;

    bool operator()(int lhs, int rhs) const
    {
        return lhs / bucket_size < rhs / bucket_size;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & bucket_size;
    }
};

struct divide_projection
{
    int divisor = 1;

    int operator()(int value) const
    {
        return value / divisor;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & divisor;
    }
};

namespace {

    template <typename T>
    void assign_values(
        hpx::partitioned_vector<T>& values, std::vector<T> const& source)
    {
        HPX_TEST_EQ(values.size(), source.size());

        auto out = values.begin();
        for (T const& value : source)
        {
            *out++ = value;
        }
    }

    template <typename T>
    std::vector<T> copy_values(hpx::partitioned_vector<T> const& values)
    {
        std::vector<T> result;
        result.reserve(values.size());

        for (auto it = values.begin(); it != values.end(); ++it)
        {
            result.push_back(*it);
        }
        return result;
    }

    template <typename Iterator>
    Iterator advanced(
        Iterator it, typename std::iterator_traits<Iterator>::difference_type n)
    {
        std::advance(it, n);
        return it;
    }

    template <typename DistPolicy1, typename DistPolicy2, typename DistPolicy3,
        typename ExPolicy, typename Comp>
    void test_merge_policy(std::vector<int> const& input1,
        std::vector<int> const& input2, DistPolicy1 const& dist1,
        DistPolicy2 const& dist2, DistPolicy3 const& dist3,
        ExPolicy const& policy, Comp comp)
    {
        hpx::partitioned_vector<int> source1(input1.size(), dist1);
        hpx::partitioned_vector<int> source2(input2.size(), dist2);
        hpx::partitioned_vector<int> destination(
            input1.size() + input2.size(), dist3);

        assign_values(source1, input1);
        assign_values(source2, input2);

        std::vector<int> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), comp);

        auto result = hpx::merge(policy, source1.begin(), source1.end(),
            source2.begin(), source2.end(), destination.begin(), comp);

        HPX_TEST(result == destination.end());
        HPX_TEST(copy_values(destination) == expected);
    }

    template <typename DistPolicy1, typename DistPolicy2, typename DistPolicy3,
        typename ExPolicy, typename Comp>
    void test_merge_task_policy(std::vector<int> const& input1,
        std::vector<int> const& input2, DistPolicy1 const& dist1,
        DistPolicy2 const& dist2, DistPolicy3 const& dist3,
        ExPolicy const& policy, Comp comp)
    {
        hpx::partitioned_vector<int> source1(input1.size(), dist1);
        hpx::partitioned_vector<int> source2(input2.size(), dist2);
        hpx::partitioned_vector<int> destination(
            input1.size() + input2.size(), dist3);

        assign_values(source1, input1);
        assign_values(source2, input2);

        std::vector<int> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), comp);

        auto future = hpx::merge(policy, source1.begin(), source1.end(),
            source2.begin(), source2.end(), destination.begin(), comp);

        HPX_TEST(future.get() == destination.end());
        HPX_TEST(copy_values(destination) == expected);
    }

    template <typename DistPolicy1, typename DistPolicy2, typename DistPolicy3>
    void test_no_policy(std::vector<int> const& input1,
        std::vector<int> const& input2, DistPolicy1 const& dist1,
        DistPolicy2 const& dist2, DistPolicy3 const& dist3)
    {
        hpx::partitioned_vector<int> source1(input1.size(), dist1);
        hpx::partitioned_vector<int> source2(input2.size(), dist2);
        hpx::partitioned_vector<int> destination(
            input1.size() + input2.size(), dist3);

        assign_values(source1, input1);
        assign_values(source2, input2);

        std::vector<int> expected(input1.size() + input2.size());
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin());

        auto result = hpx::merge(source1.begin(), source1.end(),
            source2.begin(), source2.end(), destination.begin());

        HPX_TEST(result == destination.end());
        HPX_TEST(copy_values(destination) == expected);
    }

    template <typename DistPolicy1, typename DistPolicy2, typename DistPolicy3,
        typename ExPolicy>
    void test_ranges_merge_policy(DistPolicy1 const& dist1,
        DistPolicy2 const& dist2, DistPolicy3 const& dist3,
        ExPolicy const& policy)
    {
        // Each input is sorted after applying divide_projection{10}.
        std::vector<int> const input1{10, 11, 30, 31, 50};
        std::vector<int> const input2{12, 20, 21, 40, 41};
        divide_projection projection{10};

        hpx::partitioned_vector<int> source1(input1.size(), dist1);
        hpx::partitioned_vector<int> source2(input2.size(), dist2);
        hpx::partitioned_vector<int> destination(
            input1.size() + input2.size(), dist3);

        assign_values(source1, input1);
        assign_values(source2, input2);

        std::vector<int> expected(input1.size() + input2.size());
        auto projected_less = [projection](int lhs, int rhs) {
            return projection(lhs) < projection(rhs);
        };
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), projected_less);

        auto result = hpx::ranges::merge(policy, source1.begin(), source1.end(),
            source2.begin(), source2.end(), destination.begin(),
            std::less<int>{}, projection, projection);

        HPX_TEST(result.in1 == source1.end());
        HPX_TEST(result.in2 == source2.end());
        HPX_TEST(result.out == destination.end());
        HPX_TEST(copy_values(destination) == expected);
    }

    template <typename DistPolicy1, typename DistPolicy2, typename DistPolicy3,
        typename ExPolicy>
    void test_ranges_merge_task_policy(DistPolicy1 const& dist1,
        DistPolicy2 const& dist2, DistPolicy3 const& dist3,
        ExPolicy const& policy)
    {
        std::vector<int> const input1{10, 11, 30, 31, 50};
        std::vector<int> const input2{12, 20, 21, 40, 41};
        divide_projection projection{10};

        hpx::partitioned_vector<int> source1(input1.size(), dist1);
        hpx::partitioned_vector<int> source2(input2.size(), dist2);
        hpx::partitioned_vector<int> destination(
            input1.size() + input2.size(), dist3);

        assign_values(source1, input1);
        assign_values(source2, input2);

        std::vector<int> expected(input1.size() + input2.size());
        auto projected_less = [projection](int lhs, int rhs) {
            return projection(lhs) < projection(rhs);
        };
        std::merge(input1.begin(), input1.end(), input2.begin(), input2.end(),
            expected.begin(), projected_less);

        auto future = hpx::ranges::merge(policy, source1.begin(), source1.end(),
            source2.begin(), source2.end(), destination.begin(),
            std::less<int>{}, projection, projection);
        auto result = future.get();

        HPX_TEST(result.in1 == source1.end());
        HPX_TEST(result.in2 == source2.end());
        HPX_TEST(result.out == destination.end());
        HPX_TEST(copy_values(destination) == expected);
    }

    template <typename DistPolicy1, typename DistPolicy2, typename DistPolicy3>
    void test_subranges(DistPolicy1 const& dist1, DistPolicy2 const& dist2,
        DistPolicy3 const& dist3)
    {
        std::vector<int> const input1{-99, 1, 3, 5, 7, 99};
        std::vector<int> const input2{-88, 2, 4, 6, 8, 88};
        std::vector<int> const expected{-1, 1, 2, 3, 4, 5, 6, 7, 8, -1};

        hpx::partitioned_vector<int> source1(input1.size(), dist1);
        hpx::partitioned_vector<int> source2(input2.size(), dist2);
        hpx::partitioned_vector<int> destination(expected.size(), dist3);

        assign_values(source1, input1);
        assign_values(source2, input2);
        assign_values(destination, std::vector<int>(expected.size(), -1));

        auto result =
            hpx::merge(hpx::execution::par, advanced(source1.begin(), 1),
                advanced(source1.end(), -1), advanced(source2.begin(), 1),
                advanced(source2.end(), -1), advanced(destination.begin(), 1));

        HPX_TEST(result == advanced(destination.end(), -1));
        HPX_TEST(copy_values(destination) == expected);
    }

    void test_empty_ranges()
    {
        using hpx::execution::par;
        using hpx::execution::seq;
        using hpx::execution::task;

        test_no_policy({}, {}, hpx::container_layout, hpx::container_layout,
            hpx::container_layout);
        test_merge_policy({}, {}, hpx::container_layout, hpx::container_layout,
            hpx::container_layout, seq, std::less<int>{});
        test_merge_policy({}, {1, 2, 3}, hpx::container_layout,
            hpx::container_layout, hpx::container_layout, par,
            std::less<int>{});
        test_merge_policy({1, 2, 3}, {}, hpx::container_layout,
            hpx::container_layout, hpx::container_layout, seq,
            std::less<int>{});
        test_merge_policy({1}, {1}, hpx::container_layout,
            hpx::container_layout, hpx::container_layout, par,
            std::less<int>{});
        test_merge_task_policy({}, {1, 2, 3}, hpx::container_layout,
            hpx::container_layout, hpx::container_layout, seq(task),
            std::less<int>{});
        test_merge_task_policy({1, 2, 3}, {}, hpx::container_layout,
            hpx::container_layout, hpx::container_layout, par(task),
            std::less<int>{});
    }

    void test_distributed_ranges()
    {
        using hpx::execution::par;
        using hpx::execution::seq;
        using hpx::execution::task;

        std::vector<hpx::id_type> localities = hpx::find_all_localities();
        auto const dist1 = hpx::container_layout(2, localities);
        auto const dist2 = hpx::container_layout(3, localities);
        auto const dist3 = hpx::container_layout(5, localities);

        std::vector<int> const uneven1{-10, -2, 0, 3, 3, 9, 20};
        std::vector<int> const uneven2{-9, -2, 1, 3, 4, 21, 30, 31};

        test_no_policy(uneven1, uneven2, dist1, dist2, dist3);
        test_merge_policy(
            uneven1, uneven2, dist1, dist2, dist3, seq, std::less<int>{});
        test_merge_policy(
            uneven1, uneven2, dist1, dist2, dist3, par, std::less<int>{});
        test_merge_task_policy(
            uneven1, uneven2, dist1, dist2, dist3, seq(task), std::less<int>{});
        test_merge_task_policy(
            uneven1, uneven2, dist1, dist2, dist3, par(task), std::less<int>{});

        std::vector<int> const descending1{30, 20, 10, 0, -10};
        std::vector<int> const descending2{25, 15, 5, -5, -15};
        test_merge_policy(descending1, descending2, dist1, dist2, dist3, par,
            std::greater<int>{});

        // Values within one bucket are equivalent. Comparing against
        // std::merge verifies stable ordering within and between both inputs.
        std::vector<int> const stable1{10, 11, 12, 30, 31, 50};
        std::vector<int> const stable2{13, 14, 20, 21, 32, 33};
        test_merge_policy(
            stable1, stable2, dist1, dist2, dist3, par, bucket_less{10});
        test_merge_task_policy(
            stable1, stable2, dist1, dist2, dist3, par(task), bucket_less{10});

        test_ranges_merge_policy(dist1, dist2, dist3, seq);
        test_ranges_merge_policy(dist1, dist2, dist3, par);
        test_ranges_merge_task_policy(dist1, dist2, dist3, seq(task));
        test_ranges_merge_task_policy(dist1, dist2, dist3, par(task));
        test_subranges(dist1, dist2, dist3);

        {
            std::vector<int> const input1{1, 3, 5};
            std::vector<int> const input2{2, 4, 6};
            std::vector<int> expected(6);
            std::merge(input1.begin(), input1.end(), input2.begin(),
                input2.end(), expected.begin());

            hpx::partitioned_vector<int> source1(input1.size(), dist1);
            hpx::partitioned_vector<int> source2(input2.size(), dist2);
            hpx::partitioned_vector<int> destination(expected.size(), dist3);
            assign_values(source1, input1);
            assign_values(source2, input2);

            auto result = hpx::ranges::merge(source1.begin(), source1.end(),
                source2.begin(), source2.end(), destination.begin());
            HPX_TEST(result.in1 == source1.end());
            HPX_TEST(result.in2 == source2.end());
            HPX_TEST(result.out == destination.end());
            HPX_TEST(copy_values(destination) == expected);
        }

        {
            std::vector<int> const input1{1, 3, 5, 7};
            std::vector<double> const input2{2.0, 4.0, 6.0};
            std::vector<double> const expected{
                1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};

            hpx::partitioned_vector<int> source1(input1.size(), dist1);
            hpx::partitioned_vector<double> source2(input2.size(), dist2);
            hpx::partitioned_vector<double> destination(expected.size(), dist3);
            assign_values(source1, input1);
            assign_values(source2, input2);

            auto result = hpx::merge(par, source1.begin(), source1.end(),
                source2.begin(), source2.end(), destination.begin());
            HPX_TEST(result == destination.end());
            HPX_TEST(copy_values(destination) == expected);
        }
    }
}    // namespace

int main()
{
    test_empty_ranges();
    test_distributed_ranges();

    return hpx::util::report_errors();
}
#endif
