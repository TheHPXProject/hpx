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

#include <complex>
#include <functional>
#include <vector>

#include "task_size_checks.hpp"

using hpx::threads::task_size;
using task_size_checks::check_only_size;

// The worker tasks of reduce, transform_reduce and merge have the size of the
// reduction, the transformation and the comparison. The task policy keeps the
// partitioners from running the only chunk inline.
void test_algorithm_sizes()
{
    auto const policy = hpx::execution::par(hpx::execution::task);
    std::vector<int> v(16, 1);
    std::vector<int> merged(32);

    auto const plus = [](int a, int b) { return a + b; };
    auto const identity = [](int i) { return i; };
    auto const less = [](int a, int b) { return a < b; };

    check_only_size(
        [&] {
            hpx::reduce(policy, v.begin(), v.end(), 0, hpx::large_task(plus))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::transform_reduce(
                policy, v.begin(), v.end(), 0, plus, hpx::large_task(identity))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::transform_reduce(policy, v.begin(), v.end(), v.begin(), 0,
                plus, hpx::large_task(plus))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::merge(policy, v.begin(), v.end(), v.begin(), v.end(),
                merged.begin(), hpx::large_task(less))
                .get();
        },
        task_size::large);

    // with variable chunk sizes the size is derived from the iterations of a
    // chunk as well
    check_only_size(
        [&] {
            hpx::reduce(
                policy.with(hpx::execution::experimental::guided_chunk_size()),
                v.begin(), v.end(), 0, hpx::medium_task(plus))
                .get();
        },
        task_size::medium);

    // The only worker runs all chunks in one task. A chunk of 100 iterations
    // of a nano function is small and 100 small chunks make a medium task.
    // Fewer iterations per chunk would make nano chunks and a small task;
    // counting all 10000 iterations for each chunk would make medium chunks
    // and a large task.
    std::vector<int> w(10000, 1);
    check_only_size(
        [&] {
            hpx::reduce(
                policy.with(
                    hpx::execution::experimental::static_chunk_size(100)),
                w.begin(), w.end(), 0, hpx::nano_task(plus))
                .get();
        },
        task_size::medium);

    // without a size, the worker tasks have none
    check_only_size(
        [&] { hpx::reduce(policy, v.begin(), v.end(), 0, plus).get(); },
        task_size::unknown);
    check_only_size(
        [&] {
            hpx::merge(policy, v.begin(), v.end(), v.begin(), v.end(),
                merged.begin(), less)
                .get();
        },
        task_size::unknown);

    // Builtin operations on builtin types are nano without being given a
    // size: as for the nano reduction above, 100 chunks of 100 iterations
    // make a medium task.
    auto const by_100 =
        policy.with(hpx::execution::experimental::static_chunk_size(100));
    check_only_size(
        [&] {
            hpx::reduce(by_100, w.begin(), w.end(), 0, std::plus<>()).get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::transform_reduce(by_100, w.begin(), w.end(), w.begin(), 0,
                std::plus<>(), std::multiplies<>())
                .get();
        },
        task_size::medium);

    // the 16 iterations of a merge with a builtin comparison, given or not,
    // make a nano task
    check_only_size(
        [&] {
            hpx::merge(policy, v.begin(), v.end(), v.begin(), v.end(),
                merged.begin(), std::less{})
                .get();
        },
        task_size::nano);
    check_only_size(
        [&] {
            hpx::merge(
                policy, v.begin(), v.end(), v.begin(), v.end(), merged.begin())
                .get();
        },
        task_size::nano);

    // a projection hides the type the comparison is called with
    check_only_size(
        [&] {
            hpx::ranges::merge(
                policy, v, v, merged.begin(), std::less{}, identity, identity)
                .get();
        },
        task_size::unknown);

    // the transformations of transform are sized the same way
    std::vector<int> out(10000);
    check_only_size(
        [&] {
            hpx::transform(policy, v.begin(), v.end(), out.begin(),
                hpx::large_task(identity))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::transform(policy, v.begin(), v.end(), out.begin(), identity)
                .get();
        },
        task_size::unknown);
    check_only_size(
        [&] {
            hpx::transform(
                by_100, w.begin(), w.end(), out.begin(), std::negate<>())
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::transform(by_100, w.begin(), w.end(), w.begin(), out.begin(),
                std::plus<>())
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::ranges::transform(by_100, w, w, out.begin(), std::plus<>())
                .get();
        },
        task_size::medium);
    check_only_size(
        [&] {
            hpx::ranges::transform(
                policy, v, out.begin(), std::negate<>(), identity)
                .get();
        },
        task_size::unknown);

    // builtin operations on other types have no size
    std::vector<std::complex<double>> c(16, 1.0);
    check_only_size(
        [&] {
            hpx::reduce(policy, c.begin(), c.end(), std::complex<double>(),
                std::plus<>())
                .get();
        },
        task_size::unknown);
}

int hpx_main()
{
    test_algorithm_sizes();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return task_size_checks::run(hpx_main, argc, argv);
}
