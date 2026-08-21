//  Copyright (c) 2024 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/testing.hpp>

#include <hpx/config.hpp>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

#include <hpx/execution_base/stdexec_forward.hpp>

namespace ex = hpx::execution::experimental;

// P3149: async_scope - creating scopes for non-sequential concurrency
void test_counting_scope()
{
    ex::simple_counting_scope scope;
    auto token = scope.get_token();

    ex::spawn(ex::just(), scope.get_token());

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
            HPX_TEST(std::string(e.what()) == "oops");
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

    return hpx::util::report_errors();
}
