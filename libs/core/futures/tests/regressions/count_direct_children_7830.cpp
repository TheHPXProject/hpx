//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the issue described in #7830: a run_as_child
// task run directly by the parent waiting on its future was not counted as
// an executed thread.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>

#if defined(HPX_HAVE_THREAD_CUMULATIVE_COUNTS)
namespace {

    constexpr int num_children = 100;

    std::int64_t executed_threads()
    {
        return hpx::resource::get_thread_pool("default").get_executed_threads(
            static_cast<std::size_t>(-1), false);
    }

    std::int64_t executed_thread_phases()
    {
        return hpx::resource::get_thread_pool("default")
            .get_executed_thread_phases(static_cast<std::size_t>(-1), false);
    }

    // A child run by a worker is counted when the worker releases it, a little
    // after its value is set, so wait for the count to settle before checking
    // that every child was counted exactly once.
    void check_counted(std::int64_t before, char const* what)
    {
        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (executed_threads() - before < num_children &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::yield();
        }
        for (int i = 0; i != 100; ++i)
        {
            hpx::this_thread::yield();
        }
        HPX_TEST_EQ_MSG(executed_threads() - before,
            static_cast<std::int64_t>(num_children), what);
    }

    hpx::launch child_policy(hpx::launch policy)
    {
        hpx::threads::thread_schedule_hint hint;
        hint.runs_as_child_mode(
            hpx::threads::thread_execution_hint::run_as_child);
        policy.set_hint(hint);
        return policy;
    }

    // Children the parent waits on with get() right away, so with one worker
    // the parent runs each of them directly.
    void test_run_directly(hpx::launch policy, char const* what)
    {
        hpx::threads::thread_id_type const parent = hpx::threads::get_self_id();

        int inlined = 0;
        std::int64_t const before = executed_threads();
        std::int64_t const phases_before = executed_thread_phases();
        for (int i = 0; i != num_children; ++i)
        {
            hpx::lcos::local::futures_factory<void()> task([&] {
                // an active parent means the task runs on the parent's stack
                if (hpx::threads::get_thread_state(parent).state() ==
                    hpx::threads::thread_schedule_state::active)
                {
                    ++inlined;
                }
            });
            hpx::future<void> f = task.get_future();
            task.post("child", child_policy(policy));
            f.get();
        }
        // read before check_counted, whose yields add phases of their own
        std::int64_t const phases = executed_thread_phases() - phases_before;
        check_counted(before, what);

        // With one worker the parent never yields in the loop, so each child
        // run directly is exactly one phase. More workers run some children
        // themselves and count those phases when they are done.
        if (hpx::get_os_thread_count() == 1)
        {
            HPX_TEST_EQ_MSG(inlined, num_children, what);
            HPX_TEST_EQ_MSG(
                phases, static_cast<std::int64_t>(num_children), what);
        }
    }

    // Children the parent does not run itself: it only polls the future, so
    // a worker runs each child from its queue.
    void test_run_by_worker()
    {
        std::int64_t const before = executed_threads();
        for (int i = 0; i != num_children; ++i)
        {
            hpx::lcos::local::futures_factory<void()> task([] {});
            hpx::future<void> f = task.get_future();
            task.post("child", child_policy(hpx::launch::async));
            while (!f.is_ready())
            {
                hpx::this_thread::yield();
            }
        }
        check_counted(before, "async, run by a worker");
    }

    // hpx::async with launch::fork yields to the child right away, so the
    // child runs as a normal thread, not directly in the parent.
    void test_async_fork()
    {
        std::int64_t const before = executed_threads();
        for (int i = 0; i != num_children; ++i)
        {
            hpx::async(child_policy(hpx::launch::fork), [] {}).get();
        }
        check_counted(before, "hpx::async with fork");
    }
}    // namespace
#endif

int hpx_main()
{
#if defined(HPX_HAVE_THREAD_CUMULATIVE_COUNTS)
    test_run_directly(hpx::launch::async, "async, run directly");
    test_run_directly(hpx::launch::fork, "fork, run directly");
    test_run_by_worker();
    test_async_fork();
#endif
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
