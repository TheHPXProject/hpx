//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <atomic>
#include <forward_list>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else
            return HPX_FORWARD(T, result);
    }

    struct record
    {
        int number;
    };

    using iterator = std::vector<record>::iterator;
    struct sentinel
    {
        iterator end;
        friend bool operator==(iterator it, sentinel s)
        {
            return it == s.end;
        }
    };
    using sized_range = std::ranges::subrange<iterator, sentinel,
        std::ranges::subrange_kind::sized>;

    template <typename Algorithm, typename Policy, typename... Args>
    void check_constraints(Algorithm, Policy, Args...)
    {
        static_assert(std::is_invocable_v<Algorithm, Policy, sized_range,
            Args..., decltype(&record::number)>);
        static_assert(!std::is_invocable_v<Algorithm, Policy,
            std::forward_list<record>&, Args..., decltype(&record::number)>);
        static_assert(!std::is_invocable_v<Algorithm, Policy, iterator,
            sentinel, Args..., decltype(&record::number)>);
    }

    template <typename Policy>
    void test(Policy policy)
    {
        using namespace hpx::ranges;
        std::vector<record> input{{1}, {2}, {2}, {3}, {2}};
        auto rng = sized_range(input.begin(), {input.end()}, input.size());
        auto proj = &record::number;
        auto two = [](int n) { return n == 2; };
        auto less = std::ranges::less{};
        auto equal = std::ranges::equal_to{};
        check_constraints(find, policy, 2);
        check_constraints(find_if, policy, two);
        check_constraints(find_if_not, policy, two);
        check_constraints(find_last, policy, 2);
        check_constraints(find_last_if, policy, two);
        check_constraints(find_last_if_not, policy, two);
        check_constraints(adjacent_find, policy, equal);
        check_constraints(is_sorted, policy, less);
        check_constraints(is_sorted_until, policy, less);
        check_constraints(is_heap, policy, less);
        check_constraints(is_heap_until, policy, less);
        check_constraints(min_element, policy, less);
        check_constraints(max_element, policy, less);
        check_constraints(minmax_element, policy, less);

        HPX_TEST(value(find(policy, rng, {2}, proj)) == input.begin() + 1);
        HPX_TEST(value(find_if(policy, rng, two, proj)) == input.begin() + 1);
        HPX_TEST(value(find_if_not(policy, rng, two, proj)) == input.begin());
        HPX_TEST(value(adjacent_find(policy, rng, equal, proj)) ==
            input.begin() + 1);
        auto last = value(find_last(policy, rng, {2}, proj));
        static_assert(
            std::is_same_v<decltype(last), std::ranges::subrange<iterator>>);
        HPX_TEST(last.begin() == input.begin() + 4);
        HPX_TEST(last.end() == input.end());
        HPX_TEST(value(find_last_if(policy, rng, two, proj)).begin() ==
            input.begin() + 4);
        HPX_TEST(value(find_last_if_not(policy, rng, two, proj)).begin() ==
            input.begin() + 3);
        HPX_TEST(!value(is_sorted(policy, rng, less, proj)));
        HPX_TEST(value(is_sorted_until(policy, rng, less, proj)) ==
            input.begin() + 4);
        HPX_TEST(!value(is_heap(policy, rng, less, proj)));
        HPX_TEST(
            value(is_heap_until(policy, rng, less, proj)) == input.begin() + 1);
        HPX_TEST(value(min_element(policy, rng, less, proj)) == input.begin());
        HPX_TEST(
            value(max_element(policy, rng, less, proj)) == input.begin() + 3);
        auto extrema = value(minmax_element(policy, rng, less, proj));
        HPX_TEST(extrema.min == input.begin());
        HPX_TEST(extrema.max == input.begin() + 3);
        HPX_TEST_EQ(value(hpx::ranges::min(policy, rng, less, proj)).number, 1);
        HPX_TEST_EQ(value(hpx::ranges::max(policy, rng, less, proj)).number, 3);
        auto values = value(hpx::ranges::minmax(policy, rng, less, proj));
        HPX_TEST_EQ(values.min.number, 1);
        HPX_TEST_EQ(values.max.number, 3);
        HPX_TEST_EQ(
            value(hpx::ranges::min(policy, std::vector<int>{9, 4, 7})), 4);
        auto increment = [](int& n) { ++n; };
        check_constraints(for_each, policy, increment);
        HPX_TEST(value(for_each(policy, rng, increment, proj)) == input.end());
        HPX_TEST_EQ(input[0].number, 2);
        HPX_TEST_EQ(input[4].number, 3);

        std::array needle{3, 3};
        auto match = value(find_end(policy, rng, needle, equal, proj));
        HPX_TEST(match.begin() == input.begin() + 1);
        HPX_TEST(match.end() == input.begin() + 3);
        HPX_TEST(value(find_first_of(policy, rng, needle, equal, proj)) ==
            input.begin() + 1);
        auto different = value(mismatch(policy, rng, needle, equal, proj));
        HPX_TEST(different.in1 == input.begin());
        HPX_TEST(different.in2 == needle.begin());

        auto dangling = value(find(policy, std::vector<int>{1, 2}, 2));
        static_assert(
            std::is_same_v<decltype(dangling), std::ranges::dangling>);
        auto dangling_pair =
            value(minmax_element(policy, std::vector<int>{1, 2}));
        static_assert(
            std::is_same_v<decltype(dangling_pair.min), std::ranges::dangling>);
        auto dangling_match =
            value(mismatch(policy, std::vector<int>{3, 4}, needle));
        static_assert(std::is_same_v<decltype(dangling_match.in1),
            std::ranges::dangling>);
        HPX_TEST(dangling_match.in2 == needle.begin() + 1);
        auto dangling_subrange =
            value(find_last(policy, std::vector<int>{1, 2}, 2));
        static_assert(
            std::is_same_v<decltype(dangling_subrange), std::ranges::dangling>);
    }
    void test_iterator_range()
    {
        std::vector<record> input{{1}, {2}, {3}};
        hpx::util::iterator_range common(input);
        static_assert(std::ranges::borrowed_range<decltype(common)>);
        static_assert(std::ranges::sized_range<decltype(common)>);
        auto noncommon =
            std::ranges::subrange(input.begin(), sentinel{input.end()});
        hpx::util::iterator_range view(noncommon);
        static_assert(std::is_same_v<decltype(view.end()), sentinel>);
        static_assert(!std::ranges::sized_range<decltype(view)>);
        HPX_TEST_EQ(view.size(), 3);
        auto result = hpx::ranges::find(hpx::execution::par,
            hpx::util::iterator_range(input), 2, &record::number);
        HPX_TEST(result == input.begin() + 1);
    }

    struct throwing_copy
    {
        int number;
        explicit throwing_copy(int n)
          : number(n)
        {
        }
        throwing_copy(throwing_copy const&)
        {
            throw std::runtime_error("copy");
        }
        throwing_copy(throwing_copy&&) = default;
        throwing_copy& operator=(throwing_copy const&) = default;
        throwing_copy& operator=(throwing_copy&&) = default;
    };

    template <typename Policy>
    void test_value_exceptions(Policy policy)
    {
        std::vector<throwing_copy> input;
        input.emplace_back(2);
        input.emplace_back(1);
        auto check = [&](auto algorithm) {
            bool caught = false;
            try
            {
                value(algorithm(policy, input, std::ranges::less{},
                    &throwing_copy::number));
            }
            catch (hpx::exception_list const&)
            {
                caught = true;
            }
            HPX_TEST(caught);
        };
        check(hpx::ranges::min);
        check(hpx::ranges::max);
        check(hpx::ranges::minmax);
    }

    void test_extrema_requirements()
    {
        std::vector<std::unique_ptr<int>> movable;
        movable.push_back(std::make_unique<int>(3));
        movable.push_back(std::make_unique<int>(1));
        movable.push_back(std::make_unique<int>(3));
        auto projection = [](auto const& element) { return *element; };
        auto result = hpx::ranges::minmax_element(
            hpx::execution::par, movable, std::ranges::less{}, projection);
        HPX_TEST(result.min == movable.begin() + 1);
        HPX_TEST(result.max == movable.begin() + 2);

        std::vector<int> input{3, 1, 5, 1, 2, 5, 4, 3, 1};
        std::atomic<std::size_t> comparisons{0}, projections{0};
        auto comp = [&](int a, int b) {
            ++comparisons;
            return a < b;
        };
        auto proj = [&](int n) {
            ++projections;
            return n;
        };
        auto policy = hpx::execution::par.with(
            hpx::execution::experimental::static_chunk_size(1));
        HPX_TEST(hpx::ranges::min_element(policy, input, comp, proj) ==
            input.begin() + 1);
        HPX_TEST_EQ(comparisons.load(), input.size() - 1);
        HPX_TEST_EQ(projections.load(), 2 * comparisons.load());
        comparisons = 0;
        projections = 0;
        auto both = hpx::ranges::minmax_element(policy, input, comp, proj);
        HPX_TEST(both.min == input.begin() + 1);
        HPX_TEST(both.max == input.begin() + 5);
        HPX_TEST_LTE(comparisons.load(), 3 * (input.size() - 1) / 2);
        HPX_TEST_EQ(projections.load(), 2 * comparisons.load());
    }

}    // namespace

int hpx_main()
{
    using namespace hpx::execution;
    test(seq);
    test(par);
    test(par_unseq);
    test(seq(task));
    test(par(task));
    test_extrema_requirements();
    test_iterator_range();
    test_value_exceptions(seq);
    test_value_exceptions(par);
    test_value_exceptions(seq(task));
    test_value_exceptions(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
