//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

namespace ex = hpx::execution::experimental;

///////////////////////////////////////////////////////////////////////////
// bulk_unchunked tests -- f(index, values...)
//
// Each element in the shape produces exactly one invocation of f.
///////////////////////////////////////////////////////////////////////////

void test_unchunked_sequential()
{
    hpx::execution::sequenced_executor exec;
    std::atomic<std::size_t> call_count{0};

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::bulk_unchunked(10, [&](int i) {
        (void) i;
        ++call_count;
    });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), std::size_t(10));
}

void test_unchunked_parallel()
{
    hpx::execution::parallel_executor exec;
    std::atomic<std::size_t> call_count{0};

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::bulk_unchunked(500, [&](int i) {
        (void) i;
        ++call_count;
    });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), std::size_t(500));
}

void test_unchunked_with_value()
{
    hpx::execution::parallel_executor exec;
    std::atomic<std::size_t> call_count{0};

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::then([]() { return 42; }) |
        ex::bulk_unchunked(100, [&](int i, int val) {
            (void) i;
            HPX_TEST_EQ(val, 42);
            ++call_count;
        });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), std::size_t(100));
}

///////////////////////////////////////////////////////////////////////////
// bulk_chunked tests -- f(begin, end, values...)
//
// The executor_scheduler delivers the entire range as a single monolithic
// chunk.  Every test asserts:
//   1. invocation_count == 1  (proves no O(N^2) redundant calls)
//   2. begin == 0, end == N   (proves correct chunk boundaries)
//   3. value forwarding       (proves upstream values arrive intact)
///////////////////////////////////////////////////////////////////////////

void test_chunked_sequential()
{
    hpx::execution::sequenced_executor exec;
    std::atomic<std::size_t> invocation_count{0};
    int observed_begin = -1;
    int observed_end = -1;

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd =
        ex::schedule(sched) | ex::bulk_chunked(100, [&](int begin, int end) {
            observed_begin = begin;
            observed_end = end;
            ++invocation_count;
        });

    hpx::this_thread::experimental::sync_wait(snd);

    // Exactly one chunk covering the full range
    HPX_TEST_EQ(invocation_count.load(), std::size_t(1));
    HPX_TEST_EQ(observed_begin, 0);
    HPX_TEST_EQ(observed_end, 100);
}

void test_chunked_parallel()
{
    hpx::execution::parallel_executor exec;
    std::atomic<std::size_t> invocation_count{0};
    std::atomic<int> total{0};
    int observed_begin = -1;
    int observed_end = -1;

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd =
        ex::schedule(sched) | ex::bulk_chunked(200, [&](int begin, int end) {
            observed_begin = begin;
            observed_end = end;
            total += (end - begin);
            ++invocation_count;
        });

    hpx::this_thread::experimental::sync_wait(snd);

    // Exactly one invocation, NOT 200
    HPX_TEST_EQ(invocation_count.load(), std::size_t(1));
    HPX_TEST_EQ(total.load(), 200);
    HPX_TEST_EQ(observed_begin, 0);
    HPX_TEST_EQ(observed_end, 200);
}

void test_chunked_with_value()
{
    hpx::execution::parallel_executor exec;
    std::atomic<std::size_t> invocation_count{0};
    std::atomic<int> total{0};

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::then([]() { return 7; }) |
        ex::bulk_chunked(50, [&](int begin, int end, int val) {
            HPX_TEST_EQ(val, 7);
            total += (end - begin);
            ++invocation_count;
        });

    hpx::this_thread::experimental::sync_wait(snd);

    // Single chunk, correct total, value forwarded
    HPX_TEST_EQ(invocation_count.load(), std::size_t(1));
    HPX_TEST_EQ(total.load(), 50);
}

///////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    // bulk_unchunked
    test_unchunked_sequential();
    test_unchunked_parallel();
    test_unchunked_with_value();

    // bulk_chunked
    test_chunked_sequential();
    test_chunked_parallel();
    test_chunked_with_value();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
