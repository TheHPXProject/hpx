//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Concurrency stress tests for hpx::latch / hpx::lcos::local::latch that
// specifically target:
//   - the acq_rel counter update in count_down()/arrive_and_wait(),
//   - the explicit notified_ flag guarding the robust predicate loop in
//     wait()/arrive_and_wait(), and
//   - the notify_waiters() loop that repeatedly calls notify_one() until all
//     blocked waiters have been released.
//
// None of these are single-shot assertions: races only show up with repetition,
// so most tests below loop many times, recreating the latch (and, where
// relevant, varying the number of participating threads) on each iteration.
// Every blocking wait on a future is bounded by a deadline rather than
// performed unconditionally, so that a lost wakeup shows up as a test failure
// instead of a hang.

#define HPX_HAVE_FORCE_NO_CXX_MODULES

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

namespace {

    constexpr std::size_t num_iterations = 25;
    constexpr std::chrono::seconds wait_deadline(1);

    // Convenience wrapper that turns a lost wakeup into a test failure rather
    // than an indefinite hang.
    void expect_ready(hpx::future<void>& f)
    {
        if (!f.is_ready())
        {
            HPX_TEST_EQ(static_cast<int>(f.wait_for(wait_deadline)),
                static_cast<int>(hpx::future_status::ready));
        }
        HPX_TEST(f.is_ready());

        f.get();    // rethrow exceptions, if needed
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
// 1. Exactly-once notification / no lost wakeups.
//
// A varying number of waiters block in wait() on a not-yet-ready latch. A
// single count_down() drives the counter to zero. Every waiter must actually
// return (bounded by a deadline, not an infinite wait); this exercises the
// `while (cond_.data_.notify_one(...))` loop in notify_waiters().
void test_no_lost_wakeups()
{
    for (std::size_t const waiter_counts[] = {1, 2, 4, 8, 16};
        std::size_t num_waiters : waiter_counts)
    {
        for (std::size_t iter = 0; iter != num_iterations; ++iter)
        {
            hpx::latch l(1);
            std::atomic<std::size_t> woken(0);
            std::atomic<std::size_t> waiting(0);

            std::vector<hpx::future<void>> results;
            results.reserve(num_waiters);
            for (std::size_t i = 0; i != num_waiters; ++i)
            {
                // make sure the new task is not executed inline
                results.push_back(
                    hpx::async(hpx::launch::task, [&l, &woken, &waiting]() {
                        ++waiting;
                        l.wait();
                        ++woken;
                    }));
            }

            // make sure all waiters have actually reached l.wait() before
            // releasing the latch, so that this exercises the blocking/notify
            // path rather than a race that some waiters might miss
            while (waiting.load(std::memory_order_acquire) != num_waiters)
            {
                hpx::this_thread::yield();
            }

            l.count_down(1);

            for (auto& f : results)
            {
                expect_ready(f);
            }

            HPX_TEST_EQ(woken.load(), num_waiters);
            HPX_TEST(l.try_wait());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// 2. notified_ flag race on the "last decrementer".
//
// Many threads race count_down(1) against a latch initialized with their count,
// while other threads are concurrently blocked in wait(). Exactly one
// decrementer observes the counter reaching zero (guaranteed by the atomic
// fetch_sub itself), sets notified_, and releases all waiters. This test
// asserts none of the waiters hang (no lost wakeup) and none observe a
// still-blocking predicate forever.
//
// It also documents the current contract that try_wait() (and thus is_ready())
// only inspects counter_, not notified_: as soon as the last count_down() has
// decremented the counter, try_wait() reports ready even though
// notify_waiters() might not (yet) have released any concurrently blocked
// waiters.
void test_last_decrementer_notification()
{
    for (constexpr std::size_t decrementer_counts[] = {2, 4, 8, 16};
        std::size_t const num_decrementers : decrementer_counts)
    {
        for (std::size_t iter = 0; iter != num_iterations; ++iter)
        {
            constexpr std::size_t num_waiters = 8;
            hpx::latch l(static_cast<std::ptrdiff_t>(num_decrementers));
            std::atomic<std::size_t> woken(0);
            std::atomic<std::size_t> waiting(0);

            std::vector<hpx::future<void>> results;
            results.reserve(num_decrementers + num_waiters);

            // Launch all waiters first and make sure each of them has actually
            // reached l.wait() before launching the decrementers, so that all
            // waiters are guaranteed to be blocked when the last.
            for (std::size_t i = 0; i != num_waiters; ++i)
            {
                // make sure the new task is not executed inline
                results.push_back(
                    hpx::async(hpx::launch::task, [&l, &woken, &waiting]() {
                        ++waiting;
                        l.wait();
                        ++woken;
                    }));
            }

            while (waiting.load(std::memory_order_acquire) != num_waiters)
            {
                hpx::this_thread::yield();
            }

            for (std::size_t i = 0; i != num_decrementers; ++i)
            {
                results.push_back(
                    hpx::async(hpx::launch::task, [&l]() { l.count_down(1); }));
            }

            for (auto& f : results)
            {
                expect_ready(f);
            }

            HPX_TEST_EQ(woken.load(), num_waiters);
            HPX_TEST(l.try_wait());
        }
    }
}

void test_try_wait_only_checks_counter()
{
    // As soon as count_down() has decremented counter_ to 0, try_wait() must
    // report ready -- this holds even though the internal notified_ flag and
    // any pending notify_waiters() work are separate from what try_wait()
    // inspects.
    hpx::latch l(1);
    HPX_TEST(!l.try_wait());

    l.count_down(1);

    HPX_TEST(l.try_wait());
}

///////////////////////////////////////////////////////////////////////////////
// 3. count_down() / arrive_and_wait() interleaving.
//
// Mixes plain (non-blocking) count_down() callers with arrive_and_wait()
// callers (which both decrement and block) on the same latch. Asserts no lost
// wakeups and that the final decrement total is correct.
void test_count_down_arrive_and_wait_interleaving()
{
    for (std::size_t iter = 0; iter != num_iterations; ++iter)
    {
        constexpr std::size_t num_waiters = 8;
        constexpr std::size_t num_decrementers = 8;

        hpx::latch l(
            static_cast<std::ptrdiff_t>(num_decrementers + num_waiters));
        std::atomic<std::size_t> woken(0);
        std::atomic<std::size_t> waiting(0);

        std::vector<hpx::future<void>> results;
        results.reserve(num_decrementers + num_waiters);

        // make sure the new task is not executed inline
        for (std::size_t i = 0; i != num_decrementers; ++i)
        {
            results.push_back(
                hpx::async(hpx::launch::task, [&l]() { l.count_down(1); }));
        }
        for (std::size_t i = 0; i != num_waiters; ++i)
        {
            results.push_back(
                hpx::async(hpx::launch::task, [&l, &woken, &waiting]() {
                    ++waiting;
                    l.arrive_and_wait();
                    ++woken;
                }));
        }

        while (waiting.load(std::memory_order_acquire) != num_waiters)
        {
            hpx::this_thread::yield();
        }

        for (auto& f : results)
        {
            expect_ready(f);
        }

        HPX_TEST_EQ(woken.load(), num_waiters);
        HPX_TEST(l.try_wait());
    }
}

///////////////////////////////////////////////////////////////////////////////
// 4a. reset() re-arm safety.
//
// After reset(), notified_ must have been cleared: a waiter that blocks after
// the reset must not return until the latch reaches zero again, and must then
// wake reliably. Repeated across many reset/wait/count_down cycles on the same
// latch.
void test_reset_rearm()
{
    hpx::lcos::local::latch l(0);

    for (std::size_t iter = 0; iter != num_iterations; ++iter)
    {
        l.reset(1);
        HPX_TEST(!l.try_wait());

        // make sure the new task is not executed inline
        hpx::future<void> f =
            hpx::async(hpx::launch::task, [&l]() { l.wait(); });

        // The latch cannot possibly be ready yet -- wait() may only return once
        // counter_ == 0 and notified_ is set, neither of which can happen
        // before the count_down() call below. A timeout here is therefore a
        // deterministic check, not a race.
        HPX_TEST_EQ(static_cast<int>(f.wait_for(std::chrono::milliseconds(50))),
            static_cast<int>(hpx::future_status::timeout));

        l.count_down(1);

        expect_ready(f);
        HPX_TEST(l.try_wait());
    }
}

///////////////////////////////////////////////////////////////////////////////
// 4b. reset_if_needed_and_count_up() re-arm safety.
//
// Many threads race reset_if_needed_and_count_up() against a latch that is
// currently ready (notified_ == true). Exactly one of them must observe
// notified_ == true and perform the reset (returning true); all others must see
// the already-cleared flag and simply count up (returning false).
void test_reset_if_needed_and_count_up()
{
    for (std::size_t iter = 0; iter != num_iterations; ++iter)
    {
        constexpr std::ptrdiff_t count = 5;
        constexpr std::ptrdiff_t n = 3;
        constexpr std::size_t num_callers = 8;

        hpx::lcos::local::latch l(0);
        HPX_TEST(l.try_wait());

        std::atomic<std::size_t> reset_count(0);

        std::vector<hpx::future<void>> results;
        results.reserve(num_callers);
        for (std::size_t i = 0; i != num_callers; ++i)
        {
            // make sure the new task is not executed inline
            results.push_back(
                hpx::async(hpx::launch::task, [&l, &reset_count]() {
                    if (l.reset_if_needed_and_count_up(n, count))
                    {
                        ++reset_count;
                    }
                }));
        }

        for (auto& f : results)
        {
            expect_ready(f);
        }

        // Exactly one caller must have performed the reset.
        HPX_TEST_EQ(reset_count.load(), static_cast<std::size_t>(1));
        HPX_TEST(!l.try_wait());

        constexpr std::ptrdiff_t expected_total =
            n * static_cast<std::ptrdiff_t>(num_callers) + count;

        l.count_down(expected_total);
        HPX_TEST(l.try_wait());
    }
}

///////////////////////////////////////////////////////////////////////////////
// 5. Memory-ordering sanity stress.
//
// A writer sets a plain (non-atomic) value before count_down(); waiters read it
// only after wait() returns. Repeated over many iterations/threads, this is the
// standard (non-formal) pattern used elsewhere in HPX sync tests to catch
// acquire/release bugs.
void test_memory_ordering_sanity()
{
    for (std::size_t iter = 0; iter != num_iterations; ++iter)
    {
        constexpr std::size_t num_waiters = 8;

        hpx::latch l(1);
        std::size_t shared_value = 0;    // plain, non-atomic

        std::vector<hpx::future<void>> results;
        results.reserve(num_waiters);
        for (std::size_t i = 0; i != num_waiters; ++i)
        {
            // make sure the new task is not executed inline
            results.push_back(
                hpx::async(hpx::launch::task, [&l, &shared_value]() {
                    l.wait();
                    HPX_TEST_EQ(shared_value, static_cast<std::size_t>(42));
                }));
        }

        shared_value = 42;
        l.count_down(1);

        for (auto& f : results)
        {
            expect_ready(f);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
#define RUN_TEST(func)                                                         \
    std::cout << #func << std::endl;                                           \
    func();

int hpx_main()
{
    RUN_TEST(test_no_lost_wakeups);
    RUN_TEST(test_last_decrementer_notification);
    RUN_TEST(test_try_wait_only_checks_counter);
    RUN_TEST(test_count_down_arrive_and_wait_interleaving);
    RUN_TEST(test_reset_rearm);
    RUN_TEST(test_reset_if_needed_and_count_up);
    RUN_TEST(test_memory_ordering_sanity);

    HPX_TEST_EQ(hpx::local::finalize(), 0);
    return 0;
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
