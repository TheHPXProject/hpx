//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The local priority queue scheduler steals staged tasks after
// hpx.thread_queue.idle_loop_count_steal_staged empty scheduling loop
// iterations, but only from workers that hold at least
// hpx.thread_queue.min_tasks_to_steal_staged_early of them. Once the thieves
// found a worker's staged tasks there often enough in a row (its owner doesn't
// get to them), one of them steals a task regardless. Fast idle mode steals
// staged tasks right away.
//
// Worker 0 is kept busy while tasks are posted to it (and stay staged there),
// the test checks that other workers steal them in the meantime, a few of them
// as well as many (--mode=default or --mode=fast-idle).

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// the default of hpx.thread_queue.min_tasks_to_steal_staged_early
constexpr std::ptrdiff_t min_tasks = 10;
constexpr std::ptrdiff_t few_tasks = 3;
constexpr std::ptrdiff_t many_tasks = 3 * min_tasks;

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

// Keep worker 0 busy, post num_tasks (staged) tasks to it, and return how many
// of them were run by other workers while worker 0 was busy. Worker 0 is
// released as soon as expected_stolen tasks have been stolen, or after
// wait_time.
std::ptrdiff_t run_case(std::ptrdiff_t num_tasks,
    std::ptrdiff_t expected_stolen, std::chrono::milliseconds wait_time)
{
    auto state = std::make_shared<shared_state>(num_tasks);

    hpx::execution::parallel_executor exec0(
        hpx::threads::thread_schedule_hint(0));

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
    while (!state->busy_started.load())
        hpx::this_thread::yield();

    for (std::ptrdiff_t i = 0; i != num_tasks; ++i)
    {
        hpx::post(exec0, [state] {
            if (!state->release.load() && hpx::get_worker_thread_num() != 0)
                ++state->stolen;
            state->done.count_down(1);
        });
    }

    auto const deadline = std::chrono::steady_clock::now() + wait_time;
    while (state->stolen.load() < expected_stolen &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::ptrdiff_t const result = state->stolen.load();
    state->release = true;

    state->done.arrive_and_wait();
    busy.get();
    return result;
}

int test()
{
    // fewer tasks than the minimum: they are stolen anyway, as worker 0
    // doesn't get to them
    HPX_TEST_EQ(
        run_case(few_tasks, few_tasks, std::chrono::milliseconds(timeout)),
        few_tasks);

    // enough tasks: all of them are stolen
    HPX_TEST_EQ(
        run_case(many_tasks, many_tasks, std::chrono::milliseconds(timeout)),
        many_tasks);
    return 0;
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::string const mode = vm["mode"].as<std::string>();
    if (mode == "fast-idle")
    {
        hpx::threads::add_scheduler_mode(
            hpx::threads::policies::scheduler_mode::fast_idle_mode);
    }

    // run the test from a worker other than the one kept busy
    auto const policy1 =
        hpx::launch::task_policy(hpx::threads::thread_priority::default_,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(1));
    hpx::async(policy1, [] { return test(); }).get();

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
