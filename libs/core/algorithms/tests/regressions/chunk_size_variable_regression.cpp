//  Copyright (c) 2026 Omkar Tipugade
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression tests for two argument-order bugs fixed in chunk_size.hpp:
//
//  1. get_bulk_iteration_shape_variable:  next_or_subrange was called with
//     (it_or_r, count, chunk) instead of (it_or_r, chunk, count), causing the
//     iterator to advance by the *remaining* count rather than the consumed
//     chunk on every iteration after the first.
//
//  2. add_ready_future_idx (shared_future<void> overload):  the parameter list
//     had (base_idx, first, count) instead of (first, base_idx, count), so the
//     raw iterator value was misinterpreted as base_idx, corrupting every index
//     passed to the user functor.

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <numeric>
#include <vector>

// ---------------------------------------------------------------------------
// Test 1: multi-chunk traversal in get_bulk_iteration_shape_variable
//
// Uses guided_chunk_size (the only built-in executor parameters type that
// exposes has_variable_chunk_size), which routes through
// get_bulk_iteration_shape_variable.
//
// With the old (buggy) code the iterator stored in shape[i] would point to
// the wrong position for every chunk after the first, because `count`
// (remaining elements) was used as the advance offset instead of `chunk`
// (elements just consumed).
// ---------------------------------------------------------------------------
void test_variable_chunk_multi_chunk_traversal()
{
    // Build a small integer range and split it into multiple chunks.
    constexpr std::size_t total = 10;
    constexpr std::size_t min_chunk = 3;    // guided_chunk_size minimum

    // guided_chunk_size exposes has_variable_chunk_size = true_type, which
    // selects get_bulk_iteration_shape_variable inside partitioner::partition.
    // We call get_bulk_iteration_shape_variable directly so we can inspect the
    // returned shape without actually dispatching work.
    auto policy = hpx::execution::par.with(
        hpx::execution::experimental::guided_chunk_size(min_chunk));

    // Use integer iterators (simpler than a real container for this test).
    int it = 0;
    std::size_t count = total;
    std::size_t cores = 1;

    auto shape = hpx::parallel::util::detail::get_bulk_iteration_shape_variable(
        policy, it, count, cores);

    // There must be at least 2 chunks to exercise the multi-chunk path.
    HPX_TEST(shape.size() >= 2u);

    // Invariant 1: sum of chunk sizes must equal total.
    std::size_t sum = 0;
    for (auto const& s : shape)
    {
        sum += hpx::get<1>(s);
    }
    HPX_TEST_EQ(sum, total);

    // Invariant 2: each chunk's starting iterator must equal the cumulative
    // offset of all previous chunks.  With the old bug, hpx::get<0>(shape[i])
    // would be wrong for i >= 1.
    std::size_t offset = 0;
    for (auto const& s : shape)
    {
        // The iterator stored in the shape entry is a plain integer here,
        // so we can compare directly.
        HPX_TEST_EQ(static_cast<std::size_t>(hpx::get<0>(s)), offset);
        offset += hpx::get<1>(s);
    }
}

// ---------------------------------------------------------------------------
// Test 2: base_idx propagation through add_ready_future_idx (shared_future
//         overload)
//
// Before the fix, the shared_future<void> overload of add_ready_future_idx
// had its parameters in the wrong order:
//
//   (workitems, f, base_idx, first, count)   <-- buggy
//   (workitems, f, first, base_idx, count)   <-- correct
//
// The call-site always passes (first, base_idx, count), so the raw iterator
// value was forwarded as base_idx and the actual base_idx was forwarded as
// the iterator, corrupting the index seen by the user functor.
//
// We test this by calling add_ready_future_idx directly with a known iterator
// (an int pointer) and a known base_idx, then asserting that the functor
// receives them in the right order.
// ---------------------------------------------------------------------------
void test_shared_future_base_idx_propagation()
{
    std::vector<int> data(10);
    std::iota(data.begin(), data.end(), 0);

    using FwdIter = std::vector<int>::iterator;

    // Recorded values: what the functor actually saw.
    FwdIter seen_first{};
    std::size_t seen_base_idx = static_cast<std::size_t>(-1);
    std::size_t seen_count = 0;

    auto functor = [&](FwdIter first, std::size_t count, std::size_t base_idx) {
        seen_first = first;
        seen_base_idx = base_idx;
        seen_count = count;
    };

    constexpr std::size_t expected_base_idx = 5;
    constexpr std::size_t expected_count = 3;
    FwdIter expected_first = data.begin() + 2;

    std::vector<hpx::shared_future<void>> workitems;

    hpx::parallel::util::detail::add_ready_future_idx(
        workitems, functor, expected_first, expected_base_idx, expected_count);

    // The workitems vector should have received one ready future.
    HPX_TEST_EQ(workitems.size(), 1u);

    // The functor must have been called with the correct arguments.
    // With the old buggy signature the raw address of expected_first would
    // have been cast to size_t and placed in seen_base_idx instead.
    //
    // Note: iterators do not support operator<<, so we cannot use
    // HPX_TEST_EQ directly for seen_first.  Use HPX_TEST (boolean form)
    // for the iterator comparison and verify the offset separately so
    // that any failure still produces a meaningful diagnostic.
    HPX_TEST(seen_first == expected_first);
    HPX_TEST_EQ(static_cast<std::size_t>(seen_first - data.begin()),
        static_cast<std::size_t>(expected_first - data.begin()));
    HPX_TEST_EQ(seen_base_idx, expected_base_idx);
    HPX_TEST_EQ(seen_count, expected_count);
}

// ---------------------------------------------------------------------------
// Test 3: end-to-end parallel algorithm with variable chunk size
//
// Run hpx::for_each under par with guided_chunk_size (the canonical executor-
// parameters type that sets has_variable_chunk_size = true_type).  The
// partitioner dispatches through the std::true_type overload of
// get_bulk_iteration_shape, which internally calls
// get_bulk_iteration_shape_variable.  Every element must be visited exactly
// once; with the old iterator-advance bug some elements would be visited
// multiple times or not at all.
// ---------------------------------------------------------------------------
void test_parallel_for_each_variable_chunk()
{
    constexpr std::size_t total = 100;
    constexpr std::size_t min_chunk = 3;

    std::vector<std::atomic<int>> visited(total);
    for (auto& a : visited)
        a.store(0);

    // Build an index range that we iterate over.
    std::vector<std::size_t> indices(total);
    std::iota(indices.begin(), indices.end(), 0u);

    auto policy = hpx::execution::par.with(
        hpx::execution::experimental::guided_chunk_size(min_chunk));

    hpx::for_each(policy, indices.begin(), indices.end(),
        [&](std::size_t idx) { visited[idx].fetch_add(1); });

    // Every element must have been visited exactly once.
    for (std::size_t i = 0; i != total; ++i)
    {
        HPX_TEST_EQ(visited[i].load(), 1);
    }
}

int hpx_main()
{
    test_variable_chunk_multi_chunk_traversal();
    test_shared_future_base_idx_propagation();
    test_parallel_for_each_variable_chunk();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
