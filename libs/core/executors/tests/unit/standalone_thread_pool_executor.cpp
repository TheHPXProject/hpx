//  Copyright (c)      2019 Mikael Simberg
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The parallel_executor has a constructor that takes a thread_pool_base as an
// argument and executes all its work on that thread pool. This checks that the
// usual functions of an executor work with this executor when used *without the
// HPX runtime*. This test fails if thread pools, schedulers etc. assume that
// the global runtime (configuration, thread manager, etc.) always exists.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id test(int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

template <typename Executor>
void test_sync(Executor& exec)
{
    HPX_TEST(hpx::parallel::execution::sync_execute(exec, &test, 42) ==
        hpx::this_thread::get_id());
}

template <typename Executor>
void test_async(Executor& exec)
{
    HPX_TEST(hpx::parallel::execution::async_execute(exec, &test, 42).get() !=
        hpx::this_thread::get_id());
}

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id test_f(hpx::future<void> f, int passed_through)
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

template <typename Executor>
void test_then(Executor& exec)
{
    hpx::future<void> f = hpx::make_ready_future();

    HPX_TEST(
        hpx::parallel::execution::then_execute(exec, &test_f, std::move(f), 42)
            .get() != hpx::this_thread::get_id());
}

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
decltype(auto) disable_run_as_child(Executor&& exec)
{
    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);

    return hpx::experimental::prefer(hpx::execution::experimental::with_hint,
        HPX_FORWARD(Executor, exec), hint);
}

std::atomic<int> count_inline(0);

void bulk_test(int, hpx::thread::id const& tid, int passed_through)    //-V813
{
    if (tid == hpx::this_thread::get_id())
    {
        ++count_inline;
    }
    HPX_TEST_EQ(passed_through, 42);
}

template <typename Executor>
void test_bulk_sync(Executor& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.sharing_mode(hpx::threads::thread_sharing_hint::do_not_share_function |
        hpx::threads::thread_sharing_hint::do_not_combine_tasks);
    auto no_sharing_exec = hpx::execution::to_hierarchical_spawning(
        hpx::execution::experimental::with_hint(exec, hint));

    count_inline.store(0);
    hpx::parallel::execution::bulk_sync_execute(
        no_sharing_exec, hpx::bind(&bulk_test, _1, tid, _2), v, 42);
    HPX_TEST_LTE(count_inline.load(), 1);

    count_inline.store(0);
    hpx::parallel::execution::bulk_sync_execute(
        no_sharing_exec, &bulk_test, v, tid, 42);
    HPX_TEST_LTE(count_inline.load(), 1);
}

template <typename Executor>
void test_bulk_async(Executor& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    count_inline.store(0);
    hpx::when_all(
        hpx::parallel::execution::bulk_async_execute(disable_run_as_child(exec),
            hpx::bind(&bulk_test, _1, tid, _2), v, 42))
        .get();
    HPX_TEST_LTE(count_inline.load(), 1);

    count_inline.store(0);
    hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                      disable_run_as_child(exec), &bulk_test, v, tid, 42))
        .get();
    HPX_TEST_LTE(count_inline.load(), 1);
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_f(int, hpx::shared_future<void> const& f,
    hpx::thread::id const& tid,
    int passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

template <typename Executor>
void test_bulk_then(Executor& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;

    hpx::shared_future<void> f = hpx::make_ready_future();

    hpx::parallel::execution::bulk_then_execute(
        exec, hpx::bind(&bulk_test_f, _1, _2, tid, _3), v, f, 42)
        .get();
    hpx::parallel::execution::bulk_then_execute(
        exec, &bulk_test_f, v, f, tid, 42)
        .get();
}

///////////////////////////////////////////////////////////////////////////////
void test_thread_pool_os_executor(hpx::execution::parallel_executor exec)
{
    test_sync(exec);
    test_async(exec);
    test_then(exec);
    test_bulk_sync(exec);
    test_bulk_async(exec);
    test_bulk_then(exec);
}

int main()
{
    {
        // Choose a scheduler.
        using sched_type =
            hpx::threads::policies::local_priority_queue_scheduler<>;

        // Choose all the parameters for the thread pool and scheduler.
        std::size_t const num_threads = (std::min) (std::size_t(4),
            std::size_t(hpx::threads::hardware_concurrency()));
        std::size_t const max_cores = num_threads;
        hpx::threads::policies::detail::affinity_data ad{};
        ad.init(num_threads, max_cores, 0, 1, 0, "core", "balanced", true);
        hpx::threads::policies::callback_notifier notifier{};
        hpx::threads::policies::thread_queue_init_parameters
            thread_queue_init{};
        sched_type::init_parameter_type scheduler_init(
            num_threads, ad, num_threads, thread_queue_init, "my_scheduler");
        hpx::threads::detail::network_background_callback_type
            network_callback{};
        hpx::threads::thread_pool_init_parameters thread_pool_init("my_pool", 0,
            hpx::threads::policies::scheduler_mode::default_, num_threads, 0,
            notifier, ad, network_callback, 0,
            (std::numeric_limits<std::int64_t>::max)(),
            (std::numeric_limits<std::int64_t>::max)());

        // Create the scheduler, thread pool, and associated executor.
        std::unique_ptr<sched_type> scheduler{new sched_type(scheduler_init)};
        hpx::threads::detail::scheduled_thread_pool<sched_type> pool{
            std::move(scheduler), thread_pool_init};
        hpx::execution::parallel_executor exec{&pool};

        // Run the pool.
        std::mutex m;
        std::unique_lock<std::mutex> l(m);
        pool.run(l, num_threads);

        // We can't wait for futures on the main thread, so we spawn a thread to
        // run the tests for us.
        hpx::post(exec, &test_thread_pool_os_executor, exec);

        // Stop the pool.
        pool.stop(l, true);
    }

    return hpx::util::report_errors();
}
