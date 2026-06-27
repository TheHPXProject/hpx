//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2021-2022 Hartmut Kaiser
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

struct custom_bulk_operation
{
    std::atomic<bool>& tag_invoke_overload_called;
    std::atomic<bool>& call_operator_called;
    std::atomic<int>& call_operator_count;
    bool throws;

    void operator()(int n) const
    {
        HPX_TEST_EQ(n, call_operator_count);

        call_operator_called = true;
        if (n == 3 && throws)
        {
            throw std::runtime_error("error");
        }
        ++call_operator_count;
    }
};

int main()
{
    // Success path
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<int> set_value_count{0};
        auto s = ex::bulk(ex::just(), 10, [&](int n) {
            HPX_TEST_EQ(n, set_value_count);
            ++set_value_count;
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
        HPX_TEST_EQ(set_value_count, 10);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<int> set_value_count{0};
        auto s = ex::bulk(ex::just(42), 10, [&](int n, int x) {
            HPX_TEST_EQ(n, set_value_count);
            ++set_value_count;
            return ++x;
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
        HPX_TEST_EQ(set_value_count, 10);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<int> set_value_count{0};
        auto s = ex::bulk(ex::just(custom_type_non_default_constructible{42}),
            10, [&](int n, auto x) {
                HPX_TEST_EQ(n, set_value_count);
                ++set_value_count;
                ++(x.x);
                return x;
            });

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<
            hpx::variant<hpx::tuple<custom_type_non_default_constructible>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](auto&& x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST_EQ(set_value_count, 10);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<int> set_value_count{0};
        auto s = ex::bulk(
            ex::just(custom_type_non_default_constructible_non_copyable{42}),
            10, [&](int n, auto&&) {
                HPX_TEST_EQ(n, set_value_count);
                ++set_value_count;
            });

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<
            hpx::tuple<custom_type_non_default_constructible_non_copyable>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST_EQ(set_value_count, 10);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<int> set_value_count{0};
        auto s1 = ex::bulk(ex::just(42), 10, [](int, int x) { return ++x; });

        static_assert(ex::is_sender_v<decltype(s1)>);
        static_assert(ex::is_sender_in_v<decltype(s1), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s1);
        check_error_types<hpx::variant<std::exception_ptr>>(s1);
        check_sends_stopped<false>(s1);

        auto f = [&](int, int) { ++set_value_count; };
        auto s2 = ex::bulk(std::move(s1), 10, f);

        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto s3 = ex::bulk(std::move(s2), 10, f);

        static_assert(ex::is_sender_v<decltype(s3)>);
        static_assert(ex::is_sender_in_v<decltype(s3), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s3);
        check_error_types<hpx::variant<std::exception_ptr>>(s3);
        check_sends_stopped<false>(s3);

        auto s4 = ex::bulk(std::move(s3), 10, f);

        static_assert(ex::is_sender_v<decltype(s4)>);
        static_assert(ex::is_sender_in_v<decltype(s4), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s4);
        check_error_types<hpx::variant<std::exception_ptr>>(s4);
        check_sends_stopped<false>(s4);

        auto f1 = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<decltype(f1)>{f1, set_value_called};
        auto os = ex::connect(std::move(s4), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST_EQ(set_value_count, 30);
    }

    // operator| overload
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<int> set_value_count{0};
        auto f = [&](int, int) { ++set_value_count; };
        auto s = ex::just(42) | ex::bulk(10, f) | ex::bulk(10, f) |
            ex::bulk(10, f) | ex::bulk(10, f);

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f1 = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<decltype(f1)>{f1, set_value_called};
        auto os = ex::connect(std::move(s), r);
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST_EQ(set_value_count, 40);
    }

    // tag_invoke overload
    {
        std::atomic<bool> receiver_set_value_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        std::atomic<bool> custom_bulk_call_operator_called{false};
        std::atomic<int> custom_bulk_call_count{0};
        auto s = ex::bulk(ex::just(), 10,
            custom_bulk_operation{tag_invoke_overload_called,
                custom_bulk_call_operator_called, custom_bulk_call_count,
                false});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, receiver_set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(receiver_set_value_called);
        // stdexec doesn't use tag_invoke for bulk customization
        HPX_TEST(!tag_invoke_overload_called);
        HPX_TEST(custom_bulk_call_operator_called);
        HPX_TEST_EQ(custom_bulk_call_count, 10);
    }

    // Failure path
    {
        std::atomic<bool> set_error_called{false};
        auto s = ex::bulk(
            ex::just(), 0, [](int) { throw std::runtime_error("error"); });

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called, true};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(!set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        auto s = ex::bulk(ex::just(), 10, [](int n) {
            if (n == 3)
                throw std::runtime_error("error");
        });

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
        auto s1 = ex::bulk(ex::just(0), 10, [](int, int x) { return ++x; });
        auto s2 = ex::bulk(std::move(s1), 10, [](int n, int x) {
            if (n == 3)
                throw std::runtime_error("error");
            return x + 1;
        });
        auto s3 =
            ex::bulk(std::move(s2), 10, [](int, int) { HPX_TEST(false); });
        auto s4 =
            ex::bulk(std::move(s3), 10, [](int, int) { HPX_TEST(false); });

        static_assert(ex::is_sender_v<decltype(s4)>);
        static_assert(ex::is_sender_in_v<decltype(s4), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s4);
        check_error_types<hpx::variant<std::exception_ptr>>(s4);
        check_sends_stopped<false>(s4);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s4), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> receiver_set_error_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        std::atomic<bool> custom_bulk_call_operator_called{false};
        std::atomic<int> custom_bulk_call_count{0};
        auto s = ex::bulk(ex::just(), 10,
            custom_bulk_operation{tag_invoke_overload_called,
                custom_bulk_call_operator_called, custom_bulk_call_count,
                true});

        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, receiver_set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(receiver_set_error_called);
        // stdexec doesn't use tag_invoke for bulk customization
        HPX_TEST(!tag_invoke_overload_called);
        HPX_TEST(custom_bulk_call_operator_called);
        HPX_TEST_EQ(custom_bulk_call_count, 3);
    }

    return hpx::util::report_errors();
}
