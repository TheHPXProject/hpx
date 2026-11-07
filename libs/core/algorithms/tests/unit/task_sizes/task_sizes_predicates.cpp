//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "task_size_checks.hpp"

using hpx::threads::task_size;
using task_size_checks::check_only_size;

// The worker tasks of the algorithms that call a predicate about once per
// element have the size of the predicate called with an element. The only
// worker runs all chunks in one task: a chunk of 100 iterations of a nano
// predicate is small and 100 small chunks make a medium task.
//
// The sizes are counted when the scheduler creates a thread (see
// task_size_checks.hpp). The task policy makes the algorithms asynchronous,
// they return a future: with a synchronous policy, the executor would run the
// queue of the calling thread inline instead of creating a thread for it, and
// with a single worker that is the only queue, so no thread would be counted.
void test_predicate_sizes()
{
    auto const by_100 =
        hpx::execution::par(hpx::execution::task)
            .with(hpx::execution::experimental::static_chunk_size(100));

    std::vector<int> v(10000, 1);
    std::vector<int> out(10000);
    auto const is_zero = std::logical_not<>();

    // all_of, any_of, none_of
    check_only_size(
        [&] { hpx::all_of(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::any_of(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::none_of(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);

    // count compares with equal_to, count_if calls the predicate
    check_only_size([&] { hpx::count(by_100, v.begin(), v.end(), 2).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::count_if(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);

    // the find family
    check_only_size([&] { hpx::find(by_100, v.begin(), v.end(), 2).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::find_if(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::find_if_not(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::find_last(by_100, v.begin(), v.end(), 2).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::find_last_if(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);

    // equal and mismatch, with and without the end of the second range
    check_only_size(
        [&] { hpx::equal(by_100, v.begin(), v.end(), v.begin()).get(); },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::equal(by_100, v.begin(), v.end(), v.begin(), v.end()).get();
        },
        task_size::medium);
    check_only_size(
        [&] { hpx::mismatch(by_100, v.begin(), v.end(), v.begin()).get(); },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::mismatch(by_100, v.begin(), v.end(), v.begin(), v.end()).get();
        },
        task_size::medium);

    // adjacent_find, adjacent_difference
    check_only_size(
        [&] { hpx::adjacent_find(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::adjacent_difference(by_100, v.begin(), v.end(), out.begin())
                .get();
        },
        task_size::medium);

    // is_sorted, is_heap, is_partitioned
    check_only_size([&] { hpx::is_sorted(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::is_sorted_until(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size([&] { hpx::is_heap(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::is_heap_until(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::is_partitioned(by_100, v.begin(), v.end(), is_zero).get(); },
        task_size::medium);

    // remove compares with equal_to, remove_if calls the predicate
    check_only_size(
        [&] {
            out = v;
            hpx::remove(by_100, out.begin(), out.end(), 2).get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            out = v;
            hpx::remove_if(by_100, out.begin(), out.end(), is_zero).get();
        },
        task_size::medium);

    // replace compares with equal_to, replace_if calls the predicate
    check_only_size(
        [&] {
            out = v;
            hpx::replace(by_100, out.begin(), out.end(), 2, 3).get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            out = v;
            hpx::replace_if(by_100, out.begin(), out.end(), is_zero, 3).get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::replace_copy(by_100, v.begin(), v.end(), out.begin(), 2, 3)
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::replace_copy_if(
                by_100, v.begin(), v.end(), out.begin(), is_zero, 3)
                .get();
        },
        task_size::medium);

    // a generator has a size only if it is given one
    auto const one = [] { return 1; };
    check_only_size(
        [&] {
            hpx::generate(by_100, out.begin(), out.end(), hpx::nano_task(one))
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::generate_n(by_100, out.begin(), 10000, hpx::nano_task(one))
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] { hpx::generate(by_100, out.begin(), out.end(), one).get(); },
        task_size::unknown);

    // a size given to the predicate wins, a predicate without one has none
    auto const is_two = [](int i) { return i == 2; };
    check_only_size(
        [&] {
            hpx::count_if(by_100, v.begin(), v.end(), hpx::nano_task(is_two))
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] { hpx::count_if(by_100, v.begin(), v.end(), is_two).get(); },
        task_size::unknown);

    // a projection hides the type the predicate is called with
    auto const identity = [](int i) { return i; };
    check_only_size(
        [&] { hpx::ranges::count_if(by_100, v, is_zero, identity).get(); },
        task_size::unknown);
    check_only_size([&] { hpx::ranges::find(by_100, v, 2, identity).get(); },
        task_size::unknown);

    // comparisons of other types have no size
    std::vector<std::string> s(10000, "1");
    check_only_size(
        [&] { hpx::count(by_100, s.begin(), s.end(), std::string("2")).get(); },
        task_size::unknown);
}

// A layout mapping of two extents, enough for for_each_index.
struct extents_2d
{
    std::size_t dims[2];

    [[nodiscard]] constexpr std::size_t extent(std::size_t r) const noexcept
    {
        return dims[r];
    }
};

struct mapping_2d
{
    using index_type = int;
    using extents_type = extents_2d;

    extents_2d ext;

    [[nodiscard]] static constexpr std::size_t rank() noexcept
    {
        return 2;
    }

    [[nodiscard]] constexpr extents_type const& extents() const noexcept
    {
        return ext;
    }
};

// An iteration of for_each_index calls the function for each index of the
// other extents: an iteration of 100 nano calls is small, and a chunk of 100
// iterations is medium.
void test_for_each_index_sizes()
{
    auto const by_100 =
        hpx::execution::par(hpx::execution::task)
            .with(hpx::execution::experimental::static_chunk_size(100));

    mapping_2d const m{{{100, 100}}};
    auto const f = [](int, int) {};

    check_only_size(
        [&] {
            hpx::experimental::for_each_index(by_100, m, hpx::nano_task(f))
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] { hpx::experimental::for_each_index(by_100, m, f).get(); },
        task_size::unknown);
}

int hpx_main()
{
    test_predicate_sizes();
    test_for_each_index_sizes();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return task_size_checks::run(hpx_main, argc, argv);
}
