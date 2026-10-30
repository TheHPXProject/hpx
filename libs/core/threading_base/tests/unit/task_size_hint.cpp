//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks that the size of a function reaches the hint its thread is created
// with, on every path that creates a thread for a single call of a user
// function, and that a size in a launch policy or an executor has no effect.
// A scheduler that counts the sizes it is asked to create threads with sees
// it.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/numeric.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using hpx::threads::task_size;

namespace {

    std::array<std::atomic<int>, 5> created_with{};

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

    // Runs f and checks how many threads it created with each known size (none
    // with nano). Threads created by the runtime itself have no size, so they
    // don't count.
    template <typename F>
    void check_sizes(F&& f, int small_, int medium, int large)
    {
        auto const count = [](task_size s) {
            return created_with[static_cast<std::size_t>(s)].load();
        };
        int const nano_before = count(task_size::nano);
        int const small_before = count(task_size::small_);
        int const medium_before = count(task_size::medium);
        int const large_before = count(task_size::large);

        HPX_FORWARD(F, f)();

        HPX_TEST_EQ(count(task_size::nano) - nano_before, 0);
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
        task_size const known[] = {task_size::nano, task_size::small_,
            task_size::medium, task_size::large};
        int before[4];
        for (int i = 0; i != 4; ++i)
            before[i] = count(known[i]);

        HPX_FORWARD(F, f)();

        for (int i = 0; i != 4; ++i)
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

    // a type erased function has the size of the function stored in it
    check_sizes(
        [] {
            hpx::move_only_function<int()> f = hpx::large_task(&answer);
            HPX_TEST_EQ(hpx::async(std::move(f)).get(), 42);
        },
        0, 0, 1);
    check_sizes(
        [] {
            hpx::function<int(int)> const f =
                hpx::small_task([](int i) { return i; });
            HPX_TEST_EQ(hpx::async(f, 1).get(), 1);
        },
        1, 0, 0);

    // hpx::thread
    check_sizes(
        [] {
            hpx::thread t(hpx::medium_task([] {}));
            t.join();
        },
        0, 1, 0);
    check_sizes(
        [] {
            hpx::thread t([] {});
            t.join();
        },
        0, 0, 0);

    // run_as_hpx_thread, which has to be called from an OS thread
    check_sizes(
        [] {
            HPX_TEST_EQ(hpx::run_as_os_thread([] {
                return hpx::run_as_hpx_thread(hpx::large_task(&answer));
            }).get(),
                42);
        },
        0, 0, 1);

    // a function without a size creates its thread without one
    check_sizes([] { HPX_TEST_EQ(hpx::async(&answer).get(), 42); }, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
void test_policy_sizes()
{
    // a size in the launch policy has no effect
    check_sizes(
        [] {
            HPX_TEST_EQ(
                hpx::async(async_with(task_size::small_), &answer).get(), 42);
        },
        0, 0, 0);
    check_sizes(
        [] {
            hpx::execution::parallel_policy_executor<hpx::launch::async_policy>
                exec{async_with(task_size::medium)};
            hpx::latch l(2);
            hpx::parallel::execution::post(exec, [&] { l.count_down(1); });
            l.arrive_and_wait();
        },
        0, 0, 0);

    // the function's size is used
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
// A size on an executor has no effect: f checks exec without one and with a
// small one, and expects the same threads in both cases.
template <typename Executor, typename F>
void with_and_without_size(Executor const& exec, F&& f)
{
    f(exec);
    f(hpx::execution::experimental::with_hint(
        exec, hpx::threads::thread_schedule_hint(task_size::small_)));
}

void test_executor_sizes()
{
    with_and_without_size(
        hpx::execution::parallel_executor{}, [](auto const& exec) {
            check_sizes(
                [&] {
                    hpx::latch l(2);
                    hpx::parallel::execution::post(
                        exec, hpx::large_task([&] { l.count_down(1); }));
                    l.arrive_and_wait();
                },
                0, 0, 1);
            check_sizes(
                [&] {
                    HPX_TEST_EQ(
                        hpx::async(exec, hpx::large_task(&answer)).get(), 42);
                },
                0, 0, 1);
            check_sizes(
                [&] { HPX_TEST_EQ(hpx::async(exec, &answer).get(), 42); }, 0, 0,
                0);
        });
}

///////////////////////////////////////////////////////////////////////////////
// bulk_async_execute, bulk_sync_execute, then_execute and bulk_then_execute,
// with a large function over 16 elements. The bulk functions return a value if
// WithResult is set. Each bulk call is expected to create small_ small and
// large large threads, bulk_sync_execute sync_large large threads.
template <typename Executor, bool WithResult>
void test_executor_paths(int small_, int large, int sync_large)
{
    namespace ex = hpx::parallel::execution;

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

    with_and_without_size(Executor{}, [&](auto const& exec) {
        check_sizes(
            [&] { hpx::wait_all(ex::bulk_async_execute(exec, bulk_f, shape)); },
            small_, 0, large);
        check_sizes([&] { ex::bulk_sync_execute(exec, bulk_f, shape); }, small_,
            0, sync_large);
        check_sizes(
            [&] {
                ex::then_execute(exec, then_f, hpx::make_ready_future()).get();
            },
            0, 0, 1);
        check_sizes(
            [&] {
                hpx::wait_all(ex::bulk_then_execute(exec, bulk_then_f, shape,
                    hpx::make_ready_future().share()));
            },
            small_, 0, large);
    });
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

// The scheduler based executors run the function inside a task their
// scheduler creates before it sees the function, so those threads have no
// size. then_execute and bulk_then_execute go through continues_on, which
// runs inline on an HPX thread, so they create none. Only the worker tasks of
// bulk_async_execute are expected to have a size, bulk_size, derived from the
// size of the function.
template <typename Executor>
void test_scheduler_executor(Executor const& base, task_size bulk_size)
{
    namespace ex = hpx::parallel::execution;

    auto const f = hpx::large_task([] { return 1; });
    auto const then_f = hpx::large_task([](hpx::future<void>&& p) { p.get(); });
    auto const bulk_f = hpx::large_task([](int) {});
    auto const bulk_then_f = hpx::large_task([](int, auto&&...) {});

    with_and_without_size(base, [&](auto const& exec) {
        check_only_size(
            [&] {
                hpx::latch l(2);
                ex::post(exec, hpx::large_task([&] { l.count_down(1); }));
                l.arrive_and_wait();
            },
            task_size::unknown);
        check_only_size([&] { ex::sync_execute(exec, f); }, task_size::unknown);
        check_only_size(
            [&] { wait_for(ex::async_execute(exec, f)); }, task_size::unknown);
        check_only_size(
            [&] {
                wait_for(
                    ex::then_execute(exec, then_f, hpx::make_ready_future()));
            },
            task_size::unknown);
        check_only_size(
            [&] { wait_for(ex::bulk_async_execute(exec, bulk_f, 16)); },
            bulk_size);
        check_only_size([&] { ex::bulk_sync_execute(exec, bulk_f, 16); },
            task_size::unknown);
        check_only_size(
            [&] {
                wait_for(ex::bulk_then_execute(
                    exec, bulk_then_f, 16, hpx::make_ready_future().share()));
            },
            task_size::unknown);
    });
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

// The worker tasks of reduce, transform_reduce and merge have the size of the
// reduction, the transformation and the comparison. The task policy keeps the
// partitioners from running the only chunk inline.
void test_algorithm_sizes()
{
    auto const policy = hpx::execution::par(hpx::execution::task);
    std::vector<int> v(16, 1);
    std::vector<int> merged(32);

    auto const plus = [](int a, int b) { return a + b; };
    auto const identity = [](int i) { return i; };
    auto const less = [](int a, int b) { return a < b; };

    check_only_size(
        [&] {
            hpx::reduce(policy, v.begin(), v.end(), 0, hpx::large_task(plus))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::transform_reduce(
                policy, v.begin(), v.end(), 0, plus, hpx::large_task(identity))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::transform_reduce(policy, v.begin(), v.end(), v.begin(), 0,
                plus, hpx::large_task(plus))
                .get();
        },
        task_size::large);
    check_only_size(
        [&] {
            hpx::merge(policy, v.begin(), v.end(), v.begin(), v.end(),
                merged.begin(), hpx::large_task(less))
                .get();
        },
        task_size::large);

    // without a size, the worker tasks have none
    check_only_size(
        [&] { hpx::reduce(policy, v.begin(), v.end(), 0, plus).get(); },
        task_size::unknown);
    check_only_size(
        [&] {
            hpx::merge(policy, v.begin(), v.end(), v.begin(), v.end(),
                merged.begin(), less)
                .get();
        },
        task_size::unknown);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_function_sizes();
    test_policy_sizes();
    test_executor_sizes();

    // With one worker, parallel_executor runs the bulk operations in one
    // task, which calls the large function 16 times and so is large, and runs
    // bulk_sync_execute inline.
    test_executor_paths<hpx::execution::parallel_executor, true>(0, 1, 0);
    test_executor_paths<hpx::execution::parallel_executor, false>(0, 1, 0);

    // Hierarchical spawning creates a task for each element, which has the
    // function's size, from a small task. With one worker and 16 elements, a
    // function with a result runs in 16 tasks; without one, the last element
    // runs inline in the small task.
    test_executor_paths<hpx::execution::parallel_executor_spawn_hierarchically,
        true>(1, 16, 16);
    test_executor_paths<hpx::execution::parallel_executor_spawn_hierarchically,
        false>(1, 15, 15);

    // scheduler_executor runs its bulk operations through the same tasks as
    // parallel_executor. explicit_scheduler_executor runs them through the
    // bulk sender of its scheduler, which with one worker runs the work on
    // the thread that receives the predecessor's value.
    test_scheduler_executor(
        hpx::execution::experimental::scheduler_executor(
            hpx::execution::experimental::thread_pool_scheduler{}),
        task_size::large);
    test_scheduler_executor(
        hpx::execution::experimental::explicit_scheduler_executor(
            hpx::execution::experimental::thread_pool_scheduler{}),
        task_size::unknown);
    test_fork_join_executor();
    test_algorithm_sizes();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // The numbers of tasks the executor paths create depend on the number of
    // workers, so the test runs on one.
    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=1"};
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
