//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the issue described in #7708: a stackless task
// created with the run_as_child hint crashed when the parent waiting on its
// future ran it directly.

#include <hpx/execution.hpp>
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

    auto exec = hpx::execution::experimental::with_hint(
        hpx::execution::parallel_executor(
            hpx::threads::thread_stacksize::nostack),
        hint);

    hpx::threads::thread_id_type const parent = hpx::threads::get_self_id();

    int sum = 0;
    int inlined = 0;
    for (int i = 0; i != 100; ++i)
    {
        sum += hpx::async(exec, [&] {
            // With one worker, a parent waiting in get() would be suspended,
            // so an active parent means the task runs on the parent's stack.
            if (hpx::threads::get_thread_state(parent).state() ==
                hpx::threads::thread_schedule_state::active)
            {
                ++inlined;
            }
            return 1;
        }).get();

        // Running the child directly used to clear hpx_main's HPX thread
        // context, which yield needs.
        hpx::this_thread::yield();
    }
    HPX_TEST_EQ(sum, 100);

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
