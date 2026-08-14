//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/runtime_local/pool_timer.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////
// pool_timer is neither copyable nor movable
static_assert(!std::is_copy_constructible_v<hpx::util::pool_timer>);
static_assert(!std::is_move_constructible_v<hpx::util::pool_timer>);
static_assert(!std::is_copy_assignable_v<hpx::util::pool_timer>);
static_assert(!std::is_move_assignable_v<hpx::util::pool_timer>);

///////////////////////////////////////////////////////////////////////////////
// This atomic is intentionally kept alive across the whole run. It is used to
// verify that terminating a pool_timer more than once (once explicitly through
// its destructor, once implicitly through the pre-shutdown function registered
// by start()) only invokes the termination callback a single time.
std::atomic<int> g_double_terminate_count{0};

///////////////////////////////////////////////////////////////////////////////
// Spin-wait on a predicate backed by atomics, bounded by a timeout so that a
// broken test fails instead of hanging CI.
template <typename Pred>
bool wait_for_atomic(Pred&& pred,
    std::chrono::milliseconds const timeout = std::chrono::seconds(10))
{
    auto const start = std::chrono::steady_clock::now();
    while (!pred())
    {
        if (std::chrono::steady_clock::now() - start > timeout)
        {
            return false;
        }
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
void test_default_construction()
{
    hpx::util::pool_timer timer;

    HPX_TEST(!timer.is_valid());
    HPX_TEST(!timer.is_started());
    HPX_TEST(!timer.is_terminated());

    // start()/stop() must fail before init() has been called
    HPX_TEST(!timer.start(std::chrono::seconds(30)));
    HPX_TEST(!timer.stop());

    // deferred initialization succeeds exactly once
    HPX_TEST(timer.init(
        []() -> bool { return false; }, []() {}, "default_construction", true));
    HPX_TEST(timer.is_valid());
    HPX_TEST(!timer.is_started());
    HPX_TEST(!timer.is_terminated());

    // a second init() call fails, the timer is already initialized
    HPX_TEST(!timer.init(
        []() -> bool { return false; }, []() {}, "default_construction", true));

    // normal start/stop lifecycle works after deferred initialization
    HPX_TEST(timer.start(std::chrono::seconds(30)));
    HPX_TEST(timer.is_started());
    HPX_TEST(timer.stop());
    HPX_TEST(!timer.is_started());
}

///////////////////////////////////////////////////////////////////////////////
void test_construction_with_callbacks(bool const pre_shutdown)
{
    std::atomic<int> fire_count{0};
    std::atomic<int> term_count{0};

    {
        hpx::util::pool_timer const timer(
            [&fire_count]() -> bool {
                ++fire_count;
                return false;
            },
            [&term_count]() { ++term_count; }, "construction_test",
            pre_shutdown);

        HPX_TEST(!timer.is_started());
        HPX_TEST(!timer.is_terminated());
    }

    // destroying the timer terminates it exactly once
    HPX_TEST_EQ(term_count.load(), 1);
    HPX_TEST_EQ(fire_count.load(), 0);
}

///////////////////////////////////////////////////////////////////////////////
void test_construction_empty_description()
{
    hpx::util::pool_timer const timer(
        []() -> bool { return false; }, []() {}, "", true);

    HPX_TEST(!timer.is_started());
    HPX_TEST(timer.start(std::chrono::seconds(30)));
    HPX_TEST(timer.stop());
}

///////////////////////////////////////////////////////////////////////////////
void test_start_stop_basic()
{
    std::atomic<int> fire_count{0};
    hpx::util::pool_timer const timer(
        [&fire_count]() -> bool {
            ++fire_count;
            return false;
        },
        []() {}, "start_stop_basic", true);

    // starting a non-started timer succeeds
    HPX_TEST(timer.start(std::chrono::seconds(30)));
    HPX_TEST(timer.is_started());

    // starting an already-started timer fails (no double-start)
    HPX_TEST(!timer.start(std::chrono::seconds(30)));
    HPX_TEST(timer.is_started());

    // stopping a started, not-yet-fired timer succeeds and cancels it
    HPX_TEST(timer.stop());
    HPX_TEST(!timer.is_started());

    // stopping an already-stopped (non-started) timer fails
    HPX_TEST(!timer.stop());

    // give any pending handler a chance to run; it must not fire since the
    // timer was cancelled before expiring
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    HPX_TEST_EQ(fire_count.load(), 0);
}

///////////////////////////////////////////////////////////////////////////////
void test_stop_without_start()
{
    hpx::util::pool_timer const timer(
        []() -> bool { return false; }, []() {}, "stop_without_start", true);

    HPX_TEST(!timer.stop());
}

///////////////////////////////////////////////////////////////////////////////
void test_timer_firing()
{
    std::atomic<int> fire_count{0};
    hpx::util::pool_timer const timer(
        [&fire_count]() -> bool {
            ++fire_count;
            return false;
        },
        []() {}, "timer_firing", true);

    HPX_TEST(timer.start(std::chrono::milliseconds(10)));

    HPX_TEST(wait_for_atomic([&fire_count]() {
        return fire_count.load(std::memory_order_acquire) >= 1;
    }));

    HPX_TEST_EQ(fire_count.load(), 1);
    HPX_TEST(wait_for_atomic([&timer]() { return !timer.is_started(); }));
}

///////////////////////////////////////////////////////////////////////////////
void test_repeated_start_fire_cycles()
{
    std::atomic<int> fire_count{0};
    hpx::util::pool_timer const timer(
        [&fire_count]() -> bool {
            ++fire_count;
            return false;
        },
        []() {}, "repeated_cycles", true);

    constexpr int num_cycles = 5;
    for (int i = 0; i != num_cycles; ++i)
    {
        HPX_TEST(timer.start(std::chrono::milliseconds(5)));

        int const expected = i + 1;
        HPX_TEST(wait_for_atomic([&fire_count, expected]() {
            return fire_count.load(std::memory_order_acquire) >= expected;
        }));
    }

    HPX_TEST_EQ(fire_count.load(), num_cycles);
}

///////////////////////////////////////////////////////////////////////////////
void test_callback_return_value_ignored()
{
    // returning true from the callback
    {
        std::atomic<int> fire_count{0};
        hpx::util::pool_timer const timer(
            [&fire_count]() -> bool {
                ++fire_count;
                return true;
            },
            []() {}, "return_true", true);

        HPX_TEST(timer.start(std::chrono::milliseconds(5)));
        HPX_TEST(wait_for_atomic(
            [&fire_count]() { return fire_count.load() >= 1; }));
        HPX_TEST_EQ(fire_count.load(), 1);
    }

    // returning false from the callback
    {
        std::atomic<int> fire_count{0};
        hpx::util::pool_timer const timer(
            [&fire_count]() -> bool {
                ++fire_count;
                return false;
            },
            []() {}, "return_false", true);

        HPX_TEST(timer.start(std::chrono::milliseconds(5)));
        HPX_TEST(wait_for_atomic(
            [&fire_count]() { return fire_count.load() >= 1; }));
        HPX_TEST_EQ(fire_count.load(), 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_destructor_terminates()
{
    std::atomic<int> term_count{0};

    {
        hpx::util::pool_timer const timer([]() -> bool { return false; },
            [&term_count]() { ++term_count; }, "destructor_terminates", true);

        HPX_TEST(timer.start(std::chrono::seconds(30)));
        HPX_TEST(timer.is_started());
    }

    HPX_TEST_EQ(term_count.load(), 1);
}

///////////////////////////////////////////////////////////////////////////////
// Registers a pool_timer with pre_shutdown = true and lets it fall out of
// scope. The destructor terminates the timer immediately (term count 1); the
// pre-shutdown function registered by start() keeps the underlying shared state
// alive and will invoke terminate() a second time when hpx::finalize() runs the
// registered pre-shutdown functions. The guard in pool_timer::terminate() must
// prevent the termination callback from firing again. This is verified right
// after the call to hpx::finalize() in hpx_main(), see below.
void test_double_terminate_guard()
{
    hpx::util::pool_timer const timer([]() -> bool { return false; },
        []() { ++g_double_terminate_count; }, "double_terminate", true);

    HPX_TEST(timer.start(std::chrono::seconds(30)));
}

///////////////////////////////////////////////////////////////////////////////
void test_multiple_independent_timers()
{
    constexpr int num_timers = 5;

    std::vector<std::unique_ptr<std::atomic<int>>> fire_counts;
    std::vector<std::unique_ptr<hpx::util::pool_timer>> timers;

    for (int i = 0; i != num_timers; ++i)
    {
        fire_counts.push_back(std::make_unique<std::atomic<int>>(0));
    }

    for (int i = 0; i != num_timers; ++i)
    {
        auto* counter = fire_counts[i].get();
        timers.push_back(std::make_unique<hpx::util::pool_timer>(
            [counter]() -> bool {
                ++(*counter);
                return false;
            },
            []() {}, "multi_timer", true));
    }

    for (auto const& timer : timers)
    {
        HPX_TEST(timer->start(std::chrono::milliseconds(10)));
    }

    for (int i = 0; i != num_timers; ++i)
    {
        auto* counter = fire_counts[i].get();
        HPX_TEST(wait_for_atomic([counter]() { return counter->load() >= 1; }));
    }

    for (int i = 0; i != num_timers; ++i)
    {
        HPX_TEST_EQ(fire_counts[i]->load(), 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_race_stop_before_fire()
{
    constexpr int num_iterations = 10;

    for (int i = 0; i != num_iterations; ++i)
    {
        std::atomic<int> fire_count{0};
        hpx::util::pool_timer timer(
            [&fire_count]() -> bool {
                ++fire_count;
                return false;
            },
            []() {}, "race_stop_before_fire", true);

        HPX_TEST(timer.start(std::chrono::seconds(2)));

        hpx::thread stopper([&timer]() { timer.stop(); });
        stopper.join();

        // give the timer pool time to run the handler, if it were ever going
        // to; it must not have fired since stop() cancelled it before it
        // expired
        hpx::this_thread::sleep_for(std::chrono::milliseconds(150));
        HPX_TEST_EQ(fire_count.load(), 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_concurrent_start_stop()
{
    hpx::util::pool_timer const timer(
        []() -> bool { return false; }, []() {}, "concurrent_start_stop", true);

    std::atomic<bool> stop_requested{false};
    std::atomic<int> start_success{0};
    std::atomic<int> stop_success{0};

    constexpr int num_threads = 4;
    std::vector<hpx::thread> threads;
    threads.reserve(num_threads);

    for (int i = 0; i != num_threads; ++i)
    {
        threads.emplace_back([&]() {
            while (!stop_requested.load(std::memory_order_acquire))
            {
                if (timer.start(std::chrono::milliseconds(50)))
                {
                    start_success.fetch_add(1, std::memory_order_relaxed);
                }
                if (timer.stop())
                {
                    stop_success.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop_requested.store(true, std::memory_order_release);

    for (auto& t : threads)
    {
        t.join();
    }

    // make sure the timer is stopped for good, no crashes/UB happened above,
    // and both start() and stop() succeeded at least once
    timer.stop();

    HPX_TEST(start_success.load() > 0);
    HPX_TEST(stop_success.load() > 0);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_default_construction();
    test_construction_with_callbacks(/*pre_shutdown=*/true);
    test_construction_with_callbacks(/*pre_shutdown=*/false);
    test_construction_empty_description();

    test_start_stop_basic();
    test_stop_without_start();

    test_timer_firing();
    test_repeated_start_fire_cycles();
    test_callback_return_value_ignored();

    test_destructor_terminates();
    test_double_terminate_guard();
    HPX_TEST_EQ(g_double_terminate_count.load(), 1);

    test_multiple_independent_timers();

    test_race_stop_before_fire();
    test_concurrent_start_stop();

    // hpx::finalize() causes for all registered pre-shutdown and shutdown
    // functions (on all localities) to be invoked; this includes the
    // pre-shutdown function registered by test_double_terminate_guard(), which
    // re-invokes terminate() on the still-alive detail::pool_timer object.
    return hpx::finalize();
}

int main(int const argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);

    // Check that shutting down the runtime did not invoke the termination
    // callback again.
    HPX_TEST_EQ(g_double_terminate_count.load(), 1);

    return hpx::util::report_errors();
}
