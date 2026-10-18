//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The work-requesting schedulers must hand work to idle workers: all tasks
// below are placed on worker 0, so any task running elsewhere was stolen.
// Without HPX_HAVE_THREAD_STEALING_COUNTS a victim used to answer every steal
// request with zero tasks, so nothing was ever stolen.

#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

constexpr std::size_t num_tasks = 2000;
constexpr std::uint64_t task_duration_ns = 20000;

std::atomic<std::size_t> num_stolen(0);
hpx::threads::thread_priority task_priority =
    hpx::threads::thread_priority::normal;

// the worker hpx_main runs on, which isn't necessarily worker 0, as another
// worker may have stolen hpx_main before it started
std::size_t home_worker = 0;

void busy_task()
{
    HPX_TEST_EQ(hpx::this_thread::get_priority(), task_priority);

    auto const start = hpx::chrono::high_resolution_clock::now();
    while (hpx::chrono::high_resolution_clock::now() - start < task_duration_ns)
    {
    }

    if (hpx::get_worker_thread_num() != home_worker)
    {
        ++num_stolen;
    }
}

int hpx_main()
{
    home_worker = hpx::get_worker_thread_num();
    auto const exec = hpx::execution::experimental::with_priority(
        hpx::execution::parallel_executor(
            hpx::threads::thread_schedule_hint(
                static_cast<std::int16_t>(home_worker))),
        task_priority);

    std::vector<hpx::future<void>> futures;
    futures.reserve(num_tasks);
    for (std::size_t i = 0; i != num_tasks; ++i)
    {
        futures.push_back(hpx::async(exec, &busy_task));
    }
    hpx::wait_all(futures);

    // a lower bound only, to stay robust on slow or oversubscribed machines
    HPX_TEST(num_stolen > 0);

    return hpx::local::finalize();
}

template <typename Predicate>
bool wait_until(Predicate&& predicate)
{
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!predicate() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    return predicate();
}

bool test_high_priority_queue_placement(char* argv0, char const* scheduler)
{
    std::string const queuing = std::string("--hpx:queuing=") + scheduler;
    std::vector<char*> av = {argv0, const_cast<char*>(queuing.c_str())};

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=2"};
    HPX_TEST(hpx::local::start(
        nullptr, static_cast<int>(av.size()), av.data(), init_args));

    std::atomic<bool> victim_gate_started{false};
    std::atomic<bool> release_victim_gate{false};
    std::atomic<bool> thief_gate_started{false};
    std::atomic<bool> release_thief_gate{false};
    std::atomic<bool> victim_high_started{false};
    std::atomic<bool> release_victim_high{false};
    std::atomic<bool> normal_ran{false};
    std::atomic<bool> stolen_high_ran_first{false};
    std::atomic<std::size_t> stolen_high_count{0};

    auto const bound_policy = [](std::size_t worker) {
        return hpx::launch::task_policy(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(worker));
    };

    hpx::future<void> victim_gate = hpx::async(bound_policy(0), [&] {
        victim_gate_started = true;
        while (!release_victim_gate.load(std::memory_order_relaxed))
        {
            std::this_thread::yield();
        }
    });
    HPX_TEST(wait_until(
        [&] { return victim_gate_started.load(std::memory_order_relaxed); }));

    // Worker 1 is idle while worker 0 is blocked, ensuring that its steal
    // request is waiting at worker 0 before both workers are occupied.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    hpx::future<void> thief_gate = hpx::async(bound_policy(1), [&] {
        thief_gate_started = true;
        while (!release_thief_gate.load(std::memory_order_relaxed))
        {
            std::this_thread::yield();
        }
    });
    HPX_TEST(wait_until(
        [&] { return thief_gate_started.load(std::memory_order_relaxed); }));

    hpx::execution::parallel_executor normal_exec(
        hpx::threads::thread_schedule_hint(1));
    hpx::future<void> normal =
        hpx::async(normal_exec, [&] { normal_ran = true; });

    auto const high_exec = hpx::execution::experimental::with_priority(
        hpx::execution::parallel_executor(
            hpx::threads::thread_schedule_hint(0)),
        hpx::threads::thread_priority::high);

    std::vector<hpx::future<void>> high_tasks;
    high_tasks.reserve(8);
    for (int i = 0; i != 8; ++i)
    {
        high_tasks.push_back(hpx::async(high_exec, [&] {
            HPX_TEST_EQ(hpx::this_thread::get_priority(),
                hpx::threads::thread_priority::high);
            if (hpx::get_worker_thread_num() == 0)
            {
                victim_high_started = true;
                while (!release_victim_high.load(std::memory_order_relaxed))
                {
                    std::this_thread::yield();
                }
            }
            else if (!normal_ran.load(std::memory_order_relaxed))
            {
                stolen_high_ran_first = true;
                ++stolen_high_count;
            }
            else
            {
                ++stolen_high_count;
            }
        }));
    }

    release_victim_gate = true;
    bool const victim_selected_high = wait_until(
        [&] { return victim_high_started.load(std::memory_order_relaxed); });

    // The normal task was queued on worker 1 before worker 0 answered the
    // steal request. A stolen high-priority task must still run first.
    release_thief_gate = true;
    bool const task_ran = wait_until([&] {
        return stolen_high_ran_first.load(std::memory_order_relaxed) ||
            normal_ran.load(std::memory_order_relaxed);
    });

    release_victim_high = true;
    victim_gate.get();
    thief_gate.get();
    normal.get();
    hpx::wait_all(high_tasks);

    hpx::post([] { hpx::local::finalize(); });
    HPX_TEST_EQ(hpx::local::stop(), 0);

    return victim_selected_high && task_ran && stolen_high_ran_first &&
        stolen_high_count != 0;
}

int main(int, char* argv[])
{
    for (hpx::threads::thread_priority const priority :
        {hpx::threads::thread_priority::normal,
            hpx::threads::thread_priority::high})
    {
        task_priority = priority;
        for (char const* scheduler :
            {"local-workrequesting-fifo", "local-workrequesting-mc"})
        {
            num_stolen = 0;

            std::string const queuing =
                std::string("--hpx:queuing=") + scheduler;
            std::vector<char*> av = {
                argv[0], const_cast<char*>(queuing.c_str())};

            hpx::local::init_params init_args;
            init_args.cfg = {"hpx.os_threads=4"};

            HPX_TEST_EQ(hpx::local::init(hpx_main, static_cast<int>(av.size()),
                            av.data(), init_args),
                0);
        }
    }

    for (char const* scheduler :
        {"local-workrequesting-fifo", "local-workrequesting-mc"})
    {
        bool passed = false;
        for (int attempt = 0; attempt != 10 && !passed; ++attempt)
        {
            passed = test_high_priority_queue_placement(argv[0], scheduler);
        }
        HPX_TEST(passed);
    }

    return hpx::util::report_errors();
}
