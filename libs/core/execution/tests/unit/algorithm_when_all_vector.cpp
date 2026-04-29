//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// Clang V11 ICE's on this test, Clang V8 reports a bogus constexpr problem
#if !defined(HPX_CLANG_VERSION) ||                                             \
    ((HPX_CLANG_VERSION / 10000) != 11 && (HPX_CLANG_VERSION / 10000) != 8)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace ex = hpx::execution::experimental;

int main()
{
    // Success path
    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(std::vector<decltype(ex::just())>{});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        // In P2300 just does not send stopped so this does not either
        check_sends_stopped<false>(s);

        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(std::vector<decltype(ex::just(42))>{});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<int>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(0));
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(std::vector{ex::just(42)});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<int>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(1));
            HPX_TEST_EQ(v[0], 42);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        int x = 42;
        auto s =
            ex::when_all_vector(std::vector{const_reference_sender<int>{x}});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<int>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(1));
            HPX_TEST_EQ(v[0], 42);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(
            std::vector{ex::just(42), ex::just(43), ex::just(44)});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<int>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(3));
            HPX_TEST_EQ(v[0], 42);
            HPX_TEST_EQ(v[1], 43);
            HPX_TEST_EQ(v[2], 44);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::vector<ex::unique_any_sender<int>> senders;
        senders.emplace_back(ex::just(42));
        senders.emplace_back(ex::just(43));
        senders.emplace_back(ex::just(44));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<int>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(3));
            HPX_TEST_EQ(v[0], 42);
            HPX_TEST_EQ(v[1], 43);
            HPX_TEST_EQ(v[2], 44);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::vector<ex::any_sender<double>> senders;
        senders.emplace_back(ex::just(42.0));
        senders.emplace_back(ex::just(43.0));
        senders.emplace_back(ex::just(44.0));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<double>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<double> v) {
            HPX_TEST_EQ(v.size(), std::size_t(3));
            HPX_TEST_EQ(v[0], 42.0);
            HPX_TEST_EQ(v[1], 43.0);
            HPX_TEST_EQ(v[2], 44.0);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(std::vector{ex::just()});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = []() {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(
            std::vector{ex::just(), ex::just(), ex::just()});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = []() {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::vector<ex::unique_any_sender<>> senders;
        senders.emplace_back(ex::just());
        senders.emplace_back(ex::just());
        senders.emplace_back(ex::just());
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = []() {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::vector<ex::any_sender<>> senders;
        senders.emplace_back(ex::just());
        senders.emplace_back(ex::just());
        senders.emplace_back(ex::just());
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = []() {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::vector<ex::any_sender<custom_type_non_default_constructible>>
            senders;
        senders.emplace_back(
            ex::just(custom_type_non_default_constructible{42}));
        senders.emplace_back(
            ex::just(custom_type_non_default_constructible{43}));
        senders.emplace_back(
            ex::just(custom_type_non_default_constructible{44}));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<
            hpx::tuple<std::vector<custom_type_non_default_constructible>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::vector<custom_type_non_default_constructible> v) {
            HPX_TEST_EQ(v.size(), std::size_t(3));
            HPX_TEST_EQ(v[0].x, 42);
            HPX_TEST_EQ(v[1].x, 43);
            HPX_TEST_EQ(v[2].x, 44);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::vector<ex::unique_any_sender<
            custom_type_non_default_constructible_non_copyable>>
            senders;
        senders.emplace_back(
            ex::just(custom_type_non_default_constructible_non_copyable{42}));
        senders.emplace_back(
            ex::just(custom_type_non_default_constructible_non_copyable{43}));
        senders.emplace_back(
            ex::just(custom_type_non_default_constructible_non_copyable{44}));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<
            std::vector<custom_type_non_default_constructible_non_copyable>>>>(
            s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f =
            [](std::vector<custom_type_non_default_constructible_non_copyable>
                    v) {
                HPX_TEST_EQ(v.size(), std::size_t(3));
                HPX_TEST_EQ(v[0].x, 42);
                HPX_TEST_EQ(v[1].x, 43);
                HPX_TEST_EQ(v[2].x, 44);
            };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // Test a combination with when_all
    {
        std::atomic<bool> set_value_called{false};
        std::vector<decltype(ex::just(std::declval<double>()))> senders1;
        senders1.emplace_back(ex::just(13.0));
        senders1.emplace_back(ex::just(14.0));
        senders1.emplace_back(ex::just(15.0));

        std::vector<ex::any_sender<>> senders2;
        senders2.emplace_back(ex::just());
        senders2.emplace_back(ex::just());

        std::vector<ex::unique_any_sender<int>> senders3;
        senders3.emplace_back(ex::just(42));
        senders3.emplace_back(ex::just(43));
        senders3.emplace_back(ex::just(44));
        senders3.emplace_back(ex::just(45));

        auto s = ex::when_all(ex::when_all_vector(std::move(senders1)),
            ex::when_all_vector(std::move(senders2)),
            ex::when_all_vector(std::move(senders3)));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<
            hpx::variant<hpx::tuple<std::vector<double>, std::vector<int>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<true>(s);

        auto f = [](std::vector<double> v1, std::vector<int> v3) {
            HPX_TEST_EQ(v1.size(), std::size_t(3));
            HPX_TEST_EQ(v1[0], 13.0);
            HPX_TEST_EQ(v1[1], 14.0);
            HPX_TEST_EQ(v1[2], 15.0);

            HPX_TEST_EQ(v3.size(), std::size_t(4));
            HPX_TEST_EQ(v3[0], 42);
            HPX_TEST_EQ(v3[1], 43);
            HPX_TEST_EQ(v3[2], 44);
            HPX_TEST_EQ(v3[3], 45);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // Failure path
    {
        std::atomic<bool> set_error_called{false};
        auto s = ex::when_all_vector(std::vector{error_sender<double>{}});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<double>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        auto s =
            ex::when_all_vector(std::vector{const_reference_error_sender{}});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        std::vector<ex::unique_any_sender<double>> senders;
        senders.emplace_back(error_sender<double>{});
        senders.emplace_back(ex::just(42.0));
        senders.emplace_back(ex::just(43.0));
        senders.emplace_back(ex::just(44.0));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<double>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        std::vector<ex::unique_any_sender<double>> senders;
        senders.emplace_back(error_sender<double>{});
        senders.emplace_back(ex::just(42.0));
        senders.emplace_back(ex::just(43.0));
        senders.emplace_back(ex::just(44.0));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<double>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        std::vector<ex::any_sender<double>> senders;
        senders.emplace_back(error_sender<double>{});
        senders.emplace_back(ex::just(42.0));
        senders.emplace_back(ex::just(43.0));
        senders.emplace_back(ex::just(44.0));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<double>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        std::vector<ex::any_sender<double>> senders;
        senders.emplace_back(error_sender<double>{});
        senders.emplace_back(ex::just(42.0));
        senders.emplace_back(ex::just(43.0));
        senders.emplace_back(ex::just(44.0));
        auto s = ex::when_all_vector(std::move(senders));

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::vector<double>>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    test_adl_isolation(
        ex::when_all_vector(std::vector{my_namespace::my_sender{}}));

    return 0;
}

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
#else
int main()
{
    return 0;
}
#endif
