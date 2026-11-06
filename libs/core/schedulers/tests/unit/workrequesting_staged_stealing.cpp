//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Work-requesting schedulers must carry the staged-stealing delay and early
// task-count limit in work requests, and victims must be able to convert staged
// work directly into a requesting worker's queue.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

constexpr std::ptrdiff_t num_tasks = 30;
constexpr std::size_t num_workers = 4;

struct shared_state
{
    shared_state()
      : done(num_tasks + 1)
    {
    }

    std::atomic<bool> busy_started{false};
    std::atomic<bool> release{false};
    std::atomic<std::ptrdiff_t> ran_elsewhere{0};
    hpx::latch done;
};

std::ptrdiff_t run_case()
{
    auto state = std::make_shared<shared_state>();

    hpx::execution::parallel_executor exec0(
        hpx::threads::thread_schedule_hint(0));
    auto const policy0_bound =
        hpx::launch::task_policy(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(0));

    hpx::future<void> busy = hpx::async(policy0_bound, [state] {
        HPX_TEST_EQ(hpx::get_worker_thread_num(), std::size_t(0));
        state->busy_started = true;
        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!state->release.load(std::memory_order_relaxed) &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::suspend(
                hpx::threads::thread_schedule_state::pending);
        }
    });

    while (!state->busy_started.load(std::memory_order_relaxed))
    {
        hpx::this_thread::yield();
    }

    for (std::ptrdiff_t i = 0; i != num_tasks; ++i)
    {
        hpx::post(exec0, [state] {
            if (!state->release.load(std::memory_order_relaxed) &&
                hpx::get_worker_thread_num() != 0)
            {
                ++state->ran_elsewhere;
            }
            state->done.count_down(1);
        });
    }

    auto const request_deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (state->ran_elsewhere.load(std::memory_order_relaxed) == 0 &&
        std::chrono::steady_clock::now() < request_deadline)
    {
        hpx::this_thread::yield();
    }

    std::ptrdiff_t const result = state->ran_elsewhere.load();
    state->release = true;

    state->done.arrive_and_wait();
    busy.get();
    return result;
}

void test_staged_stealing()
{
    for (int attempt = 0; attempt != 10; ++attempt)
    {
        if (run_case() != 0)
        {
            return;
        }
    }
    HPX_TEST(false);
}

int hpx_main()
{
    auto const policy1 =
        hpx::launch::task_policy(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(1));
    hpx::async(policy1, &test_staged_stealing).get();

    return hpx::local::finalize();
}

void test_scheduler(char* argv0, hpx::resource::scheduling_policy scheduler)
{
    std::vector<char*> args = {argv0};

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=" + std::to_string(num_workers),
        "hpx.max_idle_loop_count=1000000000000000",
        "hpx.thread_queue.min_tasks_to_steal_pending=1000",
        "hpx.thread_queue.min_tasks_to_steal_staged=1",
        "hpx.thread_queue.min_tasks_to_steal_staged_early=1",
        "hpx.thread_queue.idle_loop_count_steal_staged=0"};
    init_args.rp_callback = [scheduler](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default", scheduler);
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, static_cast<int>(args.size()),
                    args.data(), init_args),
        0);
}

int main(int, char* argv[])
{
    using hpx::threads::policies::detail::
        get_workrequesting_pending_steal_count;
    using hpx::threads::policies::detail::
        get_workrequesting_staged_steal_minimum;

    HPX_TEST_EQ(
        get_workrequesting_pending_steal_count(0, 0, false), std::size_t(0));
    HPX_TEST_EQ(
        get_workrequesting_pending_steal_count(1, 0, false), std::size_t(1));
    HPX_TEST_EQ(
        get_workrequesting_pending_steal_count(1, 0, true), std::size_t(0));
    HPX_TEST_EQ(
        get_workrequesting_pending_steal_count(4, 2, true), std::size_t(2));
    HPX_TEST_EQ(
        get_workrequesting_pending_steal_count(1, 2, false), std::size_t(0));

    HPX_TEST_EQ(get_workrequesting_staged_steal_minimum(100, 1000, 100, 10),
        std::int64_t(-1));
    HPX_TEST_EQ(get_workrequesting_staged_steal_minimum(101, 1000, 100, 10),
        std::int64_t(10));
    HPX_TEST_EQ(get_workrequesting_staged_steal_minimum(500, 1000, 100, 10),
        std::int64_t(10));
    HPX_TEST_EQ(get_workrequesting_staged_steal_minimum(501, 1000, 100, 10),
        std::int64_t(0));

    if (hpx::threads::hardware_concurrency() < num_workers)
    {
        return hpx::util::report_errors();
    }

    test_scheduler(
        argv[0], hpx::resource::scheduling_policy::local_workrequesting_fifo);
    test_scheduler(
        argv[0], hpx::resource::scheduling_policy::local_workrequesting_mc);

    return hpx::util::report_errors();
}
