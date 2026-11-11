//  Copyright (c) 2026 STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/async_combinators/wait_all.hpp>
#include <hpx/execution_base/agent_base.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/threading_base/thread_helpers.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std::chrono;

struct wait_observer
{
    std::mutex mutex;
    std::condition_variable cv;
    bool waiting = false;
    hpx::execution_base::agent_ref agent;

    void notify_waiting()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            waiting = true;
        }
        cv.notify_all();
    }

    void wait_until_waiting()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] { return waiting; });
    }

    void publish_agent(hpx::execution_base::agent_ref context)
    {
        std::lock_guard<std::mutex> lock(mutex);
        agent = context;
    }

    hpx::execution_base::agent_ref get_agent()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return agent;
    }
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
        observer_.notify_waiting();
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
        observer_.notify_waiting();
        return delegate_.ref().sleep_for(duration, HPX_MOVE(wait_cond), desc);
    }

    hpx::threads::thread_restart_state sleep_until(
        hpx::chrono::steady_time_point const& deadline,
        hpx::move_only_function<bool()>&& wait_cond, char const* desc) override
    {
        observer_.notify_waiting();
        return delegate_.ref().sleep_until(deadline, HPX_MOVE(wait_cond), desc);
    }

private:
    hpx::execution_base::agent_ref delegate_;
    wait_observer& observer_;
};

int main()
{
    // set_value arrives concurrently with a non-HPX thread entering a
    // timed wait; the wakeup must be recorded, not deadlock resume()
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();
        wait_observer observer;

        std::thread waiter([&] {
            observing_agent agent(
                hpx::execution_base::this_thread::agent(), observer);
            hpx::execution_base::this_thread::reset_agent reset(agent);
            hpx::future_status const status = f.wait_for(seconds(10));
            HPX_TEST(status == hpx::future_status::ready);
        });

        observer.wait_until_waiting();

        p.set_value(42);
        waiter.join();

        HPX_TEST_EQ(f.get(), 42);
    }

    // untimed wait from a non-HPX thread: set_value may arrive while the
    // agent is on its way into suspend(); the recorded wakeup ends the
    // wait instead of hanging resume()
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();
        wait_observer observer;

        std::thread waiter([&] {
            observing_agent agent(
                hpx::execution_base::this_thread::agent(), observer);
            hpx::execution_base::this_thread::reset_agent reset(agent);
            f.wait();
        });

        observer.wait_until_waiting();

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
            HPX_TEST(
                f.wait_for(milliseconds(1)) == hpx::future_status::timeout);

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
        std::mutex m;
        std::condition_variable cv;
        bool timed_out = false;
        bool proceed_to_wait = false;
        bool wait_returned = false;
        wait_observer observer;

        std::thread waiter([&] {
            HPX_TEST(
                f1.wait_for(milliseconds(1)) == hpx::future_status::timeout);
            observing_agent agent(
                hpx::execution_base::this_thread::agent(), observer);
            hpx::execution_base::this_thread::reset_agent reset(agent);
            observer.publish_agent(hpx::execution_base::this_thread::agent());
            {
                std::lock_guard<std::mutex> l(m);
                timed_out = true;
            }
            cv.notify_one();
            {
                std::unique_lock<std::mutex> l(m);
                cv.wait(l, [&] { return proceed_to_wait; });
            }
            f2.wait();
            HPX_TEST(f2.is_ready());
            {
                std::lock_guard<std::mutex> l(m);
                wait_returned = true;
            }
            cv.notify_one();
        });

        {
            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&] { return timed_out; });
        }

        observer.get_agent().resume();
        {
            std::lock_guard<std::mutex> l(m);
            proceed_to_wait = true;
        }
        cv.notify_one();

        observer.wait_until_waiting();
        p1.set_value(0);
        {
            std::unique_lock<std::mutex> l(m);
            HPX_TEST(!cv.wait_for(
                l, milliseconds(20), [&] { return wait_returned; }));
        }
        p2.set_value(42);
        waiter.join();

        HPX_TEST_EQ(f2.get(), 42);
    }

    // resume() must record the wakeup and return instead of waiting for
    // the agent to leave its wait: a waiter whose wait_cond is blocked on
    // a user-level lock (the pattern behind
    // hpx::condition_variable::wait_until(lock, time, pred) re-checking
    // pred) must not deadlock a resume() issued by the lock holder.
    {
        std::mutex user_mtx;
        std::mutex state_mtx;
        std::condition_variable state_cv;
        bool agent_ready = false;
        bool in_cond = false;
        bool allow_finish = false;
        hpx::execution_base::agent_ref agent;

        user_mtx.lock();    // main thread holds the "user's lock"

        std::thread waiter([&] {
            agent = hpx::execution_base::this_thread::agent();
            {
                std::lock_guard<std::mutex> l(state_mtx);
                agent_ready = true;
            }
            state_cv.notify_one();

            auto const state = agent.sleep_until(
                steady_clock::now() + seconds(60),
                [&] {
                    // simulates wait_until re-acquiring the user lock
                    {
                        std::lock_guard<std::mutex> l(state_mtx);
                        in_cond = true;
                    }
                    state_cv.notify_one();
                    std::lock_guard<std::mutex> lk(user_mtx);
                    std::lock_guard<std::mutex> l(state_mtx);
                    return allow_finish;
                },
                "wait_for_7915");
            HPX_TEST(state == hpx::threads::thread_restart_state::signaled);
        });

        {
            std::unique_lock<std::mutex> l(state_mtx);
            state_cv.wait(l, [&] { return agent_ready && in_cond; });
        }
        // waiter is now blocked inside cond(), mid-wait

        bool resumed = false;
        std::thread resumer([&] {
            agent.resume();    // must return without waiting for the waiter
            {
                std::lock_guard<std::mutex> l(state_mtx);
                resumed = true;
            }
            state_cv.notify_one();
        });

        {
            std::unique_lock<std::mutex> l(state_mtx);
            HPX_TEST(state_cv.wait_for(l, seconds(5), [&] { return resumed; }));
        }

        user_mtx.unlock();
        // If resume took the old blocking path, releasing the condition lets
        // the waiter park so the resumer can return before we join it.
        resumer.join();
        {
            std::lock_guard<std::mutex> lock(state_mtx);
            allow_finish = true;
            agent.resume();
        }
        state_cv.notify_one();
        waiter.join();
    }

    // timed wait followed by wait_all on futures completed immediately
    for (int i = 0; i != 100; ++i)
    {
        hpx::promise<int> p1, p2, p3;
        hpx::future<int> f1 = p1.get_future();
        hpx::future<int> f2 = p2.get_future();
        hpx::future<int> f3 = p3.get_future();
        std::mutex m;
        std::condition_variable cv;
        bool timed_out = false;
        bool proceed_to_wait_all = false;
        bool wait_all_returned = false;
        wait_observer observer;

        std::thread waiter([&] {
            HPX_TEST(
                f1.wait_for(milliseconds(1)) == hpx::future_status::timeout);
            observing_agent agent(
                hpx::execution_base::this_thread::agent(), observer);
            hpx::execution_base::this_thread::reset_agent reset(agent);
            observer.publish_agent(hpx::execution_base::this_thread::agent());
            {
                std::lock_guard<std::mutex> l(m);
                timed_out = true;
            }
            cv.notify_one();
            {
                std::unique_lock<std::mutex> l(m);
                cv.wait(l, [&] { return proceed_to_wait_all; });
            }
            hpx::wait_all(f2, f3);
            HPX_TEST(f2.is_ready());
            HPX_TEST(f3.is_ready());
            {
                std::lock_guard<std::mutex> l(m);
                wait_all_returned = true;
            }
            cv.notify_one();
        });

        {
            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&] { return timed_out; });
        }

        observer.get_agent().resume();
        {
            std::lock_guard<std::mutex> l(m);
            proceed_to_wait_all = true;
        }
        cv.notify_one();

        observer.wait_until_waiting();
        p1.set_value(0);
        {
            std::unique_lock<std::mutex> l(m);
            HPX_TEST(!cv.wait_for(
                l, milliseconds(20), [&] { return wait_all_returned; }));
        }
        p2.set_value(1);
        p3.set_value(2);
        waiter.join();

        HPX_TEST_EQ(f2.get(), 1);
        HPX_TEST_EQ(f3.get(), 2);
    }

    // an abort is reported exactly once: later waits on the same
    // thread-local agent must not keep throwing
    {
        std::mutex m;
        std::condition_variable cv;
        bool agent_ready = false;
        hpx::execution_base::agent_ref agent;

        std::thread waiter([&] {
            agent = hpx::execution_base::this_thread::agent();
            {
                std::lock_guard<std::mutex> l(m);
                agent_ready = true;
            }
            cv.notify_one();

            bool threw = false;
            try
            {
                agent.sleep_until(
                    steady_clock::now() + seconds(60), [] { return false; },
                    "wait_for_7915");
            }
            catch (hpx::exception const&)
            {
                threw = true;
            }
            HPX_TEST(threw);

            // the abort was consumed by the throw above: this wait must
            // time out normally instead of throwing again
            auto const state = agent.sleep_until(
                steady_clock::now() + milliseconds(1), [] { return false; },
                "wait_for_7915");
            HPX_TEST(state == hpx::threads::thread_restart_state::timeout);
        });

        {
            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&] { return agent_ready; });
        }

        agent.abort("wait_for_7915");
        waiter.join();
    }

    // An abort during a blocked condition check takes precedence over either
    // a true predicate or a timeout after the deadline.
    for (bool condition_result : {false, true})
    {
        std::mutex m;
        std::condition_variable cv;
        bool in_cond = false;
        bool release_cond = false;
        hpx::execution_base::agent_ref agent;
        auto const deadline = steady_clock::now() + milliseconds(50);
        bool threw_yield_aborted = false;

        std::thread waiter([&] {
            agent = hpx::execution_base::this_thread::agent();
            try
            {
                agent.sleep_until(
                    deadline,
                    [&] {
                        std::unique_lock<std::mutex> l(m);
                        in_cond = true;
                        cv.notify_one();
                        cv.wait(l, [&] { return release_cond; });
                        return condition_result;
                    },
                    "wait_for_7915");
            }
            catch (hpx::exception const& e)
            {
                threw_yield_aborted =
                    e.get_error() == hpx::error::yield_aborted;
            }
        });

        {
            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&] { return in_cond; });
        }
        std::this_thread::sleep_until(deadline + milliseconds(20));
        agent.abort("wait_for_7915");
        {
            std::lock_guard<std::mutex> l(m);
            release_cond = true;
        }
        cv.notify_one();
        waiter.join();

        HPX_TEST(threw_yield_aborted);
    }

    return hpx::util::report_errors();
}
