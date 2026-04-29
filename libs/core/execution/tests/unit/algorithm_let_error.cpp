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

namespace ex = hpx::execution::experimental;

// This overload is only used to check dispatching. It is not a useful
// implementation.
template <typename F>
auto tag_invoke(ex::let_error_t, custom_sender_tag_invoke s, F&&)
{
    s.tag_invoke_overload_called = true;
    return void_sender{};
}

int main()
{
    // "Success" path, i.e. let_error gets to handle the error
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 = error_sender{};
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr ep) {
            check_exception_ptr{}(ep);
            let_error_callback_called = true;
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
        HPX_TEST(let_error_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 = error_sender{};
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr ep) {
            check_exception_ptr{}(ep);
            let_error_callback_called = true;
            return ex::just(42);
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        /*TODO: the following is unclear: https://rentry.org/4rzhctgx
         * So this is volatile to internal stdexec changes, and is thus disabled
         * check_value_types<hpx::variant<hpx::tuple<>, hpx::tuple<int>>>(s2);*/
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_error_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 = error_sender{};
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr ep) {
            check_exception_ptr{}(ep);
            let_error_callback_called = true;
            return ex::just(custom_type_non_default_constructible{42});
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        /* Disabled due to ambiguous order
         * check_value_types<hpx::variant<hpx::tuple<>,
         *   hpx::tuple<custom_type_non_default_constructible>>>(s2); */
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_error_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 = error_sender{};
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr ep) {
            check_exception_ptr{}(ep);
            let_error_callback_called = true;
            return ex::just(
                custom_type_non_default_constructible_non_copyable{42});
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        /* Disabled due to ambiguous order
         * check_value_types<hpx::variant<hpx::tuple<>,
         *   hpx::tuple<custom_type_non_default_constructible_non_copyable>>>(
         *   s2); */
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_error_callback_called);
    }

    // operator| overload
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s = error_sender{} | ex::let_error([&](std::exception_ptr ep) {
            check_exception_ptr{}(ep);
            let_error_callback_called = true;
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
        HPX_TEST(let_error_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s = error_sender{} | ex::let_error([&](std::exception_ptr ep) {
            check_exception_ptr{}(ep);
            let_error_callback_called = true;
            return ex::just(42);
        });

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        /* Disabled due to ambiguous order
         * check_value_types<hpx::variant<hpx::tuple<>, hpx::tuple<int>>>(s); */
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(let_error_callback_called);
    }

    // tag_invoke overload
    {
        std::atomic<bool> tag_invoke_overload_called{false};
        auto s = custom_sender_tag_invoke{tag_invoke_overload_called} |
            ex::let_error([&](std::exception_ptr) { return ex::just(); });
        HPX_TEST(tag_invoke_overload_called);

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<>>(s);
        check_sends_stopped<false>(s);
    }

    // "Failure" path, i.e. let_error has no error to handle
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 = ex::just(42);
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr) {
            HPX_TEST(false);
            return ex::just(43);
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s2);
        // the returned sender does not throw any errors
        check_error_types<hpx::variant<>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(!let_error_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 = ex::just(custom_type_non_default_constructible{42});
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr) {
            HPX_TEST(false);
            return ex::just(custom_type_non_default_constructible{43});
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<
            hpx::variant<hpx::tuple<custom_type_non_default_constructible>>>(
            s2);
        check_error_types<hpx::variant<>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(!let_error_callback_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> let_error_callback_called{false};
        auto s1 =
            ex::just(custom_type_non_default_constructible_non_copyable{42});
        auto s2 = ex::let_error(std::move(s1), [&](std::exception_ptr) {
            HPX_TEST(false);
            return ex::just(
                custom_type_non_default_constructible_non_copyable{43});
        });

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<
            hpx::tuple<custom_type_non_default_constructible_non_copyable>>>(
            s2);
        check_error_types<hpx::variant<>>(s2);
        check_sends_stopped<false>(s2);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<void_callback_helper<decltype(f)>>{
            void_callback_helper<decltype(f)>{f}, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(!let_error_callback_called);
    }

    return hpx::util::report_errors();
}
