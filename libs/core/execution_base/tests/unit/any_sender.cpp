//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <exception>
#include <string>
#include <type_traits>
#include <utility>

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace ex = hpx::execution::experimental;

struct custom_type_non_copyable
{
    int x;

    custom_type_non_copyable(int x)
      : x(x)
    {
    }
    custom_type_non_copyable() = default;
    custom_type_non_copyable(custom_type_non_copyable&&) = default;
    custom_type_non_copyable& operator=(custom_type_non_copyable&&) = default;
    custom_type_non_copyable(custom_type_non_copyable const&) = delete;
    custom_type_non_copyable& operator=(
        custom_type_non_copyable const&) = delete;
};

class immovable
{
public:
    immovable() = default;
    immovable(immovable const&) = default;
    immovable(immovable&&) = delete;
    immovable& operator=(immovable const&) = default;
    immovable& operator=(immovable&&) = delete;
};

template <typename... Ts>
struct non_copyable_sender
{
    using sender_concept = ex::sender_t;

    hpx::tuple<std::decay_t<Ts>...> ts;

    // clang-format off
    template <typename Env>
    friend auto tag_invoke(ex::get_completion_signatures_t,
        non_copyable_sender const&,
        Env) noexcept -> ex::completion_signatures<ex::set_value_t(Ts...),
                          ex::set_error_t(std::exception_ptr)>;
    // clang-format on

    non_copyable_sender() = default;
    template <typename T,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T>, non_copyable_sender>>>
    non_copyable_sender(T&& t)
      : ts(std::forward<T>(t))
    {
    }
    template <typename T1, typename T2, typename... Ts_>
    non_copyable_sender(T1&& t1, T2&& t2, Ts_&&... ts)
      : ts(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Ts_>(ts)...)
    {
    }
    non_copyable_sender(non_copyable_sender&&) = default;
    non_copyable_sender(non_copyable_sender const&) = delete;
    non_copyable_sender& operator=(non_copyable_sender&&) = default;
    non_copyable_sender& operator=(non_copyable_sender const&) = delete;

    template <typename R>
    struct operation_state : immovable
    {
        std::decay_t<R> r;
        hpx::tuple<std::decay_t<Ts>...> ts;

        friend void tag_invoke(
            hpx::execution::experimental::start_t, operation_state& os) noexcept
        {
            hpx::invoke_fused(
                hpx::bind_front(
                    hpx::execution::experimental::set_value, std::move(os.r)),
                std::move(os.ts));
        };
    };

    template <typename R>
    friend operation_state<R> tag_invoke(
        hpx::execution::experimental::connect_t, non_copyable_sender&& s,
        R&& r) noexcept
    {
        return {{}, std::forward<R>(r), std::move(s.ts)};
    }
};

template <typename... Ts>
struct example_sender
{
    hpx::tuple<std::decay_t<Ts>...> ts;

    using is_sender = void;
    using sender_concept = ex::sender_t;

    // clang-format off
    template <typename Env>
    friend auto tag_invoke(ex::get_completion_signatures_t,
        example_sender const&,
        Env) noexcept -> ex::completion_signatures<ex::set_value_t(Ts...),
                          ex::set_error_t(std::exception_ptr)>;
    // clang-format on

    example_sender() = default;
    template <typename T,
        typename =
            std::enable_if_t<!std::is_same_v<std::decay_t<T>, example_sender>>>
    example_sender(T&& t)
      : ts(std::forward<T>(t))
    {
    }
    template <typename T1, typename T2, typename... Ts_>
    example_sender(T1&& t1, T2&& t2, Ts_&&... ts)
      : ts(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Ts_>(ts)...)
    {
    }
    example_sender(example_sender&&) = default;
    example_sender(example_sender const&) = default;
    example_sender& operator=(example_sender&&) = default;
    example_sender& operator=(example_sender const&) = default;

    template <typename R>
    struct operation_state : immovable
    {
        std::decay_t<R> r;
        hpx::tuple<std::decay_t<Ts>...> ts;

        friend void tag_invoke(
            hpx::execution::experimental::start_t, operation_state& os) noexcept
        {
            hpx::invoke_fused(
                hpx::bind_front(
                    hpx::execution::experimental::set_value, std::move(os.r)),
                std::move(os.ts));
        };
    };

    template <typename R>
    friend operation_state<R> tag_invoke(
        hpx::execution::experimental::connect_t, example_sender&& s, R&& r)
    {
        return {{}, std::forward<R>(r), std::move(s.ts)};
    }

    template <typename R>
    friend operation_state<R> tag_invoke(
        hpx::execution::experimental::connect_t, example_sender& s, R&& r)
    {
        return {{}, std::forward<R>(r), s.ts};
    }
};

template <typename... Ts>
struct large_non_copyable_sender : non_copyable_sender<Ts...>
{
    // This padding only needs to be larger than the embedded storage in
    // any_sender. Adjust if needed.
    unsigned char padding[128] = {0};

    large_non_copyable_sender() = default;
    template <typename T,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T>, large_non_copyable_sender>>>
    large_non_copyable_sender(T&& t)
      : non_copyable_sender<Ts...>(std::forward<T>(t))
    {
    }
    template <typename T1, typename T2, typename... Ts_>
    large_non_copyable_sender(T1&& t1, T2&& t2, Ts_&&... ts)
      : non_copyable_sender<Ts...>(std::forward<T1>(t1), std::forward<T2>(t2),
            std::forward<Ts_>(ts)...)
    {
    }
    large_non_copyable_sender(large_non_copyable_sender&&) = default;
    large_non_copyable_sender(large_non_copyable_sender const&) = delete;
    large_non_copyable_sender& operator=(large_non_copyable_sender&&) = default;
    large_non_copyable_sender& operator=(
        large_non_copyable_sender const&) = delete;
};

template <typename... Ts>
struct large_sender : example_sender<Ts...>
{
    // This padding only needs to be larger than the embedded storage in
    // any_sender. Adjust if needed.
    unsigned char padding[128] = {0};

    large_sender() = default;
    template <typename T,
        typename =
            std::enable_if_t<!std::is_same_v<std::decay_t<T>, large_sender>>>
    large_sender(T&& t)
      : example_sender<Ts...>(std::forward<T>(t))
    {
    }
    template <typename T1, typename T2, typename... Ts_>
    large_sender(T1&& t1, T2&& t2, Ts_&&... ts)
      : example_sender<Ts...>(std::forward<T1>(t1), std::forward<T2>(t2),
            std::forward<Ts_>(ts)...)
    {
    }
    large_sender(large_sender&&) = default;
    large_sender(large_sender const&) = default;
    large_sender& operator=(large_sender&&) = default;
    large_sender& operator=(large_sender const&) = default;
};

struct error_receiver
{
    std::atomic<bool>& set_error_called;

    struct is_receiver
    {
    };

    friend void tag_invoke(hpx::execution::experimental::set_error_t,
        error_receiver&& r, std::exception_ptr&& e) noexcept
    {
        try
        {
            std::rethrow_exception(std::move(e));
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ(std::string(e.what()), std::string("error"));
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        r.set_error_called = true;
    }

    friend void tag_invoke(
        hpx::execution::experimental::set_stopped_t, error_receiver&&) noexcept
    {
        HPX_TEST(false);
    };

    template <typename... Ts>
    friend void tag_invoke(hpx::execution::experimental::set_value_t,
        error_receiver&&, Ts&&...) noexcept
    {
        HPX_TEST(false);
    }
};

template <template <typename...> typename Sender, typename... Ts, typename F>
void test_any_sender(F&& f, Ts&&... ts)
{
    static_assert(std::is_copy_constructible_v<Sender<Ts...>>,
        "This test requires the example_sender to be copy constructible.");

    Sender<std::decay_t<Ts>...> s{std::forward<Ts>(ts)...};

    ex::any_sender<std::decay_t<Ts>...> as1{s};

    static_assert(ex::is_sender_v<decltype(as1)>);
    static_assert(ex::is_sender_in_v<decltype(as1), ex::empty_env>);

    auto as2 = as1;

    static_assert(ex::is_sender_v<decltype(as2)>);
    static_assert(ex::is_sender_in_v<decltype(as2), ex::empty_env>);

    check_value_types<hpx::variant<hpx::tuple<Ts...>>>(as2);
    check_error_types<hpx::variant<std::exception_ptr>>(as2);
    check_sends_stopped<false>(as2);

    // We should be able to connect both as1 and as2 multiple times; set_value
    // should always be called
    {
        std::atomic<bool> set_value_called{false};
        auto os = ex::connect(as1, callback_receiver<F>{f, set_value_called});
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto os = ex::connect(
            std::move(as1), callback_receiver<F>{f, set_value_called});
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto os = ex::connect(as2, callback_receiver<F>{f, set_value_called});
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto os = ex::connect(
            std::move(as2), callback_receiver<F>{f, set_value_called});
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // as1 and as2 have been moved so we always expect an exception here
    {
        std::atomic<bool> set_value_called{false};
        try
        {
            auto os = ex::connect(
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(as1), callback_receiver<F>{f, set_value_called});
            HPX_TEST(false);
            ex::start(os);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_function_call);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(!set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        try
        {
            auto os = ex::connect(
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(as2), callback_receiver<F>{f, set_value_called});
            HPX_TEST(false);
            ex::start(os);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_function_call);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(!set_value_called);
    }
}

template <template <typename...> typename Sender, typename... Ts, typename F>
void test_unique_any_sender(F&& f, Ts&&... ts)
{
    Sender<std::decay_t<Ts>...> s{std::forward<Ts>(ts)...};

    ex::unique_any_sender<std::decay_t<Ts>...> as1{std::move(s)};

    static_assert(ex::is_sender_v<decltype(as1)>);
    static_assert(ex::is_sender_in_v<decltype(as1), ex::empty_env>);

    check_value_types<hpx::variant<hpx::tuple<Ts...>>>(as1);
    check_error_types<hpx::variant<std::exception_ptr>>(as1);
    check_sends_stopped<false>(as1);

    auto as2 = std::move(as1);

    static_assert(ex::is_sender_v<decltype(as2)>);
    static_assert(ex::is_sender_in_v<decltype(as2), ex::empty_env>);

    check_value_types<hpx::variant<hpx::tuple<Ts...>>>(as2);
    check_error_types<hpx::variant<std::exception_ptr>>(as2);
    check_sends_stopped<false>(as2);

    // We expect set_value to be called here
    {
        std::atomic<bool> set_value_called = false;
        auto os = ex::connect(
            std::move(as2), callback_receiver<F>{f, set_value_called});
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // as1 has been moved, so we always expect an exception here
    {
        std::atomic<bool> set_value_called{false};
        try
        {
            auto os = ex::connect(
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(as1), callback_receiver<F>{f, set_value_called});
            HPX_TEST(false);
            ex::start(os);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_function_call);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(!set_value_called);
    }
}

void test_any_sender_set_error()
{
    error_sender<> s;

    ex::any_sender<> as1{std::move(s)};
    auto as2 = as1;

    // We should be able to connect the example_sender multiple times; set_error should
    // always be called
    {
        std::atomic<bool> set_error_called{false};
        auto os = ex::connect(as1, error_receiver{set_error_called});
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        auto os = ex::connect(std::move(as1), error_receiver{set_error_called});
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        auto os = ex::connect(as2, error_receiver{set_error_called});
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        auto os = ex::connect(std::move(as2), error_receiver{set_error_called});
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    // as1 and as2 have been moved so we always expect an exception here
    {
        std::atomic<bool> set_error_called{false};
        try
        {
            auto os =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                ex::connect(std::move(as1), error_receiver{set_error_called});
            HPX_TEST(false);
            ex::start(os);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_function_call);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(!set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        try
        {
            auto os =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                ex::connect(std::move(as2), error_receiver{set_error_called});
            HPX_TEST(false);
            ex::start(os);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_function_call);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(!set_error_called);
    }
}

void test_unique_any_sender_set_error()
{
    error_sender s;

    ex::unique_any_sender<> as1{std::move(s)};
    auto as2 = std::move(as1);

    // We expect set_error to be called here
    {
        std::atomic<bool> set_error_called{false};
        auto os = ex::connect(std::move(as2), error_receiver{set_error_called});
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    // as1 has been moved, so we always expect an exception here
    {
        std::atomic<bool> set_error_called{false};
        try
        {
            auto os =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                ex::connect(std::move(as1), error_receiver{set_error_called});
            HPX_TEST(false);
            ex::start(os);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_function_call);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(!set_error_called);
    }
}

// This tests that the empty vtable types used in the implementation of any_*
// are not destroyed too early. We use ensure_started inside the function to
// trigger the use of the empty vtables for any_receiver and
// any_operation_state. If the empty vtables are function-local
// static variables they would get constructed after s_global
// is constructed, and thus destroyed before s_global is destroyed. This will
// typically lead to a segfault. If the empty vtables are (constant) global
// variables they should be constructed before s_global is constructed and
// destroyed after s_global is destroyed.
ex::unique_any_sender<> global_unique_any_sender{ex::just()};
ex::any_sender<> global_any_sender{ex::just()};

// This helper only makes sure that the sender returned from split is actually
// started before being destructed.
struct wait_globals
{
    ~wait_globals()
    {
        hpx::this_thread::experimental::sync_wait(std::move(global_any_sender));
    }
} waiter{};

void test_globals()
{
    global_unique_any_sender =
        std::move(global_unique_any_sender) | ex::ensure_started();
    // P2300R8's ensure started returns non-copyable senders, so we need to split
    // to pass it to a global_any_sender.
    global_any_sender =
        std::move(global_any_sender) | ex::ensure_started() | ex::split();
}

int main()
{
    // We can only wrap copyable senders in any_sender
    test_any_sender<example_sender>([] {});
    test_any_sender<example_sender, int>([](int x) { HPX_TEST_EQ(x, 42); }, 42);
    test_any_sender<example_sender, int, double>(
        [](int x, double y) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
        },
        42, 3.14);

    test_any_sender<large_sender>([] {});
    test_any_sender<large_sender, int>([](int x) { HPX_TEST_EQ(x, 42); }, 42);
    test_any_sender<large_sender, int, double>(
        [](int x, double y) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
        },
        42, 3.14);

    // We can wrap both copyable and non-copyable senders in unique_any_sender
    test_unique_any_sender<example_sender>([] {});
    test_unique_any_sender<example_sender, int>(
        [](int x) { HPX_TEST_EQ(x, 42); }, 42);
    test_unique_any_sender<example_sender, int, double>(
        [](int x, double y) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
        },
        42, 3.14);

    test_unique_any_sender<large_sender>([] {});
    test_unique_any_sender<large_sender, int>(
        [](int x) { HPX_TEST_EQ(x, 42); }, 42);
    test_unique_any_sender<large_sender, int, double>(
        [](int x, double y) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
        },
        42, 3.14);

    test_unique_any_sender<non_copyable_sender>([] {});
    test_unique_any_sender<non_copyable_sender, int>(
        [](int x) { HPX_TEST_EQ(x, 42); }, 42);
    test_unique_any_sender<non_copyable_sender, int, double>(
        [](int x, double y) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
        },
        42, 3.14);
    test_unique_any_sender<non_copyable_sender, int, double,
        custom_type_non_copyable>(
        [](int x, double y, custom_type_non_copyable z) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
            HPX_TEST_EQ(z.x, 43);
        },
        42, 3.14, custom_type_non_copyable(43));

    test_unique_any_sender<large_non_copyable_sender>([] {});
    test_unique_any_sender<large_non_copyable_sender, int>(
        [](int x) { HPX_TEST_EQ(x, 42); }, 42);
    test_unique_any_sender<large_non_copyable_sender, int, double>(
        [](int x, double y) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
        },
        42, 3.14);
    test_unique_any_sender<large_non_copyable_sender, int, double,
        custom_type_non_copyable>(
        [](int x, double y, custom_type_non_copyable z) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, 3.14);
            HPX_TEST_EQ(z.x, 43);
        },
        42, 3.14, custom_type_non_copyable(43));

    // Failure paths
    test_any_sender_set_error();
    test_unique_any_sender_set_error();

    // Test use of *any_* in globals
    test_globals();

    return hpx::util::report_errors();
}

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
