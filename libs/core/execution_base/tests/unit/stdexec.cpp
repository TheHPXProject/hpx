//  Copyright (c) 2024 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/testing.hpp>

#include <hpx/config.hpp>
#include <atomic>
#include <chrono>
#include <exception>
#include <semaphore>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <hpx/execution_base/stdexec_forward.hpp>
#include <exec/single_thread_context.hpp>

namespace ex = hpx::execution::experimental;

// P3149: async_scope - creating scopes for non-sequential concurrency
void test_counting_scope()
{
    ex::simple_counting_scope scope;
    auto token = scope.get_token();

    bool spawn_completed = false;
    ex::spawn(ex::just() | ex::then([&]() noexcept { spawn_completed = true; }),
        scope.get_token());

    auto fut = ex::spawn_future(ex::just(42), scope.get_token());
    auto assoc = ex::associate(ex::just(7), std::move(token));

    scope.close();

    auto fut_result = ex::sync_wait(std::move(fut));
    HPX_TEST(fut_result.has_value());
    auto [fut_value] = std::move(*fut_result);
    HPX_TEST(fut_value == 42);

    auto assoc_result = ex::sync_wait(std::move(assoc));
    HPX_TEST(assoc_result.has_value());
    auto [assoc_value] = std::move(*assoc_result);
    HPX_TEST(assoc_value == 7);

    ex::sync_wait(scope.join());

    HPX_TEST(spawn_completed);
}

void test_counting_scope_request_stop()
{
    ex::counting_scope scope;
    auto token = scope.get_token();

    // an association obtained before request_stop() remains valid
    auto assoc = token.try_associate();
    HPX_TEST(static_cast<bool>(assoc));

    scope.request_stop();
    assoc = decltype(assoc)();

    // request_stop does not close the scope -- new associations succeed
    auto assoc2 = token.try_associate();
    HPX_TEST(static_cast<bool>(assoc2));
    assoc2 = decltype(assoc2)();

    // verify stop delivery: spawn_future wraps the sender with the
    // scope's stop token via __stop_when; use let_value to observe
    // the stop token from the receiver's environment
    auto snd = ex::spawn_future(ex::just() | ex::let_value([]() {
        return stdexec::read_env(stdexec::get_stop_token) |
            ex::then([](auto stoken) { return stoken.stop_requested(); });
    }),
        scope.get_token());

    auto result = ex::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    auto [stop_requested] = std::move(*result);
    HPX_TEST(stop_requested);

    scope.close();
    ex::sync_wait(scope.join());
}

void test_closed_scope_rejects_association()
{
    ex::simple_counting_scope scope;
    scope.close();

    // try_associate on a closed scope must return a falsy association
    auto token = scope.get_token();
    auto assoc = token.try_associate();
    HPX_TEST(!static_cast<bool>(assoc));

    ex::sync_wait(scope.join());
}

void test_spawn_future_with_error()
{
    ex::simple_counting_scope scope;

    // spawn_future accepts senders that may complete with an error;
    // convert the error to a value so we can observe it via sync_wait
    auto error_sender = ex::just(42) |
        ex::then([](int) -> int { throw std::runtime_error("oops"); });

    auto fut = ex::spawn_future(std::move(error_sender), scope.get_token());

    scope.close();

    // the future-sender propagates the error through set_error;
    // pipe through let_error to convert to a value we can sync_wait
    bool caught = false;
    auto handled = std::move(fut) | ex::let_error([&](auto eptr) {
        try
        {
            std::rethrow_exception(eptr);
        }
        catch (std::runtime_error const& e)
        {
            caught = true;
            HPX_TEST_EQ(std::string(e.what()), std::string("oops"));
        }
        return ex::just(-1);
    });

    auto result = ex::sync_wait(std::move(handled));
    HPX_TEST(caught);
    HPX_TEST(result.has_value());
    auto [val] = std::move(*result);
    HPX_TEST(val == -1);

    ex::sync_wait(scope.join());
}

void test_associate_pipe_syntax()
{
    ex::simple_counting_scope scope;

    // associate supports pipe: sender | associate(token)
    auto snd = ex::just(99) | ex::associate(scope.get_token());

    scope.close();

    auto result = ex::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    auto [val] = std::move(*result);
    HPX_TEST(val == 99);

    ex::sync_wait(scope.join());
}

// Verify join() remains incomplete while scoped work is in flight.
// One operation signals that it is running and blocked, then join()
// is started and must not complete until the operation is released.
void test_counting_scope_concurrent_join()
{
    exec::single_thread_context ctx;
    ex::simple_counting_scope scope;
    std::atomic<int> completed{0};
    constexpr int n = 8;

    // Synchronization: held op signals arrival, main thread releases it
    std::binary_semaphore arrived{0};
    std::binary_semaphore release{0};

    for (int i = 0; i < n; ++i)
    {
        auto work =
            ex::schedule(ctx.get_scheduler()) | ex::then([&, i]() noexcept {
                if (i == 0)
                {
                    // Signal that this operation is running and blocked
                    arrived.release();
                    release.acquire();
                }
                completed.fetch_add(1, std::memory_order_release);
            });
        ex::spawn(std::move(work), scope.get_token());
    }

    // Wait until the held operation confirms it is running
    arrived.acquire();

    scope.close();

    // Start join() on a separate thread
    std::atomic<bool> join_done{false};
    std::thread joiner([&]() {
        ex::sync_wait(scope.join());
        join_done.store(true, std::memory_order_release);
    });

    // The held operation is still blocked; join() must not complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    HPX_TEST(!join_done.load(std::memory_order_acquire));

    // Release the held operation
    release.release();
    joiner.join();

    // join() completed only after all work finished
    HPX_TEST(join_done.load(std::memory_order_acquire));
    HPX_TEST_EQ(completed.load(std::memory_order_acquire), n);
}

// Verify concurrent spawns from multiple threads don't corrupt scope
// state. One operation per thread is held at a synchronization point;
// join() must block until all are released.
void test_counting_scope_multithreaded_spawn()
{
    exec::single_thread_context ctx;
    ex::simple_counting_scope scope;
    std::atomic<int> completed{0};
    constexpr int num_threads = 4;
    constexpr int spawns_per_thread = 8;
    constexpr int n = num_threads * spawns_per_thread;

    // One held operation: signals arrival, waits for release
    std::binary_semaphore arrived{0};
    std::binary_semaphore release{0};
    std::atomic<bool> first_claimed{false};

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&]() {
            for (int i = 0; i < spawns_per_thread; ++i)
            {
                auto work = ex::schedule(ctx.get_scheduler()) |
                    ex::then([&]() noexcept {
                        // Exactly one operation holds the gate
                        bool expected = false;
                        if (first_claimed.compare_exchange_strong(
                                expected, true, std::memory_order_acq_rel))
                        {
                            arrived.release();
                            release.acquire();
                        }
                        completed.fetch_add(1, std::memory_order_release);
                    });
                ex::spawn(std::move(work), scope.get_token());
            }
        });
    }

    for (auto& th : threads)
        th.join();

    // Wait until the held operation confirms it is running
    arrived.acquire();

    scope.close();

    // Start join() on a separate thread
    std::atomic<bool> join_done{false};
    std::thread joiner([&]() {
        ex::sync_wait(scope.join());
        join_done.store(true, std::memory_order_release);
    });

    // The held operation is still blocked; join() must not complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    HPX_TEST(!join_done.load(std::memory_order_acquire));

    // Release the held operation
    release.release();
    joiner.join();

    HPX_TEST(join_done.load(std::memory_order_acquire));
    HPX_TEST_EQ(completed.load(std::memory_order_acquire), n);
}

// spawn on a closed scope silently drops the work (P3149 [exec.spawn])
void test_spawn_on_closed_scope()
{
    ex::simple_counting_scope scope;
    scope.close();

    bool executed = false;
    ex::spawn(ex::just() | ex::then([&]() noexcept { executed = true; }),
        scope.get_token());

    ex::sync_wait(scope.join());

    // work must NOT have been started
    HPX_TEST(!executed);
}

// spawn_future on a closed scope completes via set_stopped
// (P3149 [exec.spawn.future])
void test_spawn_future_on_closed_scope()
{
    ex::simple_counting_scope scope;
    scope.close();

    auto fut = ex::spawn_future(ex::just(42), scope.get_token());

    // stopped_as_optional converts set_stopped to std::nullopt
    auto result = ex::sync_wait(std::move(fut) | ex::stopped_as_optional());
    HPX_TEST(result.has_value());
    auto [opt_val] = std::move(*result);
    HPX_TEST(!opt_val.has_value());

    ex::sync_wait(scope.join());
}

int main()
{
    auto x = hpx::execution::experimental::just(42);
    auto result = hpx::execution::experimental::sync_wait(std::move(x));

    HPX_TEST(result.has_value());
    if (!result)
        return hpx::util::report_errors();
    auto [a] = std::move(*result);

    HPX_TEST(a == 42);

    test_counting_scope();
    test_counting_scope_request_stop();
    test_closed_scope_rejects_association();
    test_spawn_future_with_error();
    test_associate_pipe_syntax();
    test_counting_scope_concurrent_join();
    test_counting_scope_multithreaded_spawn();
    test_spawn_on_closed_scope();
    test_spawn_future_on_closed_scope();

    return hpx::util::report_errors();
}
