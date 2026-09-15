//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

std::size_t dummy_called = 0;
std::size_t dummy_sleep_for_called = 0;
std::size_t dummy_sleep_until_called = 0;

struct dummy_context final : hpx::execution_base::context_base
{
    hpx::execution_base::resource_base const& resource() const noexcept override
    {
        return resource_;
    }

    hpx::execution_base::resource_base resource_;
};

struct dummy_agent : hpx::execution_base::agent_base
{
    std::string description() const override
    {
        return "";
    }
    dummy_context const& context() const noexcept override
    {
        return context_;
    }

    void yield(char const*) override
    {
        ++dummy_called;
    }
    bool yield_k(std::size_t, char const*) override
    {
        return true;
    }
    void suspend(char const*) override {}
    void resume(hpx::threads::thread_priority, char const*) override {}
    void abort(char const*) override {}

    hpx::threads::thread_restart_state sleep_for(
        hpx::chrono::steady_duration const&,
        hpx::move_only_function<bool()>&& wait_cond, char const*) override
    {
        ++dummy_sleep_for_called;
        return wait_cond && wait_cond() ?
            hpx::threads::thread_restart_state::signaled :
            hpx::threads::thread_restart_state::timeout;
    }
    hpx::threads::thread_restart_state sleep_until(
        hpx::chrono::steady_time_point const&,
        hpx::move_only_function<bool()>&& wait_cond, char const*) override
    {
        ++dummy_sleep_until_called;
        return wait_cond && wait_cond() ?
            hpx::threads::thread_restart_state::signaled :
            hpx::threads::thread_restart_state::timeout;
    }

    dummy_context context_;
};

void test_basic_functionality()
{
    // Test that execution context forwards properly and resetting works
    {
        HPX_TEST_EQ(dummy_called, 0u);
        {
            dummy_agent dummy;
            hpx::execution_base::this_thread::reset_agent ctx(dummy);
            hpx::execution_base::this_thread::yield();
        }

        HPX_TEST_EQ(dummy_called, 1u);

        hpx::execution_base::this_thread::yield();

        HPX_TEST_EQ(dummy_called, 1u);
    }

    // Test that we get different contexts in different threads...
    {
        auto context = hpx::execution_base::this_thread::agent();
        std::thread t([&context]() {
            HPX_TEST_NEQ(context, hpx::execution_base::this_thread::agent());
        });
        t.join();
    }
}

struct simple_spinlock
{
    simple_spinlock() = default;

    void lock()
    {
        while (locked_.test_and_set())
        {
            hpx::execution_base::this_thread::yield();
        }
    }

    void unlock()
    {
        locked_.clear();
    }

#if defined(HPX_HAVE_CXX11_ATOMIC_FLAG_INIT)
    std::atomic_flag locked_ = ATOMIC_FLAG_INIT;
#else
    std::atomic_flag locked_;
#endif
};

void test_yield()
{
    std::vector<std::thread> ts;
    simple_spinlock mutex;
    std::size_t counter = 0;
    std::size_t repetitions = 1000;
    for (std::size_t i = 0;
        i != static_cast<std::size_t>(std::thread::hardware_concurrency()) * 10;
        ++i)
    {
        ts.emplace_back([&mutex, &counter, repetitions]() {
            for (std::size_t repeat = 0; repeat != repetitions; ++repeat)
            {
                std::unique_lock<simple_spinlock> l(mutex);
                ++counter;
            }
        });
    }

    for (auto& t : ts)
        t.join();

    HPX_TEST_EQ(
        counter, std::thread::hardware_concurrency() * repetitions * 10);
}

void test_suspend_resume()
{
    std::mutex mtx;
    hpx::execution_base::agent_ref suspended;

    bool resumed = false;

    std::thread t1([&mtx, &suspended, &resumed]() {
        auto const context = hpx::execution_base::this_thread::agent();
        {
            std::unique_lock<std::mutex> l(mtx);
            suspended = context;
        }
        context.suspend();
        resumed = true;
    });

    while (true)
    {
        std::unique_lock<std::mutex> l(mtx);
        if (suspended)
            break;
    }

    suspended.resume();

    t1.join();
    HPX_TEST(resumed);
}

void test_sleep()
{
    auto const now = std::chrono::steady_clock::now();
    auto const sleep_duration = std::chrono::milliseconds(100);
    hpx::execution_base::this_thread::sleep_for(sleep_duration);
    HPX_TEST(now + sleep_duration <= std::chrono::steady_clock::now());

    auto const sleep_time =
        sleep_duration * 2 + std::chrono::steady_clock::now();
    hpx::execution_base::this_thread::sleep_until(sleep_time);
    HPX_TEST(now + sleep_duration * 2 <= std::chrono::steady_clock::now());
}

void test_sleep_with_predicate()
{
    // predicate satisfied immediately -> the agent should report that it was
    // signaled, and the (move-only) predicate must have been forwarded all the
    // way down to the agent implementation.
    {
        dummy_agent dummy;
        hpx::execution_base::this_thread::reset_agent ctx(dummy);

        dummy_sleep_for_called = 0;
        auto const state = hpx::execution_base::this_thread::agent().sleep_for(
            std::chrono::milliseconds(100), []() { return true; });
        HPX_TEST_EQ(dummy_sleep_for_called, 1u);
        HPX_TEST(state == hpx::threads::thread_restart_state::signaled);
    }

    // predicate never satisfied -> the agent should report a timeout
    {
        dummy_agent dummy;
        hpx::execution_base::this_thread::reset_agent ctx(dummy);

        dummy_sleep_until_called = 0;
        auto const state =
            hpx::execution_base::this_thread::agent().sleep_until(
                std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(100),
                []() { return false; });
        HPX_TEST_EQ(dummy_sleep_until_called, 1u);
        HPX_TEST(state == hpx::threads::thread_restart_state::timeout);
    }

    // move-only predicates must be forwarded by value, not copied
    {
        dummy_agent dummy;
        hpx::execution_base::this_thread::reset_agent ctx(dummy);

        auto flag = std::make_unique<bool>(true);
        auto const state = hpx::execution_base::this_thread::agent().sleep_for(
            std::chrono::milliseconds(100),
            [flag = std::move(flag)]() { return *flag; });
        HPX_TEST(state == hpx::threads::thread_restart_state::signaled);
    }

    // the real (default) agent used when no other agent context has been
    // installed now also supports predicate-based early wake up (checked at
    // up to default_agent_poll_interval granularity): if the predicate is
    // already true it returns signaled well before the requested duration
    // elapses.
    {
        constexpr auto sleep_duration = std::chrono::milliseconds(100);
        auto const now = std::chrono::steady_clock::now();
        auto const state = hpx::execution_base::this_thread::agent().sleep_for(
            sleep_duration, []() { return true; });
        HPX_TEST(state == hpx::threads::thread_restart_state::signaled);
        HPX_TEST(std::chrono::steady_clock::now() < now + sleep_duration);
    }

    // the real (default) agent still reports a timeout when the predicate
    // never becomes true, and waits out (approximately) the full duration.
    {
        constexpr auto sleep_duration = std::chrono::milliseconds(100);
        auto const now = std::chrono::steady_clock::now();
        auto const state = hpx::execution_base::this_thread::agent().sleep_for(
            sleep_duration, []() { return false; });
        HPX_TEST(state == hpx::threads::thread_restart_state::timeout);
        HPX_TEST(now + sleep_duration <= std::chrono::steady_clock::now());
    }

    // predicate starts false and becomes true partway through a much longer
    // deadline: verifies the poll loop actually re-checks the predicate rather
    // than only at the initial call and the final deadline check.
    {
        std::atomic<bool> flag{false};
        std::thread setter([&flag]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            flag.store(true);
        });

        constexpr auto sleep_duration = std::chrono::milliseconds(500);
        auto const now = std::chrono::steady_clock::now();
        auto const state = hpx::execution_base::this_thread::agent().sleep_for(
            sleep_duration, [&flag]() { return flag.load(); });

        setter.join();

        HPX_TEST(state == hpx::threads::thread_restart_state::signaled);

        // must return before the full 500ms deadline, proving the predicate was
        // polled mid-wait (poll interval is 20ms) and not only checked once up
        // front or at the deadline.
        HPX_TEST(std::chrono::steady_clock::now() < now + sleep_duration);
    }
}

int main()
{
    test_basic_functionality();
    test_yield();
    test_suspend_resume();
    test_sleep();
    test_sleep_with_predicate();

    return hpx::util::report_errors();
}
