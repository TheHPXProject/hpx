//  Copyright (c) 2020 Hartmut Kaiser
//  Copyright (C) 2013 Tim Blechmann
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/semaphore.hpp>
#include <hpx/thread.hpp>

#include <chrono>

void test_semaphore_release_acquire()
{
    hpx::counting_semaphore<> sem(1);

    sem.release();
    sem.acquire();
}

void test_semaphore_try_acquire()
{
    hpx::counting_semaphore<> sem(0);

    HPX_TEST(!sem.try_acquire());
    sem.release();
    HPX_TEST(sem.try_acquire());
}

void test_semaphore_initial_count()
{
    hpx::counting_semaphore<> sem(2);

    HPX_TEST(sem.try_acquire());
    HPX_TEST(sem.try_acquire());
    HPX_TEST(!sem.try_acquire());
}

struct semaphore_acquire_and_release_test
{
    semaphore_acquire_and_release_test()
      : sem_(1)
    {
    }

    void run()
    {
        hpx::thread release_thread(
            &semaphore_acquire_and_release_test::acquire_and_release, this);
        sem_.acquire();
        release_thread.join();
    }

    void acquire_and_release()
    {
        hpx::this_thread::sleep_for(std::chrono::seconds(1));
        sem_.release();
    }

    hpx::counting_semaphore<> sem_;
};

void test_semaphore_acquire_and_release()
{
    semaphore_acquire_and_release_test test;
    test.run();
}

void test_semaphore_try_acquire_for()
{
    hpx::counting_semaphore<> sem(0);

    auto start = std::chrono::system_clock::now();

    HPX_TEST(!sem.try_acquire_for(std::chrono::milliseconds(500)));

    auto end = std::chrono::system_clock::now();
    auto acquire_time = end - start;

    // guessing!
    HPX_TEST(acquire_time > std::chrono::milliseconds(450));
    HPX_TEST(acquire_time < std::chrono::milliseconds(1000));

    sem.release();

    HPX_TEST(sem.try_acquire_for(std::chrono::milliseconds(500)));
}

void test_semaphore_try_acquire_until()
{
    hpx::counting_semaphore<> sem(0);

    {
        auto now = std::chrono::system_clock::now();
        auto timeout = now + std::chrono::milliseconds(500);

        HPX_TEST(!sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();
        auto timeout_delta = end - timeout;

        // guessing!
        HPX_TEST(timeout_delta > std::chrono::milliseconds(-400));
        HPX_TEST(timeout_delta < std::chrono::milliseconds(400));
    }

    sem.release();

    {
        auto start = std::chrono::system_clock::now();
        auto timeout = start + std::chrono::milliseconds(500);

        HPX_TEST(sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();

        // guessing!
        HPX_TEST((end - start) < std::chrono::milliseconds(100));
    }
}

void test_semaphore_try_acquire_for_until()
{
    hpx::counting_semaphore<> sem(0);

    // Relative timeouts
    {
        auto start = std::chrono::system_clock::now();

        HPX_TEST(!sem.try_acquire_for(std::chrono::milliseconds(500)));

        auto end = std::chrono::system_clock::now();
        auto acquire_time = end - start;

        // guessing!
        HPX_TEST(acquire_time > std::chrono::milliseconds(450));
        HPX_TEST(acquire_time < std::chrono::milliseconds(1000));

        sem.release();

        HPX_TEST(sem.try_acquire_for(std::chrono::milliseconds(500)));
    }

    // Absolute timeouts
    {
        auto now = std::chrono::system_clock::now();
        auto timeout = now + std::chrono::milliseconds(500);

        HPX_TEST(!sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();
        auto timeout_delta = end - timeout;

        // guessing!
        HPX_TEST(timeout_delta > std::chrono::milliseconds(-400));
        HPX_TEST(timeout_delta < std::chrono::milliseconds(400));
    }

    sem.release();

    {
        auto start = std::chrono::system_clock::now();
        auto timeout = start + std::chrono::milliseconds(500);

        HPX_TEST(sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();

        // guessing!
        HPX_TEST((end - start) < std::chrono::milliseconds(100));
    }

    sem.release();

    {
        // timed acquire after timeout
        hpx::counting_semaphore<> sema(1);

        auto start = std::chrono::steady_clock::now();
        auto timeout = start + std::chrono::milliseconds(100);

        hpx::this_thread::sleep_for(std::chrono::milliseconds(500));

        sema.release();

        HPX_TEST(sema.try_acquire_until(timeout));
    }
}

// Regression test: try_acquire_until must return true when a concurrent
// release() fires before the deadline, even when the waiter was already
// suspended inside wait_until.  Before the fix, it returned false because
// the restart-state comparison used != unknown instead of == timeout,
// treating thread_restart_state::signaled as a timeout.
void test_try_acquire_until_returns_true_when_signaled_before_deadline()
{
    // 64 iterations: the bug is deterministically present on every one
    // because the release() always arrives before the 5-second deadline.
    for (int iter = 0; iter < 64; ++iter)
    {
        hpx::counting_semaphore<> sem(0);

        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);

        hpx::thread releaser([&sem]() {
            // Yield once to maximise the chance the acquirer is already
            // suspended in wait_until when release() fires.
            hpx::this_thread::yield();
            sem.release();
        });

        bool const acquired = sem.try_acquire_until(deadline);
        releaser.join();

        // Must be true: the semaphore was signaled before the deadline.
        HPX_TEST(acquired);
    }
}

// Multiple waiters: each release() must produce exactly one successful
// try_acquire_until, so the total success count must equal N.
void test_try_acquire_until_multiple_waiters_all_succeed()
{
    constexpr int N = 8;
    hpx::counting_semaphore<> sem(0);
    std::atomic<int> success_count{0};

    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);

    std::vector<hpx::thread> waiters;
    waiters.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        waiters.emplace_back([&sem, &success_count, deadline]() {
            if (sem.try_acquire_until(deadline))
                ++success_count;
        });
    }

    for (int i = 0; i < N; ++i)
        sem.release();

    for (auto& t : waiters)
        t.join();

    HPX_TEST_EQ(success_count.load(), N);
}

int hpx_main()
{
    test_semaphore_release_acquire();
    test_semaphore_try_acquire();
    test_semaphore_initial_count();
    test_semaphore_acquire_and_release();
    test_semaphore_try_acquire_for();
    test_semaphore_try_acquire_until();
    test_semaphore_try_acquire_for_until();
    test_try_acquire_until_returns_true_when_signaled_before_deadline();
    test_try_acquire_until_multiple_waiters_all_succeed();

    hpx::local::finalize();
    return hpx::util::report_errors();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
