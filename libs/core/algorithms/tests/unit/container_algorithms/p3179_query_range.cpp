//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <concepts>
#include <cstddef>
#include <forward_list>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    template <typename I>
    struct sentinel
    {
        I end;

        friend bool operator==(I it, sentinel s)
        {
            return it == s.end;
        }
    };

    template <typename I>
    struct sized_sentinel : sentinel<I>
    {
        friend std::iter_difference_t<I> operator-(sized_sentinel s, I it)
        {
            return s.end - it;
        }

        friend std::iter_difference_t<I> operator-(I it, sized_sentinel s)
        {
            return it - s.end;
        }
    };

    struct record
    {
        int value;
    };

    struct positive
    {
        bool operator()(int value) const
        {
            return value > 0;
        }
    };

    struct invalid_predicate
    {
        void operator()(int) const;
    };

    struct invalid_binary_predicate
    {
        void operator()(int, int) const;
    };

    using iterator = std::vector<record>::iterator;
    using sized_range = std::ranges::subrange<iterator, sentinel<iterator>,
        std::ranges::subrange_kind::sized>;
    using unsized_range = std::ranges::subrange<iterator, sentinel<iterator>>;
    using forward_range = std::forward_list<record>;
    using projection = decltype(&record::value);

    static_assert(std::ranges::random_access_range<sized_range>);
    static_assert(std::ranges::sized_range<sized_range>);
    static_assert(!std::sized_sentinel_for<sentinel<iterator>, iterator>);
    static_assert(std::sized_sentinel_for<sized_sentinel<iterator>, iterator>);
    static_assert(!std::ranges::sized_range<unsized_range>);

    template <typename Algorithm, typename Policy, typename... Args>
    void check_unary_constraints(Algorithm, Policy, Args...)
    {
        static_assert(std::is_invocable_v<Algorithm, Policy, sized_range,
            Args..., projection>);
        static_assert(std::is_invocable_v<Algorithm, Policy, iterator,
            sized_sentinel<iterator>, Args..., projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, forward_range&,
            Args..., projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, unsized_range,
            Args..., projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, iterator,
            sentinel<iterator>, Args..., projection>);
        static_assert(!std::is_invocable_v<Algorithm, int, sized_range, Args...,
            projection>);
    }

    template <typename Algorithm, typename Policy>
    void check_binary_constraints(Algorithm, Policy)
    {
        using pred = std::ranges::equal_to;
        static_assert(std::is_invocable_v<Algorithm, Policy, sized_range,
            sized_range, pred, projection, projection>);
        static_assert(std::is_invocable_v<Algorithm, Policy, iterator,
            sized_sentinel<iterator>, iterator, sized_sentinel<iterator>, pred,
            projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, forward_range&,
            sized_range, pred, projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, sized_range,
            forward_range&, pred, projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, unsized_range,
            sized_range, pred, projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, sized_range,
            unsized_range, pred, projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, int, sized_range,
            sized_range, pred, projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, sized_range,
            sized_range, invalid_binary_predicate, projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, iterator,
            sentinel<iterator>, iterator, sized_sentinel<iterator>, pred,
            projection, projection>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, iterator,
            sized_sentinel<iterator>, iterator, sentinel<iterator>, pred,
            projection, projection>);
    }

    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else
            return result;
    }

    template <typename Policy>
    void test_queries(Policy policy)
    {
        using namespace hpx::ranges;
        check_unary_constraints(all_of, policy, positive{});
        check_unary_constraints(any_of, policy, positive{});
        check_unary_constraints(none_of, policy, positive{});
        check_unary_constraints(count_if, policy, positive{});
        check_unary_constraints(is_partitioned, policy, positive{});
        check_unary_constraints(count, policy, 2);
        check_unary_constraints(contains, policy, 2);
        check_binary_constraints(equal, policy);
        check_binary_constraints(starts_with, policy);
        check_binary_constraints(ends_with, policy);
        check_binary_constraints(contains_subrange, policy);
        check_binary_constraints(lexicographical_compare, policy);

        static_assert(!std::is_invocable_v<decltype(all_of), Policy,
            sized_range, invalid_predicate, projection>);

        std::vector<record> data = {{1}, {2}, {2}, {-1}};
        sized_range r(data.begin(), sentinel{data.end()}, data.size());
        sized_range prefix(data.begin(), sentinel{data.begin() + 2}, 2);
        sized_range suffix(data.begin() + 2, sentinel{data.end()}, 2);
        sized_range empty(data.end(), sentinel{data.end()}, 0);
        auto const proj = &record::value;
        auto const eq = std::ranges::equal_to{};
        HPX_TEST(!value(all_of(policy, r, positive{}, proj)));
        HPX_TEST(value(any_of(policy, r, positive{}, proj)));
        HPX_TEST(!value(none_of(policy, r, positive{}, proj)));
        HPX_TEST_EQ(value(count(policy, r, 2, proj)), 2);
        HPX_TEST_EQ(value(count(policy, r, {2}, proj)), 2);
        HPX_TEST(value(contains(policy, r, {2}, proj)));
        HPX_TEST_EQ(value(count_if(policy, r, positive{}, proj)), 3);
        HPX_TEST(value(is_partitioned(policy, r, positive{}, proj)));
        HPX_TEST(value(contains(policy, r, 2, proj)));
        HPX_TEST(!value(contains(policy, r, 9, proj)));
        HPX_TEST(value(equal(policy, r, r, eq, proj, proj)));
        HPX_TEST(!value(equal(policy, r, prefix, eq, proj, proj)));
        HPX_TEST(value(starts_with(policy, r, prefix, eq, proj, proj)));
        HPX_TEST(value(ends_with(policy, r, suffix, eq, proj, proj)));
        HPX_TEST(value(contains_subrange(policy, r, suffix, eq, proj, proj)));
        HPX_TEST(
            value(contains_subrange(policy, empty, empty, eq, proj, proj)));
        HPX_TEST(value(lexicographical_compare(
            policy, prefix, r, std::ranges::less{}, proj, proj)));
        HPX_TEST(value(all_of(policy, empty, positive{}, proj)));
        HPX_TEST(!value(any_of(policy, empty, positive{}, proj)));
        HPX_TEST(value(none_of(policy, empty, positive{}, proj)));
        HPX_TEST_EQ(value(count(policy, empty, 2, proj)), 0);

        auto const last = sized_sentinel<iterator>{{data.end()}};
        HPX_TEST_EQ(value(count(policy, data.begin(), last, 2, proj)), 2);
        HPX_TEST(value(contains(policy, data.begin(), last, 2, proj)));
        HPX_TEST(value(contains(policy, data.begin(), last, {2}, proj)));
        HPX_TEST_EQ(value(count(policy, data.begin(), last, {2}, proj)), 2);
        HPX_TEST(value(equal(
            policy, data.begin(), last, data.begin(), last, eq, proj, proj)));
        HPX_TEST(value(contains_subrange(
            policy, data.begin(), last, data.begin(), last, eq, proj, proj)));
    }

    void test_serial()
    {
        std::forward_list<int> values = {1, 2, 3};
        HPX_TEST(hpx::ranges::all_of(values, positive{}));
        HPX_TEST(hpx::ranges::any_of(values, positive{}));
        HPX_TEST(!hpx::ranges::none_of(values, positive{}));
        HPX_TEST_EQ(hpx::ranges::count(values, 2), 1);
        HPX_TEST_EQ(hpx::ranges::count_if(values, positive{}), 3);
        HPX_TEST(hpx::ranges::contains(values, 2));
        HPX_TEST(hpx::ranges::is_partitioned(values, positive{}));
        HPX_TEST(hpx::ranges::equal(values, values));
        HPX_TEST(hpx::ranges::starts_with(values, values));
        HPX_TEST(hpx::ranges::ends_with(values, values));
        HPX_TEST(hpx::ranges::contains_subrange(values, values));
        HPX_TEST(!hpx::ranges::lexicographical_compare(values, values));
    }
}    // namespace

int hpx_main()
{
    using namespace hpx::execution;
    test_queries(seq);
    test_queries(par);
    test_queries(par_unseq);
    test_queries(seq(task));
    test_queries(par(task));
    test_serial();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
