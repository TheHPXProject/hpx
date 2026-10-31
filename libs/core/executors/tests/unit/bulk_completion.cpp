//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The bulk operations of parallel_executor complete once all of their chunks
// are done. A chunk that throws counts as done, and the chunks left in the
// queue of the worker that threw are skipped, while the other workers go on,
// so that several exceptions can be reported.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace ex = hpx::parallel::execution;

constexpr std::size_t num_elements = 10007;

std::vector<std::size_t> make_shape()
{
    std::vector<std::size_t> shape(num_elements);
    std::iota(shape.begin(), shape.end(), std::size_t(0));
    return shape;
}

// Runs f and returns the number of exceptions it reported, all of which are
// expected to be std::runtime_error.
template <typename F>
std::size_t count_exceptions(F&& f)
{
    try
    {
        f();
    }
    catch (hpx::exception_list const& el)
    {
        for (std::exception_ptr const& e : el)
        {
            HPX_TEST_THROW(std::rethrow_exception(e), std::runtime_error);
        }
        return el.size();
    }
    return 0;
}

// Elements for which throws returns true throw, from the ranges of several
// workers. Each worker stops at its first exception, so there is at least one
// and at most one per worker.
template <typename Throws>
void test_exceptions(Throws throws)
{
    hpx::execution::parallel_executor exec;
    std::vector<std::size_t> const shape = make_shape();
    std::size_t const num_workers = hpx::get_num_worker_threads();

    auto f_void = [&](std::size_t i) {
        if (throws(i))
            throw std::runtime_error("bulk_completion");
    };
    auto f_result = [&](std::size_t i) {
        if (throws(i))
            throw std::runtime_error("bulk_completion");
        return i;
    };

    std::size_t n =
        count_exceptions([&] { ex::bulk_sync_execute(exec, f_void, shape); });
    HPX_TEST(n >= 1 && n <= num_workers);

    n = count_exceptions(
        [&] { ex::bulk_async_execute(exec, f_void, shape).get(); });
    HPX_TEST(n >= 1 && n <= num_workers);

    n = count_exceptions([&] { ex::bulk_sync_execute(exec, f_result, shape); });
    HPX_TEST(n >= 1 && n <= num_workers);

    n = count_exceptions(
        [&] { ex::bulk_async_execute(exec, f_result, shape).get(); });
    HPX_TEST(n >= 1 && n <= num_workers);
}

// Every element throws, but the first one waits until a second one throws as
// well, so that at least two workers report an exception. While it waits it
// yields (for at most ten seconds), so that its worker can run the task of
// another worker; any other worker that runs a chunk throws as well.
void test_exceptions_from_two_workers()
{
    std::size_t const num_workers = hpx::get_num_worker_threads();
    if (num_workers < 2)
        return;

    hpx::execution::parallel_executor exec;
    std::vector<std::size_t> const shape = make_shape();
    std::atomic<std::size_t> throwing(0);

    auto f = [&](std::size_t) {
        if (++throwing == 1)
        {
            auto const deadline =
                std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (throwing.load() < 2 &&
                std::chrono::steady_clock::now() < deadline)
            {
                hpx::this_thread::yield();
            }
        }
        throw std::runtime_error("bulk_completion");
    };

    std::size_t n =
        count_exceptions([&] { ex::bulk_sync_execute(exec, f, shape); });
    HPX_TEST(n >= 2 && n <= num_workers);

    throwing = 0;
    n = count_exceptions([&] { ex::bulk_async_execute(exec, f, shape).get(); });
    HPX_TEST(n >= 2 && n <= num_workers);
}

// Without exceptions, every element runs exactly once.
void test_all_elements_run()
{
    hpx::execution::parallel_executor exec;
    std::vector<std::size_t> const shape = make_shape();

    std::vector<std::atomic<int>> calls(num_elements);
    ex::bulk_sync_execute(exec, [&](std::size_t i) { ++calls[i]; }, shape);
    ex::bulk_async_execute(
        exec, [&](std::size_t i) { ++calls[i]; }, shape)
        .get();
    for (auto const& c : calls)
    {
        HPX_TEST_EQ(c.load(), 2);
    }

    std::vector<std::size_t> const results = ex::bulk_async_execute(
        exec, [](std::size_t i) { return 2 * i; }, shape)
                                                 .get();
    HPX_TEST_EQ(results.size(), num_elements);
    for (std::size_t i = 0; i != results.size(); ++i)
    {
        HPX_TEST_EQ(results[i], 2 * i);
    }
}

// One worker is kept busy for the whole bulk operation. The other workers take
// its chunks, so the operation completes before that worker is released.
void test_busy_worker()
{
    std::size_t const num_workers = hpx::get_num_worker_threads();
    if (num_workers < 2)
        return;

    std::size_t const busy_worker = num_workers - 1;
    std::atomic<bool> started(false);
    std::atomic<bool> release(false);

    // Spin without yielding, so the worker can't run anything else. A hint
    // alone could let another worker steal the task before it starts, bound
    // threads are never stolen (launch::task: never run the thread inline).
    auto const busy_policy =
        hpx::launch::task_policy(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(
                static_cast<std::int16_t>(busy_worker)));
    hpx::future<void> busy = hpx::async(busy_policy, [&] {
        HPX_TEST_EQ(hpx::get_worker_thread_num(), busy_worker);
        started = true;
        while (!release.load())
        {
        }
    });
    while (!started.load())
    {
        hpx::this_thread::yield();
    }

    hpx::execution::parallel_executor exec;
    std::vector<std::size_t> const shape = make_shape();
    std::atomic<std::size_t> calls(0);

    hpx::future<void> bulk =
        ex::bulk_async_execute(exec, [&](std::size_t) { ++calls; }, shape);

    // Poll instead of suspending: a suspended thread may be resumed on the
    // busy worker, where nothing else runs until it is released.
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!bulk.is_ready() && std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }
    HPX_TEST(bulk.is_ready());

    release = true;
    busy.get();
    bulk.get();
    HPX_TEST_EQ(calls.load(), num_elements);
}

int hpx_main()
{
    test_all_elements_run();

    // a few elements in the range of each worker throw
    test_exceptions([](std::size_t i) { return i % 997 == 0; });

    // every element throws
    test_exceptions([](std::size_t) { return true; });

    test_exceptions_from_two_workers();

    // Only schedulers with bound queues that take work from a busy worker. The
    // local scheduler has no bound queues, it may move the busy task, the
    // static schedulers don't steal, and the work-requesting schedulers need
    // the busy worker to answer the steal requests.
    std::string const scheduler = hpx::get_config_entry("hpx.scheduler", "");
    if (scheduler.starts_with("local-priority") ||
        scheduler.starts_with("abp-priority") || scheduler == "shared-priority")
    {
        test_busy_worker();
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
