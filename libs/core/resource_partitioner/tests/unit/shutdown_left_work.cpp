//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that work scheduled during shutdown on the queue of a
// worker that has already left its scheduling loop is still run. A task on one
// worker of a pool waits until the pool is stopping and the other worker has
// left, then schedules a normal and a bound task on the queues of that worker
// and a low priority task (which some schedulers run only on the worker that
// has left). The last worker to leave has to run all of them, otherwise the
// pool never becomes idle and the runtime waits for them forever at exit. The
// static scheduler never steals: it has to put the tasks on the queue of the
// producer's worker instead. The test runs once for each scheduler.

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {

    constexpr auto timeout = std::chrono::seconds(10);

    std::atomic<bool> producer_started(false);
    std::atomic<bool> left_work_posted(false);
    std::atomic<int> left_work_done(0);
    char const* scheduler_name = "";

    template <typename F>
    bool yield_until(F&& f)
    {
        auto const until = std::chrono::steady_clock::now() + timeout;
        while (!f())
        {
            if (std::chrono::steady_clock::now() > until)
                return false;
            hpx::this_thread::yield();
        }
        return true;
    }

    // Runs on one worker of the pool "work", worker 0 if the scheduler keeps
    // a bound task there. Once the pool is stopping, the workers no longer
    // steal, and the other worker leaves its scheduling loop as soon as its
    // own queues are empty. This task keeps its own worker in the loop until
    // then (it yields, so it is on that worker's queue), and then schedules a
    // normal and a bound task on the queues of the worker that has left and a
    // low priority task.
    // Worker 1 is the last worker of the pool, the one that converts or runs
    // the low priority tasks in some schedulers.
    void producer(hpx::threads::thread_pool_base* pool)
    {
        producer_started = true;

        // once the pool is stopping this task can't migrate anymore
        std::size_t other = 1;
        bool const other_left = yield_until([&] {
            other = 1 - hpx::get_local_worker_thread_num();
            return pool->get_state(other) == hpx::state::stopped;
        });
        HPX_TEST(other_left);
        if (!other_left)
            return;

        hpx::execution::parallel_executor exec(pool,
            hpx::threads::thread_priority::normal,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(
                static_cast<std::int16_t>(other)));
        hpx::post(exec, [] { ++left_work_done; });

        hpx::execution::parallel_executor bound_exec(pool,
            hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(
                static_cast<std::int16_t>(other)));
        hpx::post(bound_exec, [] { ++left_work_done; });

        hpx::execution::parallel_executor low_exec(
            pool, hpx::threads::thread_priority::low);
        hpx::post(low_exec, [] { ++left_work_done; });

        left_work_posted = true;
    }
}    // namespace

int hpx_main()
{
    auto* pool = &hpx::resource::get_thread_pool("work");

    hpx::execution::parallel_executor exec(pool,
        hpx::threads::thread_priority::bound,
        hpx::threads::thread_stacksize::default_,
        hpx::threads::thread_schedule_hint(0));
    hpx::post(exec, &producer, pool);

    // stop the pool only once the producer runs: a stopping worker doesn't
    // run new tasks of other workers
    HPX_TEST(yield_until([] { return producer_started.load(); }));

    {
        std::mutex mtx;
        std::unique_lock<std::mutex> l(mtx);
        pool->stop(l, false);
    }

    bool const done = yield_until([] { return left_work_done.load() == 3; });
    HPX_TEST(left_work_posted);
    HPX_TEST(done);

    if (!done)
    {
        // the task left on the queue keeps the pool busy, so the runtime
        // would never shut down
        std::cerr
            << scheduler_name
            << ": the tasks left on a stopped worker's queues were not run ("
            << left_work_done.load() << " of 3 done)\n";
        std::_Exit(hpx::util::report_errors());
    }

    return hpx::local::finalize();
}

void test_scheduler(int argc, char* argv[],
    hpx::resource::scheduling_policy const scheduler, char const* name)
{
    producer_started = false;
    left_work_posted = false;
    left_work_done = 0;
    scheduler_name = name;

    hpx::program_options::options_description desc(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc.add_options()("scheduler",
        hpx::program_options::value<std::string>()->default_value("all"),
        "run the test only for this scheduler (default: all)");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc;
    init_args.cfg = {"hpx.os_threads=3"};

    // the default pool may have to share a PU with the pool "work", see below
    init_args.rp_mode = hpx::resource::partitioner_mode::allow_oversubscription;
    init_args.rp_callback = [scheduler](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("work", scheduler);

        // two workers for the pool "work", the rest stays in the default pool
        std::size_t threads_added = 0;
        hpx::resource::pu const* first_pu = nullptr;
        for (hpx::resource::numa_domain const& d : rp.numa_domains())
        {
            for (hpx::resource::core const& c : d.cores())
            {
                for (hpx::resource::pu const& p : c.pus())
                {
                    if (threads_added < 2)
                    {
                        rp.add_resource(p, "work");
                        if (first_pu == nullptr)
                            first_pu = &p;
                        ++threads_added;
                    }
                }
            }
        }

        // with only two threads (the tests may be limited to two threads per
        // locality, see HPX_WITH_TESTS_MAX_THREADS_PER_LOCALITY) no PU is
        // left for the default pool, let it share the first PU of "work"
        if (rp.get_number_requested_threads() < 3 && first_pu != nullptr)
        {
            rp.add_resource(*first_pu, "default");
        }
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
}

int main(int argc, char* argv[])
{
    using hpx::resource::scheduling_policy;
    std::vector<std::pair<scheduling_policy, char const*>> const schedulers = {
        {scheduling_policy::local, "local"},
        {scheduling_policy::local_priority_fifo, "local_priority_fifo"},
        {scheduling_policy::local_priority_fifo_double,
            "local_priority_fifo_double"},
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        {scheduling_policy::local_priority_lifo, "local_priority_lifo"},
        {scheduling_policy::abp_priority_fifo, "abp_priority_fifo"},
        {scheduling_policy::abp_priority_lifo, "abp_priority_lifo"},
#endif
        {scheduling_policy::static_, "static"},
        {scheduling_policy::static_priority, "static_priority"},
        {scheduling_policy::shared_priority, "shared_priority"},
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        {scheduling_policy::local_workrequesting_fifo,
            "local_workrequesting_fifo"},
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        {scheduling_policy::local_workrequesting_lifo,
            "local_workrequesting_lifo"},
#endif
        {scheduling_policy::local_workrequesting_mc, "local_workrequesting_mc"},
#endif
    };

    // --scheduler=<name> runs only that scheduler
    std::string only;
    for (int i = 1; i < argc; ++i)
    {
        std::string const arg(argv[i]);
        if (arg.rfind("--scheduler=", 0) == 0)
            only = arg.substr(12);
    }

    bool found = false;
    for (auto const& [scheduler, name] : schedulers)
    {
        if (only.empty() || only == "all" || only == name)
        {
            test_scheduler(argc, argv, scheduler, name);
            found = true;
        }
    }

    // a scheduler that is not available in this build is an error
    HPX_TEST_MSG(found, "unknown scheduler: " + only);

    return hpx::util::report_errors();
}
