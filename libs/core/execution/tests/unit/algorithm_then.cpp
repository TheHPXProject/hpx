//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace ex = hpx::execution::experimental;

struct custom_transformer
{
    std::atomic<bool>& tag_invoke_overload_called;
    std::atomic<bool>& call_operator_called;
    bool throws;

    void operator()() const
    {
        call_operator_called = true;
        if (throws)
        {
            throw std::runtime_error("error");
        }
    }
};

template <typename S>
auto tag_invoke(ex::then_t, S&& s, custom_transformer t)
{
    t.tag_invoke_overload_called = true;
    return ex::then(std::forward<S>(s), [t = std::move(t)]() { t(); });
}

///////////////////////////////////////////////////////////////////////////////
void test_execution_then_return_int()
{
    hpx::future<int> f1 =
        hpx::make_ready_future_after(std::chrono::milliseconds(100), 1);
    HPX_TEST(f1.valid());
    hpx::future<int> f2 =
        ex::make_future(ex::then(ex::as_sender(std::move(f1)), [](int value) {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 2 * value;
        }));
    HPX_TEST(f2.valid());
    try
    {
        HPX_TEST_EQ(f2.get(), 2);
    }
    catch (hpx::exception const& /*ex*/)
    {
        HPX_TEST(false);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_execution_then_return_void()
{
    hpx::future<int> f1 =
        hpx::make_ready_future_after(std::chrono::milliseconds(100), 1);
    HPX_TEST(f1.valid());
    hpx::future<void> f2 =
        ex::make_future(ex::then(ex::as_sender(std::move(f1)), [](auto) {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        }));
    HPX_TEST(f2.valid());
    try
    {
        f2.wait();
    }
    catch (hpx::exception const& /*ex*/)
    {
        HPX_TEST(false);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_execution_then_return_int_shared()
{
    hpx::shared_future<int> f1 =
        hpx::make_ready_future_after(std::chrono::milliseconds(100), 1);
    HPX_TEST(f1.valid());
    hpx::future<int> f2 =
        ex::make_future(ex::then(ex::as_sender(std::move(f1)), [](int value) {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 2 * value;
        }));
    HPX_TEST(f2.valid());
    try
    {
        HPX_TEST_EQ(f2.get(), 2);
    }
    catch (hpx::exception const& /*ex*/)
    {
        HPX_TEST(false);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_execution_then_return_void_shared()
{
    hpx::shared_future<int> f1 =
        hpx::make_ready_future_after(std::chrono::milliseconds(100), 1);
    HPX_TEST(f1.valid());
    hpx::future<void> f2 =
        ex::make_future(ex::then(ex::as_sender(std::move(f1)), [](auto) {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        }));
    HPX_TEST(f2.valid());
    try
    {
        f2.wait();
    }
    catch (hpx::exception const& /*ex*/)
    {
        HPX_TEST(false);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

int hpx_main()
{
    // Success path
    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::then(ex::just(), [] {});
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
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::then(ex::just(0), [](int x) { return ++x; });
        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](int x) { HPX_TEST_EQ(x, 1); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::then(ex::just(custom_type_non_default_constructible{0}),
            [](custom_type_non_default_constructible x) {
                ++(x.x);
                return x;
            });
        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<
            hpx::variant<hpx::tuple<custom_type_non_default_constructible>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 1); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::then(
            ex::just(custom_type_non_default_constructible_non_copyable{0}),
            [](custom_type_non_default_constructible_non_copyable&& x) {
                ++(x.x);
                return std::move(x);
            });
        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<
            hpx::tuple<custom_type_non_default_constructible_non_copyable>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](auto x) { HPX_TEST_EQ(x.x, 1); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s1 = ex::then(ex::just(0), [](int x) { return ++x; });
        static_assert(ex::is_sender_v<decltype(s1)>);
        static_assert(ex::is_sender_in_v<decltype(s1), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s1);
        check_error_types<hpx::variant<std::exception_ptr>>(s1);
        check_sends_stopped<false>(s1);

        auto s2 = ex::then(std::move(s1), [](int x) { return ++x; });
        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto s3 = ex::then(std::move(s2), [](int x) { return ++x; });
        static_assert(ex::is_sender_v<decltype(s3)>);
        static_assert(ex::is_sender_in_v<decltype(s3), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s3);
        check_error_types<hpx::variant<std::exception_ptr>>(s3);
        check_sends_stopped<false>(s3);

        auto s4 = ex::then(std::move(s3), [](int x) { return ++x; });
        static_assert(ex::is_sender_v<decltype(s4)>);
        static_assert(ex::is_sender_in_v<decltype(s4), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s4);
        check_error_types<hpx::variant<std::exception_ptr>>(s4);
        check_sends_stopped<false>(s4);

        auto f = [](int x) { HPX_TEST_EQ(x, 4); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s4), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s1 = ex::then(ex::just(), []() { return 3; });
        static_assert(ex::is_sender_v<decltype(s1)>);
        static_assert(ex::is_sender_in_v<decltype(s1), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s1);
        check_error_types<hpx::variant<std::exception_ptr>>(s1);
        check_sends_stopped<false>(s1);

        auto s2 = ex::then(std::move(s1), [](int x) { return x / 1.5; });
        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<double>>>(s2);
        check_error_types<hpx::variant<std::exception_ptr>>(s2);
        check_sends_stopped<false>(s2);

        auto s3 =
            ex::then(std::move(s2), [](double x) -> int { return int(x / 2); });
        static_assert(ex::is_sender_v<decltype(s3)>);
        static_assert(ex::is_sender_in_v<decltype(s3), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<int>>>(s3);
        check_error_types<hpx::variant<std::exception_ptr>>(s3);
        check_sends_stopped<false>(s3);

        auto s4 =
            ex::then(std::move(s3), [](int x) { return std::to_string(x); });
        static_assert(ex::is_sender_v<decltype(s4)>);
        static_assert(ex::is_sender_in_v<decltype(s4), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::string>>>(s4);
        check_error_types<hpx::variant<std::exception_ptr>>(s4);
        check_sends_stopped<false>(s4);

        auto f = [](std::string x) { HPX_TEST_EQ(x, std::string("1")); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s4), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // operator| overload
    {
        std::atomic<bool> set_value_called{false};
        auto s = ex::just() | ex::then([]() { return 3; }) |
            ex::then([](int x) { return x / 1.5; }) |
            ex::then([](double x) -> int { return int(x / 2); }) |
            ex::then([](int x) { return std::to_string(x); });
        static_assert(ex::is_sender_v<decltype(s)>);
        static_assert(ex::is_sender_in_v<decltype(s), ex::empty_env>);

        check_value_types<hpx::variant<hpx::tuple<std::string>>>(s);
        check_error_types<hpx::variant<std::exception_ptr>>(s);
        check_sends_stopped<false>(s);

        auto f = [](std::string x) { HPX_TEST_EQ(x, std::string("1")); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), r);
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // tag_invoke overload
    {
        std::atomic<bool> receiver_set_value_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        std::atomic<bool> custom_transformer_call_operator_called{false};
        auto s = ex::then(ex::just(),
            custom_transformer{tag_invoke_overload_called,
                custom_transformer_call_operator_called, false});
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
        HPX_TEST(tag_invoke_overload_called);
        HPX_TEST(custom_transformer_call_operator_called);
    }

    // Failure path
    {
        std::atomic<bool> set_error_called{false};
        auto s =
            ex::then(ex::just(), [] { throw std::runtime_error("error"); });
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
        auto s1 = ex::then(ex::just(0), [](int x) { return ++x; });
        static_assert(ex::is_sender_v<decltype(s1)>);
        static_assert(ex::is_sender_in_v<decltype(s1), ex::empty_env>);

        auto s2 = ex::then(std::move(s1), [](int x) {
            throw std::runtime_error("error");
            return ++x;
        });
        static_assert(ex::is_sender_v<decltype(s2)>);
        static_assert(ex::is_sender_in_v<decltype(s2), ex::empty_env>);

        auto s3 = ex::then(std::move(s2), [](int x) {
            HPX_TEST(false);
            return ++x;
        });
        static_assert(ex::is_sender_v<decltype(s3)>);
        static_assert(ex::is_sender_in_v<decltype(s3), ex::empty_env>);

        auto s4 = ex::then(std::move(s3), [](int x) {
            HPX_TEST(false);
            return ++x;
        });
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
        std::atomic<bool> custom_transformer_call_operator_called{false};
        auto s = ex::then(ex::just(),
            custom_transformer{tag_invoke_overload_called,
                custom_transformer_call_operator_called, true});
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
        HPX_TEST(tag_invoke_overload_called);
        HPX_TEST(custom_transformer_call_operator_called);
    }

    test_execution_then_return_int();
    test_execution_then_return_void();
    test_execution_then_return_int_shared();
    test_execution_then_return_void_shared();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
