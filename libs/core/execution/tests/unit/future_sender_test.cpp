//  Copyright (c) 2025 Shivansh Singh
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// Tests for the two-way bridge between hpx::future<T> and P2300 senders.
///
/// Test coverage:
///   1. future_to_sender:    hpx::future<int> -> as_sender() -> stdexec::then
///                           -> stdexec::sync_wait
///   2. sender_to_future:    stdexec::just | stdexec::then -> as_future()
///                           -> .get()
///   3. error_propagation:   future holding exception -> sender error channel
///   4. move_only_semantics: as_sender(future) remains move-only

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace tt = hpx::this_thread::experimental;
namespace ex = hpx::execution::experimental;

///////////////////////////////////////////////////////////////////////////////
// Test 1: future<int> -> as_sender -> stdexec::then(x*2) -> sync_wait
void test_future_to_sender()
{
    hpx::future<int> f = hpx::async([]() -> int { return 42; });

    auto snd = hpx::execution::experimental::as_sender(std::move(f));
    auto result = tt::sync_wait(std::move(snd) |
        hpx::execution::experimental::then([](int x) { return x * 2; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 84);
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: stdexec::just(42) | stdexec::then(x+1) -> as_future<int>() -> .get()
void test_sender_to_future()
{
    auto snd = hpx::execution::experimental::just(42) |
        hpx::execution::experimental::then([](int x) { return x + 1; });

    hpx::future<int> f =
        hpx::execution::experimental::as_future<int>(std::move(snd));

    HPX_TEST_EQ(f.get(), 43);
}

///////////////////////////////////////////////////////////////////////////////
// Test 3: future holding std::runtime_error propagates via set_error channel.
//
// We verify error propagation through the public as_sender CPO.
void test_error_propagation()
{
    // Create a future that already holds an exception
    hpx::future<int> f = hpx::make_exceptional_future<int>(
        std::make_exception_ptr(std::runtime_error("test error")));

    // Pipe through sync_wait; if set_error is triggered, sync_wait returns
    // an empty optional (or propagates the exception, depending on stdexec
    // version).  We verify that no value result was produced.
    bool caught_error = false;
    try
    {
        auto snd = hpx::execution::experimental::as_sender(std::move(f));
        auto result = tt::sync_wait(std::move(snd) |
            hpx::execution::experimental::then([](int) { return 0; }));
        // If we get here with a valid result, that's wrong
        if (!result.has_value())
        {
            caught_error = true;
        }
    }
    catch (std::runtime_error const& e)
    {
        caught_error = true;
    }
    catch (...)
    {
        caught_error = true;
    }

    HPX_TEST(caught_error);
}

///////////////////////////////////////////////////////////////////////////////
// Test 4: as_sender(future<T>) must be move-only (not copyable)
void test_move_only_semantics()
{
    using sender_type = decltype(hpx::execution::experimental::as_sender(
        std::declval<hpx::future<int>>()));

    // Verify copy operations are deleted
    HPX_TEST(!std::is_copy_constructible_v<sender_type>);
    HPX_TEST(!std::is_copy_assignable_v<sender_type>);

    // Verify move operations are available
    HPX_TEST(std::is_move_constructible_v<sender_type>);
    HPX_TEST(std::is_move_assignable_v<sender_type>);

    // Actually move a sender and use it
    hpx::future<int> f = hpx::make_ready_future(10);
    sender_type s1 = hpx::execution::experimental::as_sender(std::move(f));
    sender_type s2 = std::move(s1);    // move construct - must compile

    auto result = tt::sync_wait(std::move(s2) |
        hpx::execution::experimental::then([](int x) { return x * 3; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 30);
}

///////////////////////////////////////////////////////////////////////////////
// Test 5: as_sender(shared_future<T>) remains copyable
void test_shared_future_copyable_semantics()
{
    using sender_type = decltype(hpx::execution::experimental::as_sender(
        std::declval<hpx::shared_future<int>>()));

    HPX_TEST(std::is_copy_constructible_v<sender_type>);
    HPX_TEST(std::is_copy_assignable_v<sender_type>);

    hpx::shared_future<int> f = hpx::make_ready_future(12);
    sender_type s1 = hpx::execution::experimental::as_sender(f);
    sender_type s2 = s1;

    auto result = tt::sync_wait(std::move(s2) |
        hpx::execution::experimental::then([](int x) { return x + 1; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 13);
}

///////////////////////////////////////////////////////////////////////////////
// Test 6: as_sender(future<T>, scheduler) preserves scheduler/domain
// information in the sender environment.
void test_as_sender_with_scheduler_ready_future()
{
    ex::thread_pool_scheduler sched{};

    auto snd = ex::as_sender(hpx::make_ready_future(21), sched);

    static_assert(ex::is_sender_v<decltype(snd)>);

    auto sched_from_env =
        ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(snd));
    HPX_TEST(sched == sched_from_env);

    auto result =
        tt::sync_wait(std::move(snd) | ex::then([](int x) { return x * 2; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 42);
}

///////////////////////////////////////////////////////////////////////////////
// Test 7: void future values are forwarded correctly through the
// scheduler-aware path.
void test_as_sender_with_scheduler_void_future()
{
    ex::thread_pool_scheduler sched{};
    bool continued = false;

    auto snd = ex::as_sender(hpx::make_ready_future(), sched);

    auto result =
        tt::sync_wait(std::move(snd) | ex::then([&]() { continued = true; }));

    HPX_TEST(result.has_value());
    HPX_TEST(continued);
}

///////////////////////////////////////////////////////////////////////////////
// Test 8: shared_future remains copyable and works through the
// scheduler-aware path.
void test_as_sender_with_scheduler_shared_future()
{
    ex::thread_pool_scheduler sched{};
    hpx::shared_future<int> f = hpx::make_ready_future(13);

    auto snd = ex::as_sender(f, sched);
    static_assert(std::is_copy_constructible_v<decltype(snd)>);
    auto snd_copy = snd;

    auto result = tt::sync_wait(
        std::move(snd_copy) | ex::then([](int x) { return x + 1; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 14);
}

///////////////////////////////////////////////////////////////////////////////
// Test 9: exceptional futures still use the receiver error channel.
void test_as_sender_with_scheduler_error()
{
    ex::thread_pool_scheduler sched{};
    auto f = hpx::make_exceptional_future<int>(
        std::make_exception_ptr(std::runtime_error("test error")));

    bool caught = false;
    try
    {
        tt::sync_wait(ex::as_sender(std::move(f), sched));
        HPX_TEST(false);
    }
    catch (std::runtime_error const& e)
    {
        caught = true;
        HPX_TEST_EQ(std::string(e.what()), std::string("test error"));
    }

    HPX_TEST(caught);
}

///////////////////////////////////////////////////////////////////////////////
// Test 10: as_sender(future) | continues_on(thread_pool_scheduler) is lowered
// to HPX's optimized thread-pool continues_on sender.
void test_as_sender_pipe_continues_on_uses_fused_bridge()
{
    ex::thread_pool_scheduler sched{};

    auto snd =
        ex::as_sender(hpx::make_ready_future(21)) | ex::continues_on(sched);

    using sender_type = decltype(snd);
    using transformed_sender_type = std::decay_t<decltype(ex::transform_sender(
        std::declval<sender_type>(), std::declval<ex::empty_env const&>()))>;

    static_assert(std::is_same_v<transformed_sender_type,
        ex::detail::thread_pool_continues_on_sender<
            ex::detail::future_sender<hpx::future<int>>,
            ex::thread_pool_scheduler>>);

    auto result =
        tt::sync_wait(std::move(snd) | ex::then([](int x) { return x * 2; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 42);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_future_to_sender();
    test_sender_to_future();
    test_error_propagation();
    test_move_only_semantics();
    test_shared_future_copyable_semantics();
    test_as_sender_with_scheduler_ready_future();
    test_as_sender_with_scheduler_void_future();
    test_as_sender_with_scheduler_shared_future();
    test_as_sender_with_scheduler_error();
    test_as_sender_pipe_continues_on_uses_fused_bridge();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
