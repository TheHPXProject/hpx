//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the schedule hint part of #7724: a run_as_child
// task with a thread hint and bound priority could start on another worker,
// because the hint was not passed on when the thread was made pending.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <cstdint>

int hpx_main()
{
    std::size_t const num_workers = hpx::get_os_thread_count();

    for (std::size_t target = 0; target != num_workers; ++target)
    {
        for (int i = 0; i != 10; ++i)
        {
            hpx::threads::thread_schedule_hint hint(
                static_cast<std::int16_t>(target));
            hint.runs_as_child_mode(
                hpx::threads::thread_execution_hint::run_as_child);

            hpx::launch policy = hpx::launch::async;
            policy.set_priority(hpx::threads::thread_priority::bound);
            policy.set_hint(hint);

            std::size_t ran_on = static_cast<std::size_t>(-1);
            hpx::future<void> f = hpx::async(
                policy, [&] { ran_on = hpx::get_worker_thread_num(); });

            // get() or wait() could run the child inline on this worker, so
            // poll until a worker has run it
            while (!f.is_ready())
            {
                hpx::this_thread::yield();
            }
            f.get();

            HPX_TEST_EQ(ran_on, target);
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
