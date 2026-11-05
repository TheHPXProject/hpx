//  Copyright (c) 2026 STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_main.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std::chrono;

int main()
{
    // set_value arrives while a non-HPX thread is in a timed wait
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();
        std::atomic<bool> entered{false};

        std::thread waiter([&] {
            entered.store(true, std::memory_order_release);
            hpx::future_status const status = f.wait_for(seconds(10));
            HPX_TEST(status == hpx::future_status::ready);
        });

        // handshake, then enough time for the waiter to enter the wait,
        // even on a loaded CI machine
        while (!entered.load(std::memory_order_acquire))
            std::this_thread::yield();
        std::this_thread::sleep_for(milliseconds(20));

        p.set_value(42);
        waiter.join();

        HPX_TEST_EQ(f.get(), 42);
    }

    // untimed wait from a non-HPX thread
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();
        std::atomic<bool> entered{false};

        std::thread waiter([&] {
            entered.store(true, std::memory_order_release);
            f.wait();
        });

        while (!entered.load(std::memory_order_acquire))
            std::this_thread::yield();
        std::this_thread::sleep_for(milliseconds(20));

        p.set_value(42);
        waiter.join();

        HPX_TEST_EQ(f.get(), 42);
    }

    // the waiter times out before set_value arrives
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        std::thread waiter([&] {
            hpx::future_status const status = f.wait_for(milliseconds(1));
            HPX_TEST(status == hpx::future_status::timeout);
        });
        waiter.join();

        p.set_value(42);
    }

    // deterministic #7915: resume() on an agent whose timed wait already
    // ended on its own must return instead of hanging
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        std::mutex m;
        std::condition_variable cv;
        bool published = false;
        bool release = false;
        hpx::execution_base::agent_ref agent;

        std::thread waiter([&] {
            HPX_TEST(f.wait_for(milliseconds(1)) ==
                hpx::future_status::timeout);

            {
                std::lock_guard<std::mutex> l(m);
                agent = hpx::execution_base::this_thread::agent();
                published = true;
            }
            cv.notify_one();

            // stay alive without touching the agent again
            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&] { return release; });
        });

        {
            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&] { return published; });
        }

        // would wait forever for a suspension that never happens,
        // before the fix
        agent.resume();

        {
            std::lock_guard<std::mutex> l(m);
            release = true;
        }
        cv.notify_one();
        waiter.join();

        p.set_value(42);
    }

    // a completion left over from a timed wait must not satisfy a later
    // untimed wait on the same thread-local agent
    for (int i = 0; i != 100; ++i)
    {
        hpx::promise<int> p1;
        hpx::promise<int> p2;
        hpx::future<int> f1 = p1.get_future();
        hpx::future<int> f2 = p2.get_future();
        std::atomic<bool> timed_out{false};

        std::thread waiter([&] {
            HPX_TEST(f1.wait_for(milliseconds(1)) ==
                hpx::future_status::timeout);
            timed_out.store(true, std::memory_order_release);
            f2.wait();
        });

        while (!timed_out.load(std::memory_order_acquire))
            std::this_thread::yield();

        // notify as close behind the waiter as possible, to land the
        // resume() in the publish/suspend window
        p2.set_value(42);
        waiter.join();

        p1.set_value(0);
        HPX_TEST_EQ(f2.get(), 42);
    }

    return hpx::util::report_errors();
}
