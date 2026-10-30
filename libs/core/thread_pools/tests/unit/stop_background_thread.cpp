//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that hpx::threads::detail::stop_background_thread() does
// not drop a background thread that is still in the middle of its work, either
// because it yielded or because it was suspended. The work has to complete
// and the scheduler's background thread count has to be restored in both
// cases.

#include <hpx/init.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

namespace {

    std::atomic<bool> entered(false);
    std::atomic<bool> release(false);
    std::atomic<bool> finished(false);
    hpx::threads::thread_id_type background_id;

    void reset_flags()
    {
        entered = false;
        release = false;
        finished = false;
        background_id = hpx::threads::invalid_thread_id;
    }

    // keep yielding until the test releases the background work
    bool yielding_background_work()
    {
        entered = true;
        while (!release)
        {
            hpx::execution_base::this_thread::yield("yielding_work");
        }
        finished = true;
        return true;
    }

    // yield once, then suspend until the test resumes the background work
    bool suspending_background_work()
    {
        background_id = hpx::threads::get_self_id();
        entered = true;
        hpx::execution_base::this_thread::yield("suspending_work");
        hpx::this_thread::suspend(
            hpx::threads::thread_schedule_state::suspended);
        finished = true;
        return true;
    }

    // drive the background thread the same way the scheduling loop does: run
    // it once while it is still allowed to run and then stop it
    void run_and_stop_background_thread(
        hpx::threads::policies::scheduler_base* scheduler,
        hpx::threads::detail::scheduling_callbacks const& callbacks)
    {
        using namespace hpx::threads;

        std::int64_t executed_threads = 0;
        std::int64_t executed_thread_phases = 0;
        std::int64_t tfunc_time = 0;
        std::int64_t exec_time = 0;
        std::int64_t idle_loop_count = 0;
        std::int64_t busy_loop_count = 0;
        bool is_active = false;
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
        std::int64_t background_work_duration = 0;
        std::int64_t background_send_duration = 0;
        std::int64_t background_receive_duration = 0;
        detail::scheduling_counters counters(executed_threads,
            executed_thread_phases, tfunc_time, exec_time, idle_loop_count,
            busy_loop_count, is_active, background_work_duration,
            background_send_duration, background_receive_duration);
#else
        detail::scheduling_counters counters(executed_threads,
            executed_thread_phases, tfunc_time, exec_time, idle_loop_count,
            busy_loop_count, is_active);
#endif
        detail::background_work_exec_time bg_exec_time(counters);

        auto* context_storage =
            hpx::execution_base::this_thread::detail::get_agent_storage();

        std::int64_t const initial_count =
            scheduler->get_background_thread_count();

        std::shared_ptr<bool> running;
        std::int64_t bg_idle_loop_count = 0;
        thread_id_ref_type background_thread;
        detail::create_background_thread(background_thread, *scheduler, 0,
            callbacks, running, bg_idle_loop_count);
        HPX_TEST_EQ(
            scheduler->get_background_thread_count(), initial_count + 1);

        // the background work starts and does not finish
        thread_id_ref_type next_thrd;
        HPX_TEST(detail::call_background_thread(background_thread, next_thrd,
            *scheduler, 0, bg_exec_time, context_storage));
        HPX_TEST(entered);
        HPX_TEST(!finished);

        detail::stop_background_thread(background_thread, next_thrd, *scheduler,
            0, bg_exec_time, context_storage, running);

        HPX_TEST(!background_thread);
        HPX_TEST(!running);
        HPX_TEST_EQ(scheduler->get_background_thread_count(), initial_count);
    }

    void wait_for_finished()
    {
        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!finished && std::chrono::steady_clock::now() < deadline)
        {
            hpx::execution_base::this_thread::yield("wait_for_finished");
        }
        HPX_TEST(finished);
    }

    void test_stop_yielded_background_thread()
    {
        reset_flags();

        auto* scheduler =
            hpx::threads::get_self_id_data()->get_scheduler_base();
        std::int64_t const initial_count =
            scheduler->get_background_thread_count();

        // the background thread may still use the callbacks after this
        // function has returned
        static hpx::threads::detail::scheduling_callbacks const callbacks(
            []() -> bool { return false; },    // outer
            []() {},                           // inner
            &yielding_background_work);

        // run the background thread from a plain OS thread, like a worker's
        // scheduling loop does
        std::thread t([scheduler]() {
            run_and_stop_background_thread(scheduler, callbacks);
        });
        t.join();

        // the background thread was handed to the scheduler, so its work has
        // to complete once it may
        release = true;
        wait_for_finished();

        HPX_TEST_EQ(scheduler->get_background_thread_count(), initial_count);
    }

    void test_stop_suspended_background_thread()
    {
        reset_flags();

        auto* scheduler =
            hpx::threads::get_self_id_data()->get_scheduler_base();
        std::int64_t const initial_count =
            scheduler->get_background_thread_count();

        static hpx::threads::detail::scheduling_callbacks const callbacks(
            []() -> bool { return false; },    // outer
            []() {},                           // inner
            &suspending_background_work);

        std::thread t([scheduler]() {
            run_and_stop_background_thread(scheduler, callbacks);
        });
        t.join();

        // resuming the background thread lets its work complete
        HPX_TEST(background_id != hpx::threads::invalid_thread_id);
        hpx::threads::set_thread_state(
            background_id, hpx::threads::thread_schedule_state::pending);
        wait_for_finished();

        HPX_TEST_EQ(scheduler->get_background_thread_count(), initial_count);
    }
}    // namespace

int hpx_main()
{
    test_stop_yielded_background_thread();
    test_stop_suspended_background_thread();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init(hpx_main, argc, argv);
    return hpx::util::report_errors();
}
