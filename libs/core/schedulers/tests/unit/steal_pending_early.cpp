//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Idle workers of the local priority queue scheduler steal pending tasks only
// from workers that hold at least
// hpx.thread_queue.min_tasks_to_steal_pending_early of them. A thief that finds
// a worker's queue short but not empty often enough in a row (its owner
// doesn't get to the tasks) ignores the minimum for that worker. In fast idle
// mode, the minimum doesn't apply.
//
// Worker 0 is kept busy while tasks are put into its pending queue, the test
// checks that other workers steal them in the meantime, a few of them as well
// as many (--mode=default or --mode=fast-idle).

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>

// the default of hpx.thread_queue.min_tasks_to_steal_pending_early
constexpr std::ptrdiff_t min_tasks = 4;
constexpr std::ptrdiff_t few_tasks = min_tasks - 1;
constexpr std::ptrdiff_t many_tasks = 8 * min_tasks;

constexpr std::chrono::seconds timeout(10);

struct shared_state
{
    explicit shared_state(std::ptrdiff_t num_tasks)
      : done(num_tasks + 1)
    {
    }

    std::atomic<bool> busy_started{false};
    std::atomic<bool> release{false};
    std::atomic<std::ptrdiff_t> stolen{0};
    hpx::latch done;
};

// Keep worker 0 busy, put num_tasks tasks into its pending queue, and return
// how many of them were run by other workers while worker 0 was busy. Worker 0
// is released as soon as expected_stolen tasks have been stolen, or after
// wait_time. Has to run on a worker other than worker 0.
std::ptrdiff_t run_case_impl(std::ptrdiff_t num_tasks,
    std::ptrdiff_t expected_stolen, std::chrono::milliseconds wait_time)
{
    HPX_TEST_NEQ(hpx::get_worker_thread_num(), std::size_t(0));

    auto state = std::make_shared<shared_state>(num_tasks);

    // bound threads are never stolen, so this really keeps worker 0 busy
    // (launch::task: never run the thread inline, i.e., as a child of a
    // thread waiting for its future)
    auto const policy0_bound =
        hpx::launch::task_policy(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(0));

    hpx::future<void> busy = hpx::async(policy0_bound, [state] {
        HPX_TEST_EQ(hpx::get_worker_thread_num(), std::size_t(0));
        state->busy_started = true;
        auto const deadline = std::chrono::steady_clock::now() + 2 * timeout;
        while (!state->release.load() &&
            std::chrono::steady_clock::now() < deadline)
        {
        }
    });
    // Until worker 0 is released, the waits in this function block the OS
    // thread instead of suspending the HPX thread: a resumed thread can be
    // placed on worker 0, behind the busy thread.
    while (!state->busy_started.load())
        std::this_thread::yield();

    for (std::ptrdiff_t i = 0; i != num_tasks; ++i)
    {
        // register_thread creates the thread right away and puts it into the
        // pending queue of worker 0 (hpx::post would stage it instead)
        hpx::threads::thread_init_data data(
            hpx::threads::make_thread_function_nullary([state] {
                if (!state->release.load() && hpx::get_worker_thread_num() != 0)
                    ++state->stolen;
                state->done.count_down(1);
            }),
            "steal_pending_early", hpx::threads::thread_priority::normal,
            hpx::threads::thread_schedule_hint(0));
        hpx::threads::register_thread(data);
    }

    auto const deadline = std::chrono::steady_clock::now() + wait_time;
    while (state->stolen.load() < expected_stolen &&
        std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::ptrdiff_t const result = state->stolen.load();
    state->release = true;

    state->done.arrive_and_wait();
    busy.get();
    return result;
}

std::ptrdiff_t run_case(std::ptrdiff_t num_tasks,
    std::ptrdiff_t expected_stolen, std::chrono::milliseconds wait_time)
{
    // run each case on a new thread bound to worker 1, wherever the caller
    // was resumed after the previous case
    auto const policy1_bound =
        hpx::launch::task_policy(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(1));
    return hpx::async(policy1_bound, [=] {
        return run_case_impl(num_tasks, expected_stolen, wait_time);
    }).get();
}

void test()
{
    // fewer tasks than the minimum: they are stolen anyway, as worker 0
    // doesn't get to them, and quickly (well before worker 0 would be released
    // after the timeout)
    auto const start = std::chrono::steady_clock::now();
    HPX_TEST_EQ(
        run_case(few_tasks, few_tasks, std::chrono::milliseconds(timeout)),
        few_tasks);
    HPX_TEST(std::chrono::steady_clock::now() - start < timeout / 2);

    // enough tasks: all of them are stolen
    HPX_TEST_EQ(
        run_case(many_tasks, many_tasks, std::chrono::milliseconds(timeout)),
        many_tasks);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::string const mode = vm["mode"].as<std::string>();
    if (mode == "fast-idle")
    {
        hpx::threads::add_scheduler_mode(
            hpx::threads::policies::scheduler_mode::fast_idle_mode);
    }

    test();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc.add_options()("mode",
        hpx::program_options::value<std::string>()->default_value("default"),
        "default | fast-idle");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc;
    // the tested behavior is specific to the local priority queue scheduler,
    // make sure it is used regardless of --hpx:queuing
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool(
            "default", hpx::resource::scheduling_policy::local_priority_fifo);
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
