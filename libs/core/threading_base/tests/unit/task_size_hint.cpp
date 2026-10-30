//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks that the size of a function reaches the hint its thread is created
// with, on every path that creates a thread from a user function, and that a
// size in the caller's launch policy is kept unless the function has one. A
// scheduler that counts the sizes it is asked to create threads with sees it.

#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using hpx::threads::task_size;

namespace {

    std::array<std::atomic<int>, 4> created_with{};

    using base_scheduler =
        hpx::threads::policies::local_priority_queue_scheduler<>;

    struct counting_scheduler : base_scheduler
    {
        using base_scheduler::base_scheduler;

        void create_thread(hpx::threads::thread_init_data& data,
            hpx::threads::thread_id_ref_type* id, hpx::error_code& ec) override
        {
            ++created_with[static_cast<std::size_t>(
                data.schedulehint.task_size())];
            base_scheduler::create_thread(data, id, ec);
        }
    };

    // Runs f and checks how many threads it created with each known size.
    // Threads created by the runtime itself have no size, so they don't count.
    template <typename F>
    void check_sizes(F&& f, int small_, int medium, int large)
    {
        auto const count = [](task_size s) {
            return created_with[static_cast<std::size_t>(s)].load();
        };
        int const small_before = count(task_size::small_);
        int const medium_before = count(task_size::medium);
        int const large_before = count(task_size::large);

        HPX_FORWARD(F, f)();

        HPX_TEST_EQ(count(task_size::small_) - small_before, small_);
        HPX_TEST_EQ(count(task_size::medium) - medium_before, medium);
        HPX_TEST_EQ(count(task_size::large) - large_before, large);
    }

    // For paths whose number of threads depends on the workers: at least one
    // thread has the given size and none has another known size, or no
    // thread has a known size if the given size is unknown.
    template <typename F>
    void check_only_size(F&& f, task_size size)
    {
        auto const count = [](task_size s) {
            return created_with[static_cast<std::size_t>(s)].load();
        };
        task_size const known[] = {
            task_size::small_, task_size::medium, task_size::large};
        int before[3];
        for (int i = 0; i != 3; ++i)
            before[i] = count(known[i]);

        HPX_FORWARD(F, f)();

        for (int i = 0; i != 3; ++i)
        {
            int const created = count(known[i]) - before[i];
            if (known[i] == size)
                HPX_TEST_LT(0, created);
            else
                HPX_TEST_EQ(created, 0);
        }
    }

    int answer()
    {
        return 42;
    }

    hpx::launch::async_policy async_with(task_size size)
    {
        return hpx::execution::experimental::with_hint(
            hpx::launch::async, hpx::threads::thread_schedule_hint(size));
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
void test_function_sizes()
{
    using hpx::threads::large_task;
    using hpx::threads::medium_task;
    using hpx::threads::small_task;

    // hpx::post
    check_sizes(
        [] {
            hpx::latch l(2);
            hpx::post(large_task([&] { l.count_down(1); }));
            l.arrive_and_wait();
        },
        0, 0, 1);

    // hpx::async, without and with arguments (bound with deferred_call)
    check_sizes([] { HPX_TEST_EQ(hpx::async(medium_task(&answer)).get(), 42); },
        0, 1, 0);
    check_sizes(
        [] {
            HPX_TEST_EQ(
                hpx::async(small_task([](int i) { return i; }), 1).get(), 1);
        },
        1, 0, 0);
    check_sizes(
        [] {
            HPX_TEST_EQ(
                hpx::async(hpx::launch::fork, large_task(&answer)).get(), 42);
        },
        0, 0, 1);

    // an executor
    check_sizes(
        [] {
            hpx::execution::parallel_executor exec;
            hpx::latch l(2);
            hpx::parallel::execution::post(
                exec, small_task([&] { l.count_down(1); }));
            l.arrive_and_wait();
        },
        1, 0, 0);

    // future::then and dataflow
    check_sizes(
        [] {
            HPX_TEST_EQ(hpx::make_ready_future(1)
                            .then(hpx::launch::async,
                                medium_task([](hpx::future<int> f) {
                                    return f.get() + 1;
                                }))
                            .get(),
                2);
        },
        0, 1, 0);
    check_sizes(
        [] {
            HPX_TEST_EQ(
                hpx::dataflow(hpx::launch::async,
                    large_task([](hpx::future<int> f) { return f.get(); }),
                    hpx::make_ready_future(3))
                    .get(),
                3);
        },
        0, 0, 1);

    // the names in namespace hpx
    check_sizes(
        [] {
            hpx::latch l(2);
            hpx::post(hpx::large_task([&] { l.count_down(1); }));
            l.arrive_and_wait();
        },
        0, 0, 1);
    check_sizes(
        [] {
            HPX_TEST_EQ(
                hpx::async(hpx::with_task_size<hpx::task_size::small_>(&answer))
                    .get(),
                42);
        },
        1, 0, 0);

    // a function without a size creates its thread without one
    check_sizes([] { HPX_TEST_EQ(hpx::async(&answer).get(), 42); }, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
void test_policy_sizes()
{
    // the policy's size is kept for a function without a size
    check_sizes(
        [] {
            HPX_TEST_EQ(
                hpx::async(async_with(task_size::small_), &answer).get(), 42);
        },
        1, 0, 0);
    check_sizes(
        [] {
            hpx::execution::parallel_policy_executor<hpx::launch::async_policy>
                exec{async_with(task_size::medium)};
            hpx::latch l(2);
            hpx::parallel::execution::post(exec, [&] { l.count_down(1); });
            l.arrive_and_wait();
        },
        0, 1, 0);

    // and the function's size wins over it
    check_sizes(
        [] {
            HPX_TEST_EQ(hpx::async(async_with(task_size::small_),
                            hpx::threads::large_task(&answer))
                            .get(),
                42);
        },
        0, 0, 1);
}

///////////////////////////////////////////////////////////////////////////////
void test_executor_sizes()
{
    // an executor with a size overwrites the function's size
    auto const small_exec = hpx::execution::experimental::with_hint(
        hpx::execution::parallel_executor{},
        hpx::threads::thread_schedule_hint(task_size::small_));
    check_sizes(
        [&] {
            hpx::latch l(2);
            hpx::parallel::execution::post(
                small_exec, hpx::large_task([&] { l.count_down(1); }));
            l.arrive_and_wait();
        },
        1, 0, 0);
    check_sizes(
        [&] {
            HPX_TEST_EQ(
                hpx::async(small_exec, hpx::large_task(&answer)).get(), 42);
        },
        1, 0, 0);

    // without a size on the executor, the function's size stays
    check_sizes(
        [] {
            hpx::execution::parallel_executor exec;
            hpx::latch l(2);
            hpx::parallel::execution::post(
                exec, hpx::large_task([&] { l.count_down(1); }));
            l.arrive_and_wait();
        },
        0, 0, 1);
    check_sizes(
        [] {
            hpx::execution::parallel_executor exec;
            HPX_TEST_EQ(hpx::async(exec, hpx::large_task(&answer)).get(), 42);
        },
        0, 0, 1);
}

///////////////////////////////////////////////////////////////////////////////
// bulk_async_execute, bulk_sync_execute, then_execute and bulk_then_execute,
// with a large function, through Executor with and without a small size. The
// bulk functions return a value if WithResult is set.
template <typename Executor, bool WithResult>
void test_executor_paths(task_size bulk_sync_with_executor_size)
{
    namespace ex = hpx::parallel::execution;

    auto const small_exec = hpx::execution::experimental::with_hint(
        Executor{}, hpx::threads::thread_schedule_hint(task_size::small_));
    Executor const exec;
    std::vector<int> const shape(16);

    auto const bulk_f = hpx::large_task([](int i) {
        if constexpr (WithResult)
            return i;
    });
    auto const then_f = hpx::large_task([](hpx::future<void>&& f) { f.get(); });
    auto const bulk_then_f = hpx::large_task([](int i, auto&&...) {
        if constexpr (WithResult)
            return i;
    });

    // the executor's size wins on every path
    check_only_size(
        [&] {
            hpx::wait_all(ex::bulk_async_execute(small_exec, bulk_f, shape));
        },
        task_size::small_);
    check_only_size([&] { ex::bulk_sync_execute(small_exec, bulk_f, shape); },
        bulk_sync_with_executor_size);
    check_only_size(
        [&] {
            ex::then_execute(small_exec, then_f, hpx::make_ready_future())
                .get();
        },
        task_size::small_);
    check_only_size(
        [&] {
            hpx::wait_all(ex::bulk_then_execute(small_exec, bulk_then_f, shape,
                hpx::make_ready_future().share()));
        },
        task_size::small_);

    // without one, then_execute uses the function's size; these bulk paths
    // run f inside tasks of their own, so they don't
    check_only_size(
        [&] { hpx::wait_all(ex::bulk_async_execute(exec, bulk_f, shape)); },
        task_size::unknown);
    check_only_size([&] { ex::bulk_sync_execute(exec, bulk_f, shape); },
        task_size::unknown);
    check_only_size(
        [&] { ex::then_execute(exec, then_f, hpx::make_ready_future()).get(); },
        task_size::large);
    check_only_size(
        [&] {
            hpx::wait_all(ex::bulk_then_execute(
                exec, bulk_then_f, shape, hpx::make_ready_future().share()));
        },
        task_size::unknown);
}

///////////////////////////////////////////////////////////////////////////////
// Waits for what an executor returns, a future or a sender.
template <typename T>
void wait_for(T&& t)
{
    if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
        HPX_FORWARD(T, t).get();
    else
        hpx::this_thread::experimental::sync_wait(HPX_FORWARD(T, t));
}

// The scheduler based executors run the function inside a task of their
// scheduler, so a size set on them reaches that task's thread, and the
// function's size isn't seen. then_execute and bulk_then_execute go through
// continues_on, which runs inline on an HPX thread, so they create none.
template <typename Executor>
void test_scheduler_executor_paths(
    Executor const& exec, task_size size, task_size bulk_sync_size)
{
    namespace ex = hpx::parallel::execution;

    auto const f = hpx::large_task([] { return 1; });
    auto const then_f = hpx::large_task([](hpx::future<void>&& p) { p.get(); });
    auto const bulk_f = hpx::large_task([](int) {});
    auto const bulk_then_f = hpx::large_task([](int, auto&&...) {});

    check_only_size(
        [&] {
            hpx::latch l(2);
            ex::post(exec, hpx::large_task([&] { l.count_down(1); }));
            l.arrive_and_wait();
        },
        size);
    check_only_size([&] { ex::sync_execute(exec, f); }, size);
    check_only_size([&] { wait_for(ex::async_execute(exec, f)); }, size);
    check_only_size(
        [&] {
            wait_for(ex::then_execute(exec, then_f, hpx::make_ready_future()));
        },
        task_size::unknown);
    check_only_size(
        [&] { wait_for(ex::bulk_async_execute(exec, bulk_f, 16)); }, size);
    check_only_size(
        [&] { ex::bulk_sync_execute(exec, bulk_f, 16); }, bulk_sync_size);
    check_only_size(
        [&] {
            wait_for(ex::bulk_then_execute(
                exec, bulk_then_f, 16, hpx::make_ready_future().share()));
        },
        task_size::unknown);
}

template <typename Executor>
void test_scheduler_executor(Executor const& exec, task_size bulk_sync_size)
{
    test_scheduler_executor_paths(
        hpx::execution::experimental::with_hint(
            exec, hpx::threads::thread_schedule_hint(task_size::small_)),
        task_size::small_, bulk_sync_size);
    test_scheduler_executor_paths(exec, task_size::unknown, task_size::unknown);
}

// fork_join_executor runs everything on worker threads it started when it was
// created, so no call creates a thread that could carry a size.
void test_fork_join_executor()
{
    namespace ex = hpx::parallel::execution;

    hpx::execution::experimental::fork_join_executor exec;
    auto bulk_f = hpx::large_task([](int i) { return i; });

    check_only_size(
        [&] { ex::bulk_sync_execute(exec, bulk_f, 16); }, task_size::unknown);
    check_only_size([&] { ex::bulk_async_execute(exec, bulk_f, 16).get(); },
        task_size::unknown);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_function_sizes();
    test_policy_sizes();
    test_executor_sizes();

    // With one worker, parallel_executor's bulk_sync_execute runs its only
    // chunk inline, without a thread. The hierarchical executor is tested
    // with functions without a result, which it runs inside tasks of its own.
    test_executor_paths<hpx::execution::parallel_executor, true>(
        task_size::unknown);
    test_executor_paths<hpx::execution::parallel_executor_spawn_hierarchically,
        false>(task_size::small_);

    // With one worker, scheduler_executor's bulk_sync_execute runs inline.
    test_scheduler_executor(
        hpx::execution::experimental::scheduler_executor(
            hpx::execution::experimental::thread_pool_scheduler{}),
        task_size::unknown);
    test_scheduler_executor(
        hpx::execution::experimental::explicit_scheduler_executor(
            hpx::execution::experimental::thread_pool_scheduler{}),
        task_size::small_);
    test_fork_join_executor();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init_params init_args;
    init_args.rp_callback = [](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            [](hpx::threads::thread_pool_init_parameters thread_pool_init,
                hpx::threads::policies::thread_queue_init_parameters
                    thread_queue_init)
                -> std::unique_ptr<hpx::threads::thread_pool_base> {
                counting_scheduler::init_parameter_type init(
                    thread_pool_init.num_threads_,
                    thread_pool_init.affinity_data_, std::size_t(-1),
                    thread_queue_init);
                auto scheduler = std::make_unique<counting_scheduler>(init);

                return std::make_unique<hpx::threads::detail::
                        scheduled_thread_pool<counting_scheduler>>(
                    std::move(scheduler), thread_pool_init);
            });
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
