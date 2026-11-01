//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The bulk operations of thread_pool_scheduler complete once all of their
// chunks are done. A chunk that throws counts as done, and the chunks left in
// the queue of the worker that threw are skipped, while the other workers go
// on, so that several exceptions can be reported. A worker task that starts
// after the operation has completed must not touch its operation state, which
// may already be gone.

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
#include <stdexcept>
#include <string>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

constexpr std::size_t num_elements = 10007;

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

// Calls f(i) for each element, through bulk_unchunked and through
// bulk_chunked.
template <typename F>
void bulk_unchunked(F f)
{
    tt::sync_wait(ex::schedule(ex::thread_pool_scheduler{}) |
        ex::bulk_unchunked(ex::par, num_elements, f));
}

template <typename F>
void bulk_chunked(F f)
{
    tt::sync_wait(ex::schedule(ex::thread_pool_scheduler{}) |
        ex::bulk_chunked(ex::par, num_elements,
            [f](std::size_t const begin, std::size_t const end) {
                for (std::size_t i = begin; i != end; ++i)
                {
                    f(i);
                }
            }));
}

// Elements for which throws returns true throw, from the ranges of several
// workers. Each worker stops at its first exception, so there is at least one
// and at most one per worker.
template <typename Throws>
void test_exceptions(Throws throws)
{
    std::size_t const num_workers = hpx::get_num_worker_threads();

    auto f = [&](std::size_t i) {
        if (throws(i))
            throw std::runtime_error("bulk_completion_scheduler");
    };

    std::size_t n = count_exceptions([&] { bulk_unchunked(f); });
    HPX_TEST(n >= 1 && n <= num_workers);

    n = count_exceptions([&] { bulk_chunked(f); });
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
        throw std::runtime_error("bulk_completion_scheduler");
    };

    std::size_t n = count_exceptions([&] { bulk_unchunked(f); });
    HPX_TEST(n >= 2 && n <= num_workers);

    throwing = 0;
    n = count_exceptions([&] { bulk_chunked(f); });
    HPX_TEST(n >= 2 && n <= num_workers);
}

// Without exceptions, every element runs exactly once.
void test_all_elements_run()
{
    std::vector<std::atomic<int>> calls(num_elements);
    auto f = [&](std::size_t i) { ++calls[i]; };

    bulk_unchunked(f);
    bulk_chunked(f);
    for (auto const& c : calls)
    {
        HPX_TEST_EQ(c.load(), 2);
    }
}

// One worker is kept busy for the whole bulk operation. The other workers take
// its chunks, so the operation completes before that worker is released. The
// operation is started detached, so its operation state is destroyed when it
// completes; the task of the busy worker runs only after that.
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

    std::atomic<std::size_t> calls(0);
    std::atomic<bool> done(false);

    ex::start_detached(ex::schedule(ex::thread_pool_scheduler{}) |
        ex::bulk_unchunked(
            ex::par, num_elements, [&](std::size_t) { ++calls; }) |
        ex::then([&] { done = true; }));

    // Poll instead of suspending: a suspended thread may be resumed on the
    // busy worker, where nothing else runs until it is released.
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!done.load() && std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }
    HPX_TEST(done.load());
    HPX_TEST_EQ(calls.load(), num_elements);

    // Let the busy worker run the task it was given after the operation and
    // its operation state are gone.
    release = true;
    busy.get();
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
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

    // the runtime still works after the late tasks
    test_all_elements_run();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
