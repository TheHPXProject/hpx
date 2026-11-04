//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A waiter in wait_until() may leave its wait without suspending once it sees
// that it was notified, while the notifying thread is still resuming it. The
// wakeup must not reach the waiter in whatever suspension it is in next.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>

using mutex_type = hpx::spinlock;
using condition_variable = hpx::lcos::local::detail::condition_variable;

std::size_t stray_wakeups = 0;

void test_round(bool notify_all)
{
    mutex_type mtx;
    condition_variable cv;
    bool ready = false;

    hpx::threads::thread_id_type const waiter = hpx::threads::get_self_id();
    std::atomic<bool> suspended{false};
    std::atomic<bool> released{false};

    std::unique_lock<mutex_type> l(mtx);

    // the notifier can take the lock only once this thread waits below
    hpx::future<void> f = hpx::async([&] {
        {
            std::unique_lock<mutex_type> nl(mtx);
            ready = true;
            if (notify_all)
            {
                cv.notify_all(HPX_MOVE(nl));
            }
            else
            {
                cv.notify_one(HPX_MOVE(nl));
            }
        }

        // wait for the waiter to suspend below, give a late wakeup some time
        // to arrive, then release the waiter
        hpx::util::yield_while([&] {
            return !suspended.load() ||
                hpx::threads::get_thread_state(waiter).state() !=
                hpx::threads::thread_schedule_state::suspended;
        });
        for (std::size_t k = 0; k != 100; ++k)
        {
            hpx::this_thread::yield();
        }

        released = true;
        hpx::threads::set_thread_state(waiter,
            hpx::threads::thread_schedule_state::pending,
            hpx::threads::thread_restart_state::signaled);
    });

    // short timeouts keep the waiter spinning and checking whether it was
    // notified instead of suspending. Spinning yields with a boosted priority,
    // which doesn't let other work on this worker thread run (such as the
    // notifier), so yield normally now and then.
    std::size_t k = 0;
    while (!ready)
    {
        cv.wait_until(
            l, std::chrono::steady_clock::now() + std::chrono::microseconds(5));

        if (++k % 16 == 0)
        {
            hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
            hpx::this_thread::yield();
        }
    }
    l.unlock();

    suspended = true;
    while (!released.load())
    {
        hpx::this_thread::suspend(
            hpx::threads::thread_schedule_state::suspended);
        if (!released.load())
        {
            ++stray_wakeups;
        }
    }

    f.get();
}

int hpx_main()
{
    for (std::size_t i = 0; i != 200; ++i)
    {
        test_round(false);
        test_round(true);
    }

    HPX_TEST_EQ(stray_wakeups, static_cast<std::size_t>(0));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
