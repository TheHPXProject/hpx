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

// The worker tasks of the algorithms that compare an element with more than
// one other element. The only worker runs all chunks in one task: a chunk of
// 100 iterations of a nano comparison is small and 100 small chunks make a
// medium task.
//
// The sizes are counted when the scheduler creates a thread (see
// task_size_checks.hpp). The task policy makes the algorithms asynchronous,
// they return a future: with a synchronous policy, the executor would run the
// queue of the calling thread inline instead of creating a thread for it, and
// with a single worker that is the only queue, so no thread would be counted.
void test_multi_call_sizes()
{
    auto const by_100 =
        hpx::execution::par(hpx::execution::task)
            .with(hpx::execution::experimental::static_chunk_size(100));

    std::vector<int> v(10000, 1);
    std::vector<int> const needle(3, 2);

    // min_element, max_element, minmax_element compare two elements
    check_only_size([&] { hpx::min_element(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size([&] { hpx::max_element(by_100, v.begin(), v.end()).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::minmax_element(by_100, v.begin(), v.end()).get(); },
        task_size::medium);

    // lexicographical_compare compares two elements in both orders
    check_only_size(
        [&] {
            hpx::lexicographical_compare(
                by_100, v.begin(), v.end(), v.begin(), v.end())
                .get();
        },
        task_size::medium);

    // find_end and search: most positions don't match the first element of
    // the sequence
    check_only_size(
        [&] {
            hpx::find_end(
                by_100, v.begin(), v.end(), needle.begin(), needle.end())
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::search(
                by_100, v.begin(), v.end(), needle.begin(), needle.end())
                .get();
        },
        task_size::medium);

    // find_first_of compares an element with each element of the second
    // range, search_n with count elements: with 100 of them an iteration is
    // small, which makes the task large
    std::vector<int> const one(1, 2);
    std::vector<int> const hundred(100, 2);
    check_only_size(
        [&] {
            hpx::find_first_of(
                by_100, v.begin(), v.end(), one.begin(), one.end())
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::find_first_of(
                by_100, v.begin(), v.end(), hundred.begin(), hundred.end())
                .get();
        },
        task_size::large);
    check_only_size(
        [&] { hpx::search_n(by_100, v.begin(), v.end(), 1, 2).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::search_n(by_100, v.begin(), v.end(), 100, 2).get(); },
        task_size::large);

    // a size given to the comparison wins, one without a size has none
    auto const less = [](int a, int b) { return a < b; };
    check_only_size(
        [&] {
            hpx::min_element(by_100, v.begin(), v.end(), hpx::nano_task(less))
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] { hpx::min_element(by_100, v.begin(), v.end(), less).get(); },
        task_size::unknown);

    // a projection hides the type the comparison is called with
    auto const identity = [](int i) { return i; };
    check_only_size(
        [&] {
            hpx::ranges::min_element(by_100, v, std::less<>(), identity).get();
        },
        task_size::unknown);

    // comparisons of other types have no size
    std::vector<std::string> s(10000, "1");
    check_only_size([&] { hpx::min_element(by_100, s.begin(), s.end()).get(); },
        task_size::unknown);
}

int hpx_main()
{
    test_multi_call_sizes();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return task_size_checks::run(hpx_main, argc, argv);
}
