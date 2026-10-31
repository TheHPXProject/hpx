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
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <numeric>
#include <stdexcept>
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

    // spin without yielding, so the worker can't run anything else
    hpx::future<void> busy =
        hpx::async(hpx::execution::experimental::with_hint(hpx::launch::async,
                       hpx::threads::thread_schedule_hint(
                           static_cast<std::int16_t>(busy_worker))),
            [&] {
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
    HPX_TEST(
        bulk.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);

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

    test_busy_worker();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
