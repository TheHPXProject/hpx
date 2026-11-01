//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/testing.hpp>
#if defined(HPX_HAVE_DATAPAR)
#include <hpx/datapar.hpp>
#endif

#include <cstddef>
#include <string>
#include <vector>

#include "task_size_checks.hpp"

using hpx::threads::task_size;
using task_size_checks::check_only_size;

// The only worker runs all chunks in one task (see task_sizes_predicates.cpp).
// With chunks of 100 elements, an operation of about a nanosecond per element
// gives small chunks and a medium worker task. An operation that handles
// several elements in a nanosecond gives nano chunks and a small worker task.
//
// The sizes are counted when the scheduler creates a thread (see
// task_size_checks.hpp). The task policy makes the algorithms asynchronous,
// they return a future: with a synchronous policy, the executor would run the
// queue of the calling thread inline instead of creating a thread for it, and
// with a single worker that is the only queue, so no thread would be counted.
auto const by_100 =
    hpx::execution::par(hpx::execution::task)
        .with(hpx::execution::experimental::static_chunk_size(100));

// Assigning, swapping and destroying builtin elements takes about a
// nanosecond each.
void test_builtin_elements()
{
    std::vector<int> v(10000, 1);
    std::vector<int> w(10000, 2);

    check_only_size([&] { hpx::fill(by_100, v.begin(), v.end(), 3).get(); },
        task_size::medium);
    check_only_size([&] { hpx::fill_n(by_100, v.begin(), 10000, 4).get(); },
        task_size::medium);
    check_only_size(
        [&] { hpx::swap_ranges(by_100, v.begin(), v.end(), w.begin()).get(); },
        task_size::medium);

    // reverse swaps half of the elements
    std::vector<int> r(20000, 1);
    check_only_size([&] { hpx::reverse(by_100, r.begin(), r.end()).get(); },
        task_size::medium);
}

// Trivially copyable elements are copied with memmove at 16 bytes in a
// nanosecond (the default of hpx.task_size_chunk_size.copy_bytes_per_ns), so
// a chunk of 100 ints takes 25 nano calls.
void test_copied_elements()
{
    std::vector<int> v(10000, 1);
    std::vector<int> out(10000);

    check_only_size(
        [&] { hpx::copy(by_100, v.begin(), v.end(), out.begin()).get(); },
        task_size::small_);
    check_only_size(
        [&] { hpx::copy_n(by_100, v.begin(), 10000, out.begin()).get(); },
        task_size::small_);
    check_only_size(
        [&] { hpx::move(by_100, v.begin(), v.end(), out.begin()).get(); },
        task_size::small_);
}

// The size of copying or assigning other elements is not known.
void test_other_elements()
{
    std::vector<std::string> v(10000, "1");
    std::vector<std::string> out(10000);

    check_only_size(
        [&] { hpx::copy(by_100, v.begin(), v.end(), out.begin()).get(); },
        task_size::unknown);
    check_only_size(
        [&] { hpx::move(by_100, v.begin(), v.end(), out.begin()).get(); },
        task_size::unknown);
    check_only_size([&] { hpx::fill(by_100, v.begin(), v.end(), "2").get(); },
        task_size::unknown);
}

// The unsequenced policies let the compiler vectorize a loop body of a
// nanosecond, which then handles HPX_LANE_SIZE bytes of elements in that
// time.
void test_unsequenced()
{
    auto const unseq_by_100 =
        hpx::execution::par_unseq(hpx::execution::task)
            .with(hpx::execution::experimental::static_chunk_size(100));

    std::vector<int> v(10000, 1);
    check_only_size(
        [&] { hpx::fill(unseq_by_100, v.begin(), v.end(), 3).get(); },
        HPX_LANE_SIZE / sizeof(int) > 1 ? task_size::small_ :
                                          task_size::medium);
}

#if defined(HPX_HAVE_DATAPAR)
// The datapar policies call the loop body with packs of elements.
void test_datapar()
{
    auto const simd_by_100 =
        hpx::execution::par_simd(hpx::execution::task)
            .with(hpx::execution::experimental::static_chunk_size(100));

    using pack_type = hpx::parallel::traits::vector_pack_type_t<int>;
    std::size_t const lanes =
        hpx::parallel::traits::vector_pack_size_v<pack_type>;

    std::vector<int> v(10000, 1);
    check_only_size(
        [&] { hpx::fill(simd_by_100, v.begin(), v.end(), 3).get(); },
        lanes > 1 ? task_size::small_ : task_size::medium);
}
#endif

int hpx_main()
{
    test_builtin_elements();
    test_copied_elements();
    test_other_elements();
    test_unsequenced();
#if defined(HPX_HAVE_DATAPAR)
    test_datapar();
#endif

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return task_size_checks::run(&hpx_main, argc, argv);
}
