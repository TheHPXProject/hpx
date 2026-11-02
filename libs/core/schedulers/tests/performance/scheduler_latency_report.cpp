//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  How quickly the scheduler gets idle or sleeping workers to new work. Each
//  timed step is a sequence of rounds, and each round waits for its work to
//  start or finish before the next one begins, so a round that stalls makes
//  its whole step longer:
//    sparse: a busy worker creates one small task every --sparse-gap us and
//            waits until another worker has started it;
//    burst:  after --burst-gap us in which all workers are idle, 16 tasks of
//            100 us each are created at once;
//    batch:  a busy worker places 4 tasks of 20 us each on its own queue
//            every --batch-gap us and waits until other workers have run
//            them;
//    loop:   a short parallel loop (about 2 us of work per iteration, 4
//            iterations per worker) after --loop-gap us in which all workers
//            are idle.
//  The steps of the sparse, batch and loop cases have 100 or more rounds, so
//  that a stall in 1% of the rounds shows up in most steps. The idle gaps of
//  the burst and loop cases suspend the calling task; on systems with a
//  coarse timer they last longer than requested. The sparse, burst and
//  batch cases need other workers to take tasks from the creating worker's
//  queue; they are skipped with a scheduler that does not steal and with
//  only one worker.

#include <hpx/config.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>

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

    std::uint64_t now_ns()
    {
        return hpx::chrono::high_resolution_clock::now();
    }

    void spin_ns(std::uint64_t const ns)
    {
        auto const start = now_ns();
        while (now_ns() - start < ns)
        {
        }
    }

    // Keep the calling worker busy until the predicate holds. After 100 ms
    // let the calling worker run other tasks, so that a step ends even if no
    // other worker picks up the work.
    template <typename Pred>
    void spin_until(Pred&& pred)
    {
        auto const deadline = now_ns() + 100'000'000;
        while (!pred())
        {
            if (now_ns() > deadline)
                hpx::this_thread::yield();
        }
    }

    void sparse(std::size_t const rounds, std::uint64_t const gap_us)
    {
        for (std::size_t i = 0; i != rounds; ++i)
        {
            spin_ns(gap_us * 1000);

            std::atomic<bool> started(false);
            hpx::post([&] { started.store(true, std::memory_order_release); });
            spin_until([&] { return started.load(std::memory_order_acquire); });
        }
    }

    void burst(std::size_t const rounds, std::uint64_t const gap_us)
    {
        constexpr std::ptrdiff_t tasks = 16;
        for (std::size_t i = 0; i != rounds; ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::microseconds(gap_us));

            hpx::latch done(tasks);
            for (std::ptrdiff_t t = 0; t != tasks; ++t)
            {
                hpx::post([&] {
                    spin_ns(100'000);
                    done.count_down(1);
                });
            }
            done.wait();
        }
    }

    void batch(std::size_t const rounds, std::uint64_t const gap_us)
    {
        constexpr std::size_t tasks = 4;
        hpx::execution::parallel_executor const exec_self(
            hpx::threads::thread_schedule_hint(
                static_cast<std::int16_t>(hpx::get_worker_thread_num())));

        for (std::size_t i = 0; i != rounds; ++i)
        {
            spin_ns(gap_us * 1000);

            std::atomic<std::size_t> left(tasks);
            for (std::size_t t = 0; t != tasks; ++t)
            {
                hpx::post(exec_self, [&] {
                    spin_ns(20'000);
                    left.fetch_sub(1, std::memory_order_release);
                });
            }
            spin_until(
                [&] { return left.load(std::memory_order_acquire) == 0; });
        }
    }

    void loop(std::size_t const rounds, std::uint64_t const gap_us)
    {
        std::size_t const n = 4 * hpx::get_num_worker_threads();
        for (std::size_t i = 0; i != rounds; ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::microseconds(gap_us));
            hpx::experimental::for_loop(hpx::execution::par, std::size_t(0), n,
                [](std::size_t) { spin_ns(2'000); });
        }
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    std::size_t const steps = vm["test_count"].as<std::size_t>();

    hpx::util::perftests_init(vm, "scheduler_latency_report");

    // let the runtime settle, all workers go idle
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (scheduler_steals())
    {
        hpx::util::perftests_report("scheduler latency - sparse", "post", steps,
            [gap = vm["sparse-gap"].as<std::uint64_t>()] { sparse(200, gap); });

        hpx::util::perftests_report("scheduler latency - burst", "post", steps,
            [gap = vm["burst-gap"].as<std::uint64_t>()] { burst(10, gap); });

        hpx::util::perftests_report("scheduler latency - batch",
            "parallel_executor", steps,
            [gap = vm["batch-gap"].as<std::uint64_t>()] { batch(100, gap); });
    }
    else
    {
        std::cerr << "the scheduler does not steal or runs only one worker, "
                     "skipping the sparse, burst and batch cases\n";
    }

    hpx::util::perftests_report("scheduler latency - loop", "par", steps,
        [gap = vm["loop-gap"].as<std::uint64_t>()] { loop(100, gap); });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("sparse-gap", value<std::uint64_t>()->default_value(500),
         "time between the tasks of the sparse case (in us)")

        ("burst-gap", value<std::uint64_t>()->default_value(20000),
         "idle time before each burst (in us)")

        ("batch-gap", value<std::uint64_t>()->default_value(2000),
         "time between the batches of the batch case (in us)")

        ("loop-gap", value<std::uint64_t>()->default_value(1000),
         "idle time before each parallel loop (in us)")

        ("test_count", value<std::size_t>()->default_value(10),
         "number of timed steps of each case");
    // clang-format on

    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
