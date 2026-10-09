//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tests for P3149 async_scope facilities (spawn, spawn_future, associate)

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/modules/synchronization.hpp>

#include <async_scope_test_utilities.hpp>

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

using hpx::test::wait_join;

// spawn_future with thread_pool_scheduler: value propagation
void test_spawn_future_value()
{
    ex::simple_counting_scope scope;

    auto fut = ex::spawn_future(ex::schedule(ex::thread_pool_scheduler{}) |
            ex::then([]() { return 42; }),
        scope.get_token());

    scope.close();

    auto result = tt::sync_wait(std::move(fut));
    HPX_TEST(result.has_value());
    auto [val] = std::move(*result);
    HPX_TEST_EQ(val, 42);

    wait_join(scope);
}

// spawn_future with thread_pool_scheduler: error propagation
void test_spawn_future_error()
{
    ex::simple_counting_scope scope;

    auto fut = ex::spawn_future(ex::schedule(ex::thread_pool_scheduler{}) |
            ex::then([]() -> int { throw std::runtime_error("test error"); }),
        scope.get_token());

    scope.close();

    bool caught = false;
    auto handled = std::move(fut) | ex::let_error([&](auto eptr) {
        try
        {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& e)
        {
            caught = true;
            HPX_TEST_EQ(std::string(e.what()), std::string("test error"));
        }
        return ex::just(-1);
    });

    auto result = tt::sync_wait(std::move(handled));
    HPX_TEST(caught);
    HPX_TEST(result.has_value());
    auto [val] = std::move(*result);
    HPX_TEST_EQ(val, -1);

    wait_join(scope);
}

// spawn with thread_pool_scheduler: observable side effect.
// thread_pool_scheduler's sender includes set_error_t in its completions,
// so we pipe through upon_error to satisfy spawn's no-error requirement.
void test_spawn_with_scheduler()
{
    ex::simple_counting_scope scope;
    std::atomic<int> completed{0};
    constexpr int n = 10;

    for (int i = 0; i < n; ++i)
    {
        ex::spawn(ex::schedule(ex::thread_pool_scheduler{}) |
                ex::then([&]() noexcept {
                    completed.fetch_add(1, std::memory_order_relaxed);
                }) |
                ex::upon_error([](auto) noexcept {}),
            scope.get_token());
    }

    scope.close();
    wait_join(scope);

    HPX_TEST_EQ(completed.load(), n);
}

// join() waits for all spawned work to finish. Uses start_detached
// instead of sync_wait on a separate thread to avoid blocking an OS worker.
void test_join_blocks_for_async_work()
{
    ex::simple_counting_scope scope;
    std::atomic<int> completed{0};
    constexpr int n = 8;

    hpx::binary_semaphore arrived{0};
    hpx::binary_semaphore release{0};

    for (int i = 0; i < n; ++i)
    {
        ex::spawn(ex::schedule(ex::thread_pool_scheduler{}) |
                ex::then([&, i]() noexcept {
                    if (i == 0)
                    {
                        arrived.release();
                        release.acquire();
                    }
                    completed.fetch_add(1, std::memory_order_release);
                }) |
                ex::upon_error([](auto) noexcept {}),
            scope.get_token());
    }

    // Wait until the held operation is running
    arrived.acquire();

    scope.close();

    // Start join() detached. Record completed count inside the join
    // continuation to prove join() waited for all work.
    int completed_at_join = 0;
    hpx::binary_semaphore join_finished{0};

    ex::start_detached(ex::schedule(ex::thread_pool_scheduler{}) |
            ex::let_value([&]() { return scope.join(); }) |
            ex::then([&]() noexcept {
                completed_at_join = completed.load(std::memory_order_acquire);
                join_finished.release();
            }),
        ex::make_env(
            ex::prop(ex::get_start_scheduler, ex::thread_pool_scheduler{})));

    // Release the held operation
    release.release();

    // Suspends the HPX task; does not block the OS worker
    join_finished.acquire();

    HPX_TEST_EQ(completed_at_join, n);
    HPX_TEST_EQ(completed.load(std::memory_order_acquire), n);
}

// spawn_future on a closed scope completes via set_stopped
void test_spawn_future_closed_scope()
{
    ex::simple_counting_scope scope;
    scope.close();

    auto fut = ex::spawn_future(ex::schedule(ex::thread_pool_scheduler{}) |
            ex::then([]() { return 99; }),
        scope.get_token());

    auto result = tt::sync_wait(std::move(fut) | ex::stopped_as_optional());
    HPX_TEST(result.has_value());
    auto [opt_val] = std::move(*result);
    HPX_TEST(!opt_val.has_value());

    wait_join(scope);
}

int hpx_main(int, char*[])
{
    test_spawn_future_value();
    test_spawn_future_error();
    test_spawn_with_scheduler();
    test_join_blocks_for_async_work();
    test_spawn_future_closed_scope();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
