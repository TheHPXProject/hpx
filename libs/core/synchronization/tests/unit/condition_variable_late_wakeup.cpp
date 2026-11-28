//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Divyanshi Talreja
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
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>

using mutex_type = hpx::spinlock;
using condition_variable = hpx::lcos::local::detail::condition_variable;

std::size_t stray_wakeups = 0;

struct wait_observer
{
    void entered_suspend()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            ++suspend_entries;
        }
        cv.notify_all();
    }

    void wait_returned()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            returned = true;
        }
        cv.notify_all();
    }

    bool wait_for_second_suspend(std::chrono::seconds timeout)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(
            lock, timeout, [&] { return suspend_entries >= 2 || returned; });
        return suspend_entries >= 2;
    }

    bool has_returned()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return returned;
    }

    std::mutex mtx;
    std::condition_variable cv;
    std::size_t suspend_entries = 0;
    bool returned = false;
};

struct observing_agent final : hpx::execution_base::agent_base
{
    observing_agent(
        hpx::execution_base::agent_ref delegate, wait_observer& observer)
      : delegate_(delegate)
      , observer_(observer)
    {
    }

    std::string description() const override
    {
        return delegate_.ref().description();
    }

    hpx::execution_base::context_base const& context() const noexcept override
    {
        return delegate_.ref().context();
    }

    void yield(char const* desc) override
    {
        delegate_.ref().yield(desc);
    }

    bool yield_k(std::size_t k, char const* desc) override
    {
        return delegate_.ref().yield_k(k, desc);
    }

    void suspend(char const* desc) override
    {
        observer_.entered_suspend();
        delegate_.ref().suspend(desc);
    }

    void resume(
        hpx::threads::thread_priority priority, char const* desc) override
    {
        delegate_.ref().resume(priority, desc);
    }

    void abort(char const* desc) override
    {
        delegate_.ref().abort(desc);
    }

    hpx::threads::thread_restart_state sleep_for(
        hpx::chrono::steady_duration const& duration,
        hpx::move_only_function<bool()>&& wait_cond, char const* desc) override
    {
        return delegate_.ref().sleep_for(duration, HPX_MOVE(wait_cond), desc);
    }

    hpx::threads::thread_restart_state sleep_until(
        hpx::chrono::steady_time_point const& deadline,
        hpx::move_only_function<bool()>&& wait_cond, char const* desc) override
    {
        return delegate_.ref().sleep_until(deadline, HPX_MOVE(wait_cond), desc);
    }

private:
    hpx::execution_base::agent_ref delegate_;
    wait_observer& observer_;
};

// A wakeup left by an earlier wait must not complete a later untimed wait on
// the same non-HPX thread. The later wait has to suspend again until its own
// queue entry is notified.
void test_std_thread_stale_wakeup()
{
    mutex_type mtx;
    condition_variable cv;
    wait_observer observer;
    bool cancelled = false;

    std::thread waiter([&] {
        observing_agent agent(
            hpx::execution_base::this_thread::agent(), observer);
        hpx::execution_base::this_thread::reset_agent reset(agent);

        // Simulate a late resume belonging to an earlier wait. This records a
        // wakeup in the underlying default_agent before the condition-variable
        // wait below has parked.
        hpx::execution_base::this_thread::agent().resume();

        auto result = hpx::threads::thread_restart_state::signaled;
        bool waited = false;
        {
            std::unique_lock<mutex_type> lock(mtx);

            // If the bounded observer wait expired before this thread queued
            // its entry, do not enter a wait that can no longer be notified.
            if (!cancelled)
            {
                waited = true;
                result = cv.wait(lock);
            }
        }

        if (waited)
        {
            HPX_TEST_EQ(result, hpx::threads::thread_restart_state::signaled);
        }
        observer.wait_returned();
    });

    // The first suspend consumes the stale wakeup. A correct wait sees that
    // its own queue entry is still active and suspends a second time. The old
    // single-attempt implementation returns timeout instead.
    bool const suspended_again =
        observer.wait_for_second_suspend(std::chrono::seconds(10));

    HPX_TEST(suspended_again);
    HPX_TEST(!observer.has_returned());

    // If the waiter has not queued yet, cancellation prevents it from
    // entering the wait later. Otherwise, notify its existing queue entry.
    {
        std::unique_lock<mutex_type> lock(mtx);
        cancelled = !suspended_again;
        cv.notify_one(HPX_MOVE(lock));
    }

    waiter.join();
    HPX_TEST(observer.has_returned());
}
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

// Exercise notification around a timed wait and check the next suspension.
// This does not force a pause between observing the first suspension and
// waking the waiter, so it is not a deterministic reproducer of #7916.
void test_timed_round(bool notify_all)
{
    mutex_type mtx;
    condition_variable cv;

    hpx::threads::thread_id_type const waiter = hpx::threads::get_self_id();
    std::atomic<bool> first_waiting{false};
    std::atomic<bool> second_suspended{false};
    std::atomic<bool> released{false};

    std::unique_lock<mutex_type> l(mtx);
    hpx::future<void> f = hpx::async([&] {
        // Race the notification against an actual timed suspension: wait
        // until the waiter is in it. The waiter doesn't suspend anywhere else
        // after it set first_waiting, so the first suspension seen is the
        // timed one. The wait is bounded: the timeout may end the suspension
        // before it is seen, which is one of the accepted results.
        for (std::size_t k = 0; k != 100000 &&
            (!first_waiting.load() ||
                hpx::threads::get_thread_state(waiter).state() !=
                    hpx::threads::thread_schedule_state::suspended);
            ++k)
        {
            hpx::this_thread::yield();
        }

        std::unique_lock<mutex_type> nl(mtx);
        if (notify_all)
        {
            cv.notify_all(HPX_MOVE(nl));
        }
        else
        {
            cv.notify_one(HPX_MOVE(nl));
        }

        hpx::util::yield_while([&] {
            return !second_suspended.load() ||
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

    first_waiting = true;
    auto const result = cv.wait_until(
        l, std::chrono::steady_clock::now() + std::chrono::milliseconds(2));
    HPX_TEST(result == hpx::threads::thread_restart_state::signaled ||
        result == hpx::threads::thread_restart_state::timeout);
    l.unlock();

    second_suspended = true;
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
    test_std_thread_stale_wakeup();

    for (std::size_t i = 0; i != 200; ++i)
    {
        test_round(false);
        test_round(true);
    }

    for (std::size_t i = 0; i != 100; ++i)
    {
        test_timed_round(false);
        test_timed_round(true);
    }

    HPX_TEST_EQ(stray_wakeups, static_cast<std::size_t>(0));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
