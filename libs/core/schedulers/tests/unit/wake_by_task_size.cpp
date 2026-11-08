//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The local priority queue scheduler wakes up at most one sleeping worker for
// a new nano or small task: the worker it was placed on, if that sleeps, or,
// for a task a worker placed on itself, another one once the staged work adds
// up to hpx.thread_queue.min_work_to_wake or no idle worker spins. A task that
// round-robin placement put on the creating worker wakes up one sleeping
// worker.
//
// The test lets the workers go to sleep, then creates nano and small tasks in
// each of these ways and checks that all of them run, also while the worker
// that created them stays busy without yielding, so that only other workers
// can run them. The ctest variants change hpx.thread_queue.min_work_to_wake
// (0: wake up for each task, very large: never by the amount of work) and
// hpx.min_idle_backoff_time (0: idle workers never spin).

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
#include <utility>

constexpr std::chrono::seconds timeout(10);

// fewer small tasks than make up the default hpx.thread_queue.min_work_to_wake
// (2000 ns, a small task stands for 100 ns), and more
constexpr std::size_t few_tasks = 3;
constexpr std::size_t many_tasks = 100;

// let the idle workers go to sleep (hpx.max_idle_loop_count is lowered in
// main)
void let_workers_sleep()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
}

template <typename F>
void post_to(std::size_t const worker, F&& f)
{
    hpx::execution::parallel_executor exec(
        hpx::threads::thread_schedule_hint(static_cast<std::int16_t>(worker)));
    hpx::post(exec, std::forward<F>(f));
}

// Tasks placed on other workers wake up the one they were placed on.
void test_placed_on_other_workers(std::size_t const num_tasks)
{
    let_workers_sleep();

    std::size_t const self = hpx::get_worker_thread_num();
    std::size_t const num_workers = hpx::get_os_thread_count();

    hpx::latch done(static_cast<std::ptrdiff_t>(2 * num_tasks + 1));
    for (std::size_t i = 0; i != num_tasks; ++i)
    {
        std::size_t const target =
            (self + 1 + i % (num_workers - 1)) % num_workers;
        post_to(
            target, hpx::threads::nano_task([&done] { done.count_down(1); }));
        post_to(
            target, hpx::threads::small_task([&done] { done.count_down(1); }));
    }
    done.arrive_and_wait();
}

// Runs the tasks created by create while the calling worker stays busy and
// doesn't yield: other workers have to run all of them in time.
template <typename Create>
void run_while_busy(std::size_t const num_tasks, Create&& create)
{
    let_workers_sleep();

    auto ran = std::make_shared<std::atomic<std::size_t>>(0);
    for (std::size_t i = 0; i != num_tasks; ++i)
    {
        create([ran] { ++*ran; });
    }

    auto const deadline = std::chrono::steady_clock::now() + timeout;
    while (
        ran->load() != num_tasks && std::chrono::steady_clock::now() < deadline)
    {
    }
    HPX_TEST_EQ(ran->load(), num_tasks);
}

// Tasks the calling worker places on itself.
void test_placed_on_self_while_busy(std::size_t const num_tasks)
{
    // the calling thread may resume on another worker after the sleep in
    // run_while_busy, so the worker is looked up when the tasks are created
    run_while_busy(num_tasks, [](auto&& f) {
        post_to(hpx::get_worker_thread_num(),
            hpx::threads::small_task(std::forward<decltype(f)>(f)));
    });
    run_while_busy(num_tasks, [](auto&& f) {
        post_to(hpx::get_worker_thread_num(),
            hpx::threads::nano_task(std::forward<decltype(f)>(f)));
    });
}

// Tasks placed round-robin, some of them on the calling worker.
void test_round_robin_while_busy(std::size_t const num_tasks)
{
    run_while_busy(num_tasks, [](auto&& f) {
        hpx::post(hpx::threads::small_task(std::forward<decltype(f)>(f)));
    });
}

int test()
{
    for (std::size_t const num_tasks : {few_tasks, many_tasks})
    {
        test_placed_on_other_workers(num_tasks);
        test_placed_on_self_while_busy(num_tasks);
        test_round_robin_while_busy(num_tasks);
    }
    return 0;
}

int hpx_main(hpx::program_options::variables_map&)
{
    // run the test from a worker other than the first one
    auto const policy1 =
        hpx::launch::task_policy(hpx::threads::thread_priority::default_,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(1));
    hpx::async(policy1, [] { return test(); }).get();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init_params init_args;

    // idle workers go to sleep after a few hundred microseconds
    init_args.cfg = {"hpx.max_idle_loop_count=1000"};

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
