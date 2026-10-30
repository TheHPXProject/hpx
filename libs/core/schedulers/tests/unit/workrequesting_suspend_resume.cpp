//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Suspend and resume workers of a work-requesting scheduler while they are
// stealing from a loaded worker (with steal requests outstanding). All tasks
// must complete (a hang shows up as a test timeout), and once resumed, the
// workers must be able to steal again: their task channels must have been
// reopened and their outstanding steal request accounted for.

#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pool_util.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

constexpr std::size_t num_workers = 4;
constexpr std::size_t tasks_per_round = 400;
constexpr std::uint64_t task_duration_ns = 10000;
constexpr int num_rounds = 40;

std::atomic<std::size_t> num_completed(0);
std::array<std::atomic<std::size_t>, num_workers> ran_on{};

void busy_task()
{
    auto const start = hpx::chrono::high_resolution_clock::now();
    while (hpx::chrono::high_resolution_clock::now() - start < task_duration_ns)
    {
    }

    std::size_t const worker = hpx::get_worker_thread_num();
    if (worker < num_workers)
    {
        ++ran_on[worker];
    }
    ++num_completed;
}

std::vector<hpx::future<void>> spawn_on_worker_0(std::size_t count)
{
    // with run_as_child the waiting parent can run these tasks itself, so
    // there would be nothing left for the other workers to steal; revisit
    // once a worker running children inline answers steal requests (#7789)
    hpx::threads::thread_schedule_hint hint(0);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
    hpx::execution::parallel_executor exec(hint);

    std::vector<hpx::future<void>> fs;
    fs.reserve(count);
    for (std::size_t i = 0; i != count; ++i)
    {
        fs.push_back(hpx::async(exec, &busy_task));
    }
    return fs;
}

int hpx_main()
{
    hpx::threads::thread_pool_base& tp =
        hpx::resource::get_thread_pool("default");

    // suspend/resume the workers that steal from worker 0 while there is
    // work to steal
    std::size_t total = 0;
    for (int round = 0; round != num_rounds; ++round)
    {
        auto fs = spawn_on_worker_0(tasks_per_round);
        total += tasks_per_round;

        std::size_t const victim = 1 + static_cast<std::size_t>(round) % 3;
        hpx::threads::suspend_processing_unit(tp, victim).get();
        hpx::threads::resume_processing_unit(tp, victim).get();

        hpx::wait_all(fs);
    }
    HPX_TEST_EQ(num_completed.load(), total);

    // after resuming, every worker must be able to steal again
    bool all_stole = false;
    for (int attempt = 0; attempt != 10 && !all_stole; ++attempt)
    {
        for (auto& count : ran_on)
        {
            count = 0;
        }

        auto fs = spawn_on_worker_0(2000);
        hpx::wait_all(fs);

        all_stole = true;
        for (std::size_t w = 1; w != num_workers; ++w)
        {
            all_stole = all_stole && ran_on[w] != 0;
        }
    }
    HPX_TEST(all_stole);

    return hpx::local::finalize();
}

void test_scheduler(char* argv0, hpx::resource::scheduling_policy scheduler)
{
    num_completed = 0;

    std::vector<char*> av = {argv0};

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=" + std::to_string(num_workers)};
    init_args.rp_callback = [scheduler](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default", scheduler,
            hpx::threads::policies::scheduler_mode::default_ |
                hpx::threads::policies::scheduler_mode::enable_elasticity);
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, static_cast<int>(av.size()),
                    av.data(), init_args),
        0);
}

int main(int, char* argv[])
{
    if (hpx::threads::hardware_concurrency() < num_workers)
    {
        return 0;    // not enough cores to run this test meaningfully
    }

    test_scheduler(
        argv[0], hpx::resource::scheduling_policy::local_workrequesting_fifo);
    test_scheduler(
        argv[0], hpx::resource::scheduling_policy::local_workrequesting_mc);

    return hpx::util::report_errors();
}
