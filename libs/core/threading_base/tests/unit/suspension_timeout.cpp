//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The two kinds of timers that make a thread pending:
//
// - The timeout of a suspension (hpx::this_thread::suspend with a time,
//   sleep_for, a timed wait on a condition variable) is tied to that
//   suspension. Its timer wakes the thread directly, without helper threads,
//   and a timeout whose suspension has ended (the thread was woken
//   otherwise) doesn't touch the thread in a later suspension.
// - A timer set with hpx::threads::set_thread_state and a time is not tied
//   to a suspension. It runs in two helper threads (at_timer and
//   wake_timer), and it has to be aborted through the id it returns if the
//   thread is woken otherwise, or it resumes whatever suspension of the
//   thread comes next.
//
// For each: the timeout wakes the thread, the threads that exist while it
// waits, and a wake up before the timeout.
//
// A thread created suspended to run at a given time (make_ready_future_after,
// the thread of an interval_timer) waits with the timeout of the suspension
// it was created in, so it needs no helper threads either.

#include <hpx/condition_variable.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>

using hpx::threads::thread_restart_state;
using hpx::threads::thread_schedule_state;

namespace {

    std::int64_t suspended_threads()
    {
        return hpx::threads::get_thread_count(
            hpx::threads::thread_priority::default_,
            thread_schedule_state::suspended);
    }

    hpx::chrono::steady_time_point after(std::chrono::milliseconds ms)
    {
        return hpx::chrono::steady_time_point(
            std::chrono::steady_clock::now() + ms);
    }

    void wait_for_flag(std::atomic<bool> const& flag)
    {
        while (!flag.load())
        {
            hpx::this_thread::yield();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // the timeout of a suspension

    // the timeout wakes the thread
    void test_suspension_timeout()
    {
        hpx::async(hpx::launch::task, [] {
            HPX_TEST(hpx::this_thread::suspend(after(std::chrono::milliseconds(
                         10))) == thread_restart_state::timeout);
        }).get();

        // many short timed waits, each ends with its timeout
        hpx::mutex mtx;
        hpx::condition_variable cv;
        for (int i = 0; i != 100; ++i)
        {
            std::unique_lock<hpx::mutex> lk(mtx);
            HPX_TEST(cv.wait_for(lk, std::chrono::microseconds(100)) ==
                hpx::cv_status::timeout);
        }
    }

    // while a thread sleeps, it is the only additional suspended thread
    void test_suspension_timeout_threads()
    {
        std::int64_t const before = suspended_threads();

        std::atomic<bool> sleeping(false);
        hpx::future<void> sleeper = hpx::async(hpx::launch::task, [&sleeping] {
            sleeping = true;
            hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
        });

        wait_for_flag(sleeping);
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));

        // this thread sleeps no more, the sleeper does
        HPX_TEST_EQ(suspended_threads(), before + 1);

        sleeper.get();
    }

    // a timeout whose suspension ended doesn't wake the thread later
    void test_suspension_timeout_after_notify()
    {
        hpx::mutex mtx;
        hpx::condition_variable cv;
        bool notified = false;

        std::atomic<bool> waiting(false);
        std::atomic<bool> suspended(false);
        std::atomic<thread_restart_state> second(thread_restart_state::unknown);
        hpx::threads::thread_id_type waiter_id;

        hpx::future<void> waiter = hpx::async(hpx::launch::task, [&] {
            waiter_id = hpx::threads::get_self_id();
            {
                std::unique_lock<hpx::mutex> lk(mtx);
                waiting = true;
                HPX_TEST(cv.wait_for(lk, std::chrono::milliseconds(50),
                    [&] { return notified; }));
            }

            // the timeout of 50 ms of the wait above must not end this
            suspended = true;
            second =
                hpx::this_thread::suspend(thread_schedule_state::suspended);
        });

        wait_for_flag(waiting);
        {
            std::lock_guard<hpx::mutex> lk(mtx);
            notified = true;
        }
        cv.notify_one();

        wait_for_flag(suspended);

        // well after the timeout of the first wait
        hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
        HPX_TEST(second.load() == thread_restart_state::unknown);

        hpx::threads::set_thread_state(waiter_id,
            thread_schedule_state::pending, thread_restart_state::signaled);
        waiter.get();
        HPX_TEST(second.load() == thread_restart_state::signaled);
    }

    ///////////////////////////////////////////////////////////////////////////
    // a timer set with set_thread_state

    // the timer wakes the thread
    void test_timer()
    {
        hpx::async(hpx::launch::task, [] {
            hpx::threads::set_thread_state(hpx::threads::get_self_id(),
                after(std::chrono::milliseconds(10)));
            HPX_TEST(
                hpx::this_thread::suspend(thread_schedule_state::suspended) ==
                thread_restart_state::timeout);
        }).get();
    }

    // while a thread waits for its timer, the timer's two helper threads
    // (at_timer and wake_timer) are suspended as well
    void test_timer_threads()
    {
        std::int64_t const before = suspended_threads();

        std::atomic<bool> started(false);
        std::atomic<bool> waiting(false);
        hpx::future<void> sleeper =
            hpx::async(hpx::launch::task, [&started, &waiting] {
                hpx::threads::set_thread_state(hpx::threads::get_self_id(),
                    after(std::chrono::milliseconds(200)), &started);
                waiting = true;
                HPX_TEST(hpx::this_thread::suspend(
                             thread_schedule_state::suspended) ==
                    thread_restart_state::timeout);
            });

        wait_for_flag(waiting);
        wait_for_flag(started);
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));

        HPX_TEST_EQ(suspended_threads(), before + 3);

        sleeper.get();
    }

    // a thread woken before its timer aborts the timer, which then doesn't
    // wake it in a later suspension
    void test_timer_aborted()
    {
        std::atomic<bool> suspended(false);
        std::atomic<bool> suspended_again(false);
        std::atomic<thread_restart_state> second(thread_restart_state::unknown);
        hpx::threads::thread_id_type waiter_id;

        hpx::future<void> waiter = hpx::async(hpx::launch::task, [&] {
            waiter_id = hpx::threads::get_self_id();

            std::atomic<bool> started(false);
            hpx::threads::thread_id_ref_type const timer =
                hpx::threads::set_thread_state(
                    waiter_id, after(std::chrono::milliseconds(50)), &started);

            suspended = true;
            HPX_TEST(
                hpx::this_thread::suspend(thread_schedule_state::suspended) ==
                thread_restart_state::signaled);

            // woken before the timer: abort it
            wait_for_flag(started);
            hpx::threads::set_thread_state(timer.noref(),
                thread_schedule_state::pending, thread_restart_state::abort);

            suspended_again = true;
            second =
                hpx::this_thread::suspend(thread_schedule_state::suspended);
        });

        wait_for_flag(suspended);
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        hpx::threads::set_thread_state(waiter_id,
            thread_schedule_state::pending, thread_restart_state::signaled);

        wait_for_flag(suspended_again);

        // well after the timer's time
        hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
        HPX_TEST(second.load() == thread_restart_state::unknown);

        hpx::threads::set_thread_state(waiter_id,
            thread_schedule_state::pending, thread_restart_state::signaled);
        waiter.get();
        HPX_TEST(second.load() == thread_restart_state::signaled);
    }

    ///////////////////////////////////////////////////////////////////////////
    // threads created suspended to run at a given time

    // a future that becomes ready after a time: only its thread waits
    void test_ready_future_after()
    {
        std::int64_t const before = suspended_threads();

        auto const start = std::chrono::steady_clock::now();
        hpx::future<int> f =
            hpx::make_ready_future_after(std::chrono::milliseconds(200), 42);

        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
        HPX_TEST(!f.is_ready());
        HPX_TEST_EQ(suspended_threads(), before + 1);

        HPX_TEST_EQ(f.get(), 42);
        HPX_TEST(std::chrono::steady_clock::now() - start >=
            std::chrono::milliseconds(200));
    }

    // an interval timer runs its function every interval until it is
    // stopped, and again once it is started again
    void test_interval_timer()
    {
        std::atomic<int> ticks(0);
        hpx::util::interval_timer timer(
            [&ticks] {
                ++ticks;
                return true;
            },
            std::chrono::milliseconds(10), "suspension_timeout");

        timer.start(false);
        hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
        HPX_TEST(timer.stop());
        int const stopped = ticks.load();
        HPX_TEST_LT(2, stopped);

        // the timeout of the stopped timer's thread doesn't run it
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        HPX_TEST_EQ(ticks.load(), stopped);

        timer.start(false);
        hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
        timer.stop();
        HPX_TEST_LT(stopped + 2, ticks.load());
    }
}    // namespace

int hpx_main()
{
    test_suspension_timeout();
    test_suspension_timeout_threads();
    test_suspension_timeout_after_notify();

    test_timer();
    test_timer_threads();
    test_timer_aborted();

    test_ready_future_after();
    test_interval_timer();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
