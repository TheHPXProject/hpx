//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tests for the thread exit-callback state maintained by thread_data
// (thread_data.hpp/.cpp): registration via add_thread_exit_callback(),
// execution via run_thread_exit_callbacks(), and cleanup via
// free_thread_exit_callbacks(), all serialized through the per-instance
// spinlock mtx_ guarding exit_funcs_, running_exit_funcs_ and
// ran_exit_funcs_.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// 1. A thread that never registers an exit callback must run and terminate
//    cleanly, exercising the has_exit_funcs_ == false skip-lock fast paths in
//    run_thread_exit_callbacks()/free_thread_exit_callbacks() without deadlock
//    or triggering the HPX_ASSERT in free_thread_exit_callbacks().
void test_no_callback_fast_path()
{
    hpx::async([]() {
        // intentionally left blank: no exit callback registered
    }).get();

    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool running = false;

    hpx::thread t([&]() {
        {
            std::scoped_lock lk(mtx);
            running = true;
            cond.notify_all();
        }
        // no exit callback registered for this thread either
    });

    {
        std::unique_lock<hpx::mutex> lk(mtx);
        while (!running)
            cond.wait(lk);
    }

    t.join();

    HPX_TEST(true);
}

////////////////////////////////////////////////////////////////////////////////
// 2. A single callback registered via add_thread_exit_callback must run exactly
//    once.
void test_single_callback_executes_once()
{
    std::atomic<int> call_count{0};

    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool signaled = false;
    std::atomic<bool> added = false;

    // disable inline execution as this may mis-identify self as the target
    // thread
    hpx::async(hpx::launch::task,
        [&call_count, &mtx, &cond, &signaled, &added]() {
            hpx::threads::thread_id_type const id = hpx::threads::get_self_id();

            // Make sure this thread doesn't run inline
            HPX_TEST(!hpx::threads::get_thread_id_data(id)->runs_as_child());

            // hpx::async(...).get() only waits for the task body; the exit callback
            // runs during thread cleanup afterward, so this can still after the
            // test for the call_count being incremented below.
            added = hpx::threads::add_thread_exit_callback(
                id, [&call_count, &mtx, &cond, &signaled]() {
                    ++call_count;

                    std::scoped_lock lk(mtx);
                    signaled = true;
                    cond.notify_all();
                });
        })
        .get();

    HPX_TEST(added.load());

    // Make sure the exit callback has run to completion (if it was successfully
    // added).
    if (added.load())
    {
        std::unique_lock<hpx::mutex> lk(mtx);
        while (!signaled)
        {
            cond.wait(lk);
        }
    }

    HPX_TEST_EQ(call_count.load(), 1);
}

///////////////////////////////////////////////////////////////////////////////
// Helper used by both the concurrent registration test and the stress test
// below: spawns a target thread, keeps it suspended (i.e. running, not yet
// terminated) while a number of racing tasks concurrently try to register an
// exit callback on it, then releases the target thread and verifies that every
// callback whose registration succeeded ran exactly once (and none that failed
// to register ever ran).
void run_concurrent_registration_round(std::size_t const num_racers)
{
    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool running = false;

    hpx::thread t([&]() {
        {
            std::scoped_lock lk(mtx);
            running = true;
            cond.notify_all();
        }

        // stay suspended (thread is still alive, not terminated) while the
        // racers register their callbacks
        hpx::this_thread::suspend(
            hpx::threads::thread_schedule_state::suspended);
    });

    {
        std::unique_lock<hpx::mutex> lk(mtx);
        while (!running)
        {
            cond.wait(lk);
        }
    }

    hpx::threads::thread_id_type const id = t.native_handle();

    std::vector<std::atomic<bool>> registered(num_racers);
    std::vector<std::atomic<bool>> executed(num_racers);
    for (std::size_t i = 0; i != num_racers; ++i)
    {
        registered[i].store(false);
        executed[i].store(false);
    }

    std::vector<hpx::future<void>> futures;
    futures.reserve(num_racers);
    for (std::size_t i = 0; i != num_racers; ++i)
    {
        futures.push_back(hpx::async([&, i]() {
            bool const added = hpx::threads::add_thread_exit_callback(
                id, [&executed, i]() { executed[i].store(true); });
            registered[i].store(added);
        }));
    }
    hpx::wait_all(futures);

    // release the target thread so it can terminate, running all successfully
    // registered exit callbacks
    hpx::threads::set_thread_state(
        id, hpx::threads::thread_schedule_state::pending);
    t.join();

    std::size_t registered_count = 0;
    for (std::size_t i = 0; i != num_racers; ++i)
    {
        // a callback ran if and only if its registration succeeded
        HPX_TEST_EQ(registered[i].load(), executed[i].load());
        if (registered[i].load())
        {
            ++registered_count;
        }
    }

    // all racers attempt registration while the target thread is still
    // suspended (not terminated), so all of them are expected to succeed
    HPX_TEST_EQ(registered_count, num_racers);
}

///////////////////////////////////////////////////////////////////////////////
// 3. Multiple threads racing to register exit callbacks on the same
//    still-running target thread: every callback whose registration returned
//    true must run exactly once.
void test_concurrent_registration()
{
    run_concurrent_registration_round(16);
}

///////////////////////////////////////////////////////////////////////////////
// 4. Registering an exit callback after the target thread has already run its
//    exit callbacks (i.e. it has terminated) must fail and the callback must
//    never be invoked.
void test_post_exit_rejection()
{
    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool running = false;

    hpx::thread t([&]() {
        std::scoped_lock lk(mtx);
        running = true;
        cond.notify_all();
        // falls through and terminates right away
    });

    {
        std::unique_lock<hpx::mutex> lk(mtx);
        while (!running)
            cond.wait(lk);
    }

    hpx::threads::thread_id_type const id = t.native_handle();

    // by the time join() returns, the thread has already run and freed
    // its exit callbacks
    t.join();

    bool executed = false;
    bool const added = hpx::threads::add_thread_exit_callback(
        id, [&executed]() { executed = true; });

    HPX_TEST(!added);
    HPX_TEST(!executed);
}

///////////////////////////////////////////////////////////////////////////////
// 5. Stress test: many rounds of spawn + concurrent racing registrations to
//    catch lost or duplicated callbacks under contention.
void test_stress_loop()
{
    constexpr int num_iterations = 50;

    for (int i = 0; i != num_iterations; ++i)
    {
        constexpr std::size_t num_racers = 8;
        run_concurrent_registration_round(num_racers);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_no_callback_fast_path();
    test_single_callback_executes_once();
    test_concurrent_registration();
    test_post_exit_rejection();
    test_stress_loop();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
