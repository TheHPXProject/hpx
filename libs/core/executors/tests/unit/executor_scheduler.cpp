//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/modules/executors.hpp>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void test_executor_scheduler_schedule()
{
    using namespace hpx::execution::experimental;

    hpx::execution::sequenced_executor exec;

    // Retrieve a P2300-compliant scheduler from the legacy executor
    // using the native query() member function path
    auto sched = exec.query(get_scheduler_t{});

    // Verify the scheduler satisfies the is_scheduler trait
    static_assert(is_scheduler_v<decltype(sched)>,
        "executor_scheduler must satisfy is_scheduler");

    // Capture the main thread's ID
    auto main_tid = hpx::this_thread::get_id();

    // Create a sender pipeline: schedule | then
    auto s =
        then(schedule(sched), [&]() { return hpx::this_thread::get_id(); });

    // Execute synchronously and verify the result
    auto result = hpx::this_thread::experimental::sync_wait(std::move(s));

    HPX_TEST(result.has_value());

    if (result)
    {
        // For a sequenced executor, the work runs inline on the calling thread
        auto executed_tid = hpx::get<0>(*result);
        HPX_TEST_EQ(executed_tid, main_tid);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_executor_scheduler_schedule_parallel()
{
    using namespace hpx::execution::experimental;

    hpx::execution::parallel_executor exec;

    // Retrieve a P2300-compliant scheduler from the legacy executor
    auto sched = exec.query(get_scheduler_t{});

    // Verify the scheduler satisfies the is_scheduler trait
    static_assert(is_scheduler_v<decltype(sched)>,
        "executor_scheduler must satisfy is_scheduler");

    // Capture the main thread's ID
    auto main_tid = hpx::this_thread::get_id();

    // Create a sender pipeline: schedule | then
    auto s =
        then(schedule(sched), [&]() { return hpx::this_thread::get_id(); });

    // Execute synchronously and verify the result
    auto result = hpx::this_thread::experimental::sync_wait(std::move(s));

    HPX_TEST(result.has_value());

    if (result)
    {
        // For a parallel executor, the work may run on a different thread
        auto executed_tid = hpx::get<0>(*result);
        HPX_TEST_NEQ(executed_tid, hpx::thread::id());    // valid thread ID
    }
    (void) main_tid;    // used only for documentation
}

///////////////////////////////////////////////////////////////////////////////
void test_executor_scheduler_schedule_restricted()
{
    using namespace hpx::execution::experimental;

    hpx::execution::experimental::restricted_thread_pool_executor exec;

    // Retrieve a P2300-compliant scheduler from the legacy executor
    auto sched = exec.query(get_scheduler_t{});

    // Verify the scheduler satisfies the is_scheduler trait
    static_assert(is_scheduler_v<decltype(sched)>,
        "executor_scheduler must satisfy is_scheduler");

    // Capture the main thread's ID
    auto main_tid = hpx::this_thread::get_id();

    // Create a sender pipeline: schedule | then
    auto s =
        then(schedule(sched), [&]() { return hpx::this_thread::get_id(); });

    // Execute synchronously and verify the result
    auto result = hpx::this_thread::experimental::sync_wait(std::move(s));

    HPX_TEST(result.has_value());

    if (result)
    {
        // For a restricted executor, the work may run on a different thread
        auto executed_tid = hpx::get<0>(*result);
        HPX_TEST_NEQ(executed_tid, hpx::thread::id());    // valid thread ID
    }
    (void) main_tid;    // used only for documentation
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_executor_scheduler_schedule();
    test_executor_scheduler_schedule_parallel();
    test_executor_scheduler_schedule_restricted();

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
