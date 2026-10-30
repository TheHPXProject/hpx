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
#include <vector>

constexpr std::size_t num_tasks = 2000;
constexpr std::uint64_t task_duration_ns = 20000;

std::atomic<std::size_t> num_stolen(0);
hpx::threads::thread_priority task_priority =
    hpx::threads::thread_priority::normal;
hpx::threads::thread_priority expected_task_priority =
    hpx::threads::thread_priority::normal;

void busy_task()
{
    HPX_TEST_EQ(hpx::this_thread::get_priority(), expected_task_priority);

    auto const start = hpx::chrono::high_resolution_clock::now();
    while (hpx::chrono::high_resolution_clock::now() - start < task_duration_ns)
    {
    }

    if (hpx::get_worker_thread_num() != 0)
    {
        ++num_stolen;
    }
}

int hpx_main()
{
    // with run_as_child the waiting parent can run these tasks itself, so
    // there would be nothing left for the other workers to steal; revisit
    // once a worker running children inline answers steal requests (#7789)
    hpx::threads::thread_schedule_hint hint(0);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
    auto const exec = hpx::execution::experimental::with_priority(
        hpx::execution::parallel_executor(hint), task_priority);

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

int main(int, char* argv[])
{
    for (hpx::threads::thread_priority const priority :
        {hpx::threads::thread_priority::normal,
            hpx::threads::thread_priority::high,
            hpx::threads::thread_priority::boost})
    {
        task_priority = priority;
        expected_task_priority =
            priority == hpx::threads::thread_priority::boost ?
            hpx::threads::thread_priority::normal :
            priority;
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

    return hpx::util::report_errors();
}
