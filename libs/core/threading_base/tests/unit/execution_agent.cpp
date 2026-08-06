//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <mutex>

// Predicate flips to true from another thread shortly before the deadline,
// but via a plain shared flag (no notify()/resume()) - so the waiting
// hpx::thread stays suspended on its scheduler timer for the full duration
// and only rechecks the predicate once that timer fires. Verifies the
// post-timeout recheck returns signaled (not timeout) when the predicate
// happens to already be true by then.
void test_sleep_predicate_true_at_deadline()
{
    std::atomic<bool> flag{false};
    std::atomic<bool> waiter_started{false};

    hpx::thread setter([&]() {
        // Wait until the waiter thread has entered sleep_for and
        // evaluated the predicate at least once before flipping flag.
        while (!waiter_started.load(std::memory_order_acquire))
        {
            hpx::this_thread::yield();
        }
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
        flag.store(true);
    });

    constexpr auto sleep_duration = std::chrono::milliseconds(500);
    std::chrono::steady_clock::time_point now;

    hpx::threads::thread_restart_state state{};
    hpx::thread waiter([&]() {
        now = std::chrono::steady_clock::now();
        state = hpx::execution_base::this_thread::agent().sleep_for(
            sleep_duration, [&flag, &waiter_started]() {
                waiter_started.store(true, std::memory_order_release);
                return flag.load();
            });
    });

    waiter.join();
    setter.join();

    HPX_TEST(state == hpx::threads::thread_restart_state::signaled);

    // no early-wake claim here: the flag doesn't trigger a resume, so the
    // waiter stays suspended on its timer for (approximately) the full
    // deadline before it rechecks the predicate and returns signaled.
    HPX_TEST(std::chrono::steady_clock::now() >=
        now + sleep_duration - std::chrono::milliseconds(50));
}

// Verifies that hpx::condition_variable::wait_for (which drives
// execution_agent::sleep_until under the hood) wakes a genuine hpx::thread via
// a real notify()-triggered resume once the predicate becomes true, rather than
// only returning at the deadline.
void test_wait_for_notify_before_timeout()
{
    hpx::mutex mtx;
    hpx::condition_variable cv;
    bool flag = false;
    std::atomic<bool> waiter_started{false};

    constexpr auto sleep_duration = std::chrono::milliseconds(500);
    std::chrono::steady_clock::time_point now;

    hpx::thread setter([&]() {
        // Ensure the waiter has already entered wait_for and is holding
        // the lock/checked the predicate once before setting flag.
        while (!waiter_started.load(std::memory_order_acquire))
        {
            hpx::this_thread::yield();
        }
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::lock_guard<hpx::mutex> l(mtx);
        flag = true;
        cv.notify_one();
    });

    bool result = false;
    hpx::thread waiter([&]() {
        now = std::chrono::steady_clock::now();
        std::unique_lock<hpx::mutex> l(mtx);
        result = cv.wait_for(l, sleep_duration, [&]() {
            waiter_started.store(true, std::memory_order_release);
            return flag;
        });
    });

    waiter.join();
    setter.join();

    HPX_TEST(result);

    // must return well before the full deadline - proves notify() actually
    // resumed the suspended waiter instead of it timing out and only then
    // observing the predicate is true.
    HPX_TEST(std::chrono::steady_clock::now() < now + sleep_duration / 2);
}

void test_sleep_with_predicate_hpx_thread_immediate()
{
    constexpr auto sleep_duration = std::chrono::milliseconds(100);
    hpx::threads::thread_restart_state state{};
    std::chrono::steady_clock::time_point now;

    hpx::thread waiter([&]() {
        now = std::chrono::steady_clock::now();
        state = hpx::execution_base::this_thread::agent().sleep_for(
            sleep_duration, []() { return true; });
    });
    waiter.join();

    HPX_TEST(state == hpx::threads::thread_restart_state::signaled);
    HPX_TEST(std::chrono::steady_clock::now() < now + sleep_duration);
}

void test_sleep_with_predicate_hpx_thread_timeout()
{
    constexpr auto sleep_duration = std::chrono::milliseconds(100);
    hpx::threads::thread_restart_state state{};
    std::chrono::steady_clock::time_point now;

    hpx::thread waiter([&]() {
        now = std::chrono::steady_clock::now();
        state = hpx::execution_base::this_thread::agent().sleep_for(
            sleep_duration, []() { return false; });
    });
    waiter.join();

    HPX_TEST(state == hpx::threads::thread_restart_state::timeout);
    HPX_TEST(now + sleep_duration <= std::chrono::steady_clock::now());
}

int hpx_main()
{
    test_sleep_predicate_true_at_deadline();
    test_wait_for_notify_before_timeout();
    test_sleep_with_predicate_hpx_thread_immediate();
    test_sleep_with_predicate_hpx_thread_timeout();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
