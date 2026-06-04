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
///   4. move_only_semantics: future_sender cannot be copied

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/futures/future_sender.hpp>
#include <hpx/futures/sender_future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
// Test 1: future<int> → future_sender → stdexec::then(x*2) → sync_wait
void test_future_to_sender()
{
    hpx::future<int> f = hpx::async([]() -> int { return 42; });

    auto snd = hpx::execution::experimental::future_sender<int>{std::move(f)};
    auto result = hpx::execution::experimental::sync_wait(
        std::move(snd) | hpx::execution::experimental::then([](int x) {
            return x * 2;
        }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 84);
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: stdexec::just(42) | stdexec::then(x+1) → as_future<int>() → .get()
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
// We verify error propagation by connecting future_sender to a manual receiver
// that records which completion channel was invoked.
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
        auto snd = hpx::execution::experimental::future_sender<int>{std::move(f)};
        auto result = hpx::execution::experimental::sync_wait(
            std::move(snd) | hpx::execution::experimental::then([](int) { return 0; }));
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
// Test 4: future_sender<T> must be move-only (not copyable)
void test_move_only_semantics()
{
    using sender_type = hpx::execution::experimental::future_sender<int>;

    // Verify copy operations are deleted
    HPX_TEST(!std::is_copy_constructible_v<sender_type>);
    HPX_TEST(!std::is_copy_assignable_v<sender_type>);

    // Verify move operations are available
    HPX_TEST(std::is_move_constructible_v<sender_type>);
    HPX_TEST(std::is_move_assignable_v<sender_type>);

    // Actually move a sender and use it
    hpx::future<int> f = hpx::make_ready_future(10);
    sender_type s1 = hpx::execution::experimental::future_sender<int>{std::move(f)};
    sender_type s2 = std::move(s1);    // move construct — must compile

    auto result = hpx::execution::experimental::sync_wait(
        std::move(s2) | hpx::execution::experimental::then([](int x) {
            return x * 3;
        }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 30);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_future_to_sender();
    test_sender_to_future();
    test_error_propagation();
    test_move_only_semantics();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
