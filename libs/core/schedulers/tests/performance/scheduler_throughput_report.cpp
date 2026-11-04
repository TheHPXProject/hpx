//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Throughput of the scheduler for many small tasks. Each timed step creates
//  --tasks tasks and waits for all of them:
//    spawn:     the tasks are placed by the default executor;
//    spawnself: the tasks are placed on the worker that creates them;
//    steal:     the tasks are placed on worker 0 and run for --delay ns each,
//               so that the other workers have to steal them.
//  The results depend on the placement of the scheduler's per-worker data in
//  memory, so run this with many executions and on many cores. The steal
//  case is skipped with a scheduler that does not steal and with only one
//  worker.

#include <hpx/config.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

///////////////////////////////////////////////////////////////////////////////
namespace {

    // The static schedulers do not steal: tasks placed on one worker stay
    // there. With only one worker there is no other worker to steal them.
    bool scheduler_steals()
    {
        return hpx::get_num_worker_threads() > 1 &&
            hpx::threads::get_self_id_data()
                ->get_scheduler_base()
                ->has_scheduler_mode(
                    hpx::threads::policies::scheduler_mode::enable_stealing,
                    hpx::get_worker_thread_num());
    }

    void burn(std::uint64_t const ns)
    {
        auto const start = hpx::chrono::high_resolution_clock::now();
        while (hpx::chrono::high_resolution_clock::now() - start < ns)
        {
        }
    }

    template <typename Executor, typename F>
    void spawn_and_wait(Executor const& exec, std::size_t const tasks, F f)
    {
        std::vector<hpx::future<void>> futures;
        futures.reserve(tasks);
        std::generate_n(std::back_inserter(futures), tasks,
            [&] { return hpx::async(exec, f); });
        hpx::wait_all(futures);
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    std::size_t const tasks = vm["tasks"].as<std::size_t>();
    std::uint64_t const delay = vm["delay"].as<std::uint64_t>();
    std::size_t const steps = vm["test_count"].as<std::size_t>();

    if (tasks == 0)
        throw std::logic_error("error: count of 0 tasks specified\n");

    hpx::util::perftests_init(vm, "scheduler_throughput_report");

    hpx::execution::parallel_executor const exec_default;
    hpx::execution::parallel_executor const exec_self(
        hpx::threads::thread_schedule_hint(
            static_cast<std::int16_t>(hpx::get_worker_thread_num())));
    hpx::execution::parallel_executor const exec_zero(
        hpx::threads::thread_schedule_hint(0));

    hpx::util::perftests_report("scheduler throughput - spawn",
        "parallel_executor", steps,
        [&] { spawn_and_wait(exec_default, tasks, [] {}); });

    hpx::util::perftests_report("scheduler throughput - spawnself",
        "parallel_executor", steps,
        [&] { spawn_and_wait(exec_self, tasks, [] {}); });

    if (scheduler_steals())
    {
        hpx::util::perftests_report("scheduler throughput - steal",
            "parallel_executor", steps,
            [&] { spawn_and_wait(exec_zero, tasks, [=] { burn(delay); }); });
    }
    else
    {
        std::cerr << "the scheduler does not steal or runs only one worker, "
                     "skipping the steal case\n";
    }

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("tasks", value<std::size_t>()->default_value(100000),
         "number of tasks created in each step")

        ("delay", value<std::uint64_t>()->default_value(1000),
         "time each task of the steal case runs (in ns)")

        ("test_count", value<std::size_t>()->default_value(10),
         "number of timed steps of each case");
    // clang-format on

    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
