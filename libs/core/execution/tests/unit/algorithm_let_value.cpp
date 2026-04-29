//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <hpx/modules/type_support.hpp>

namespace ex = hpx::execution::experimental;

// This overload is only used to check dispatching. It is not a useful
// implementation.
template <typename F>
auto tag_invoke(ex::let_value_t, custom_sender_tag_invoke s, F&&)
{
    s.tag_invoke_overload_called = true;
    return void_sender{};
}

int main()
{
    // Success path
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s1 = void_sender{};
        auto s2 = ex::let_value(std::move(s1), [&]() {
            let_value_callback_called = true;
            return void_sender();
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_value_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s1 = ex::just(42);
        auto s2 = ex::let_value(std::move(s1), [&](int& x) {
            HPX_TEST_EQ(x, 42);
            let_value_callback_called = true;
            return ex::just(x);
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_value_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s1 = ex::just(custom_type_non_default_constructible{42});
        auto s2 = ex::let_value(std::move(s1), [&](auto& x) {
            HPX_TEST_EQ(x.x, 42);
            let_value_callback_called = true;
            return ex::just(x);
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<
            hpx::variant<hpx::tuple<custom_type_non_default_constructible>>>(
            s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_value_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s1 =
            ex::just(custom_type_non_default_constructible_non_copyable{42});
        auto s2 = ex::let_value(std::move(s1), [&](auto& x) {
            HPX_TEST_EQ(x.x, 42);
            let_value_callback_called = true;
            return ex::just(std::move(x));
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<
            hpx::tuple<custom_type_non_default_constructible_non_copyable>>>(
            s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_value_callback_called);
    }

    // operator| overload
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s = void_sender{} | ex::let_value([&]() {
            let_value_callback_called = true;
            return void_sender();
        });

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_value_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s = ex::just(42) | ex::let_value([&](int& x) {
            HPX_TEST_EQ(x, 42);
            let_value_callback_called = true;
            return ex::just(x);
        });

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_value_callback_called);
    }

    // tag_invoke overload
    {
        std::atomic<bool> tag_invoke_overload_called{false};
        auto s = custom_sender_tag_invoke{tag_invoke_overload_called} |
            ex::let_value([]() { return ex::just(); });
        HPX_TEST(tag_invoke_overload_called);

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<>>(s);
        check_sends_stopped<false>(s);
    }

    // Failure path
    {
        std::atomic<bool> set_error_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s1 = error_sender{};
        auto s2 = std::move(s1) | ex::let_value([&]() {
            let_value_callback_called = true;
            return void_sender();
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(void_sender());
        // In STDEXEC the sender returned by let_value has completion signatures
        // equal to the union of the:
        // - completion signatures of the sender returned by the function for
        //   the input given by the predecessor sender
        // - completion signatures of the predecessor sender for the CPOs other
        //   than set_value
        // - set_error(std::exception_ptr)

        // Here, error sender does not send stopped so we expect
        // let_value(error_sender, [](){...}) not to send stopped either.
        check_sends_stopped<false>(s2);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
        HPX_TEST(!let_value_callback_called);
    }

    {
        // Can have multiple value types (i.e. different for the predecessor
        // sender, and the one produced by the factory), as long as the receiver
        // connected to the let_value sender can handle them. In the test below
        // value_types for the let_value sender is Variant<Tuple<int>, Tuple<>>.
        std::atomic<bool> set_error_called{false};
        std::atomic<bool> let_value_callback_called{false};
        auto s1 = error_sender{};
        auto s2 = ex::let_value(std::move(s1), [&]() {
            let_value_callback_called = true;
            return ex::just(42);
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
        HPX_TEST(!let_value_callback_called);
    }

    return hpx::util::report_errors();
}
