//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "task_size_checks.hpp"

using hpx::threads::task_size;
using task_size_checks::check_only_size;

// The algorithms built on the scan partitioner run each chunk of their first
// and third steps as a task of its own, which has the size of the iterations
// of its chunk: 100 iterations of a nano operation make a small task. The
// third step of copy_if, partition_copy and unique_copy only copies elements,
// its tasks have no size.
//
// Without an initial value, the scans and unique_copy start at the second
// element: they get one more element, so that all of their chunks have 100
// iterations.
//
// The scan partitioner creates a thread for every chunk, with any policy. The
// task policy is not needed here, it is used as in the other task size tests.
void test_scan_sizes()
{
    auto const by_100 =
        hpx::execution::par(hpx::execution::task)
            .with(hpx::execution::experimental::static_chunk_size(100));

    std::vector<int> v(10000, 1);
    std::vector<int> v1(10001, 1);
    std::vector<int> out(10001);
    std::vector<int> out2(10000);
    auto const plus = std::plus<>();
    auto const negate = std::negate<>();
    auto const is_zero = std::logical_not<>();

    // the scans call op with the result so far and an element
    check_only_size(
        [&] {
            hpx::inclusive_scan(by_100, v1.begin(), v1.end(), out.begin(), plus)
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::inclusive_scan(
                by_100, v.begin(), v.end(), out.begin(), plus, 0)
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::exclusive_scan(
                by_100, v.begin(), v.end(), out.begin(), 0, plus)
                .get();
        },
        task_size::small_);

    // the transform scans call the transformation in the first step and op
    // in the third
    check_only_size(
        [&] {
            hpx::transform_inclusive_scan(
                by_100, v1.begin(), v1.end(), out.begin(), plus, negate)
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::transform_exclusive_scan(
                by_100, v.begin(), v.end(), out.begin(), 0, plus, negate)
                .get();
        },
        task_size::small_);

    // copy_if and partition_copy call the predicate, unique_copy compares
    // two elements
    check_only_size(
        [&] {
            hpx::copy_if(by_100, v.begin(), v.end(), out.begin(), is_zero)
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::partition_copy(
                by_100, v.begin(), v.end(), out.begin(), out2.begin(), is_zero)
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::unique_copy(by_100, v1.begin(), v1.end(), out.begin()).get();
        },
        task_size::small_);

    // a size given to the operation wins, one without a size has none
    auto const add = [](int a, int b) { return a + b; };
    check_only_size(
        [&] {
            hpx::inclusive_scan(
                by_100, v1.begin(), v1.end(), out.begin(), hpx::nano_task(add))
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::inclusive_scan(by_100, v.begin(), v.end(), out.begin(), add)
                .get();
        },
        task_size::unknown);

    // an unsized transformation leaves the first step without a size, the
    // third step still has the size of op
    check_only_size(
        [&] {
            hpx::transform_inclusive_scan(by_100, v1.begin(), v1.end(),
                out.begin(), plus, [](int i) { return -i; })
                .get();
        },
        task_size::small_);

    // operations on other types have no size
    std::vector<std::string> s(10000, "1");
    std::vector<std::string> s_out(10000);
    check_only_size(
        [&] {
            hpx::inclusive_scan(by_100, s.begin(), s.end(), s_out.begin(), plus)
                .get();
        },
        task_size::unknown);
}

int hpx_main()
{
    test_scan_sizes();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return task_size_checks::run(hpx_main, argc, argv);
}
