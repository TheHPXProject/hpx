//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the issue described in #7713: a stackful task
// created with the run_as_child hint and run directly by the parent waiting
// on its future was left in state unknown instead of terminated.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

int hpx_main()
{
    hpx::threads::thread_schedule_hint hint;
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::run_as_child);

    hpx::launch policy = hpx::launch::async;
    policy.set_hint(hint);
    policy.set_stacksize(hpx::threads::thread_stacksize::small_);

    hpx::threads::thread_id_type const parent = hpx::threads::get_self_id();

    int inlined = 0;
    for (int i = 0; i != 100; ++i)
    {
        hpx::lcos::local::futures_factory<int()> task([&] {
            // With one worker, a parent waiting in get() would be suspended,
            // so an active parent means the task runs on the parent's stack.
            if (hpx::threads::get_thread_state(parent).state() ==
                hpx::threads::thread_schedule_state::active)
            {
                ++inlined;
            }
            return 1;
        });
        hpx::future<int> f = task.get_future();
        hpx::threads::thread_id_ref_type const child =
            task.post("stackful child", policy);

        HPX_TEST_EQ(f.get(), 1);

        // A worker running the child from the queue sets the value before
        // the thread itself has finished, so wait for it to leave active.
        while (hpx::threads::get_thread_state(child.noref()).state() ==
            hpx::threads::thread_schedule_state::active)
        {
            hpx::this_thread::yield();
        }
        HPX_TEST_EQ(hpx::threads::get_thread_state(child.noref()).state(),
            hpx::threads::thread_schedule_state::terminated);
    }

    // With one worker nothing else can pick the task up before the parent
    // waits on it, so every one of them has to run directly.
    if (hpx::get_os_thread_count() == 1)
    {
        HPX_TEST_EQ(inlined, 100);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
