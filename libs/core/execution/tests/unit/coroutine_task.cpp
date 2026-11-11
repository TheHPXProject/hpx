//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
// 1. Basic Value & Void Coroutines
ex::task<int> basic_value_coroutine(int val)
{
    co_return val + 1;
}

ex::task<void> basic_void_coroutine(bool& executed)
{
    executed = true;
    co_return;
}

void test_basic_task()
{
    // Non-void task
    {
        auto f = basic_value_coroutine(41);
        auto res = tt::sync_wait(HPX_MOVE(f));
        HPX_TEST(res.has_value());
        HPX_TEST_EQ(hpx::get<0>(res.value()), 42);
    }

    // Void task
    {
        bool executed = false;
        auto f = basic_void_coroutine(executed);
        auto res = tt::sync_wait(HPX_MOVE(f));
        HPX_TEST(res.has_value());
        HPX_TEST(executed);
    }
}

///////////////////////////////////////////////////////////////////////////////
// 2. Awaiting Senders and Nested Tasks
ex::task<int> nested_child(int x)
{
    co_return x * 2;
}

ex::task<int> sender_awaiting_coroutine(int input)
{
    // Await standard P2300 senders
    int v1 = co_await ex::just(input + 10);
    int v2 = co_await (ex::just(5) | ex::then([](int n) { return n * 3; }));

    // Await child coroutine task
    int v3 = co_await nested_child(4);

    co_return v1 + v2 + v3;
}

void test_sender_and_nested_await()
{
    auto t = sender_awaiting_coroutine(5);
    auto res = tt::sync_wait(HPX_MOVE(t));
    HPX_TEST(res.has_value());
    // v1 = 15, v2 = 15, v3 = 8 => total 38
    HPX_TEST_EQ(hpx::get<0>(res.value()), 38);
}

///////////////////////////////////////////////////////////////////////////////
// 3. Multi-threaded Task Execution with when_all
ex::task<int> concurrent_worker(int id)
{
    co_await ex::just();
    co_return id * 10;
}

void test_concurrent_tasks()
{
    auto t1 = concurrent_worker(1);
    auto t2 = concurrent_worker(2);
    auto t3 = concurrent_worker(3);

    auto work = ex::when_all(HPX_MOVE(t1), HPX_MOVE(t2), HPX_MOVE(t3));
    auto res = tt::sync_wait(HPX_MOVE(work));

    HPX_TEST(res.has_value());
    HPX_TEST_EQ(hpx::get<0>(res.value()), 10);
    HPX_TEST_EQ(hpx::get<1>(res.value()), 20);
    HPX_TEST_EQ(hpx::get<2>(res.value()), 30);
}

///////////////////////////////////////////////////////////////////////////////
// 5. P3552 Custom Allocator Awareness
struct test_alloc_state
{
    std::size_t allocated = 0;
    std::size_t deallocated = 0;
};

template <typename T>
struct test_counting_allocator
{
    using value_type = T;
    test_alloc_state* state_;

    explicit test_counting_allocator(test_alloc_state* s) noexcept
      : state_(s)
    {
    }

    template <typename U>
    test_counting_allocator(test_counting_allocator<U> const& other) noexcept
      : state_(other.state_)
    {
    }

    T* allocate(std::size_t n)
    {
        state_->allocated += n * sizeof(T);
        return static_cast<T*>(std::malloc(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        state_->deallocated += n * sizeof(T);
        std::free(p);
    }

    bool operator==(test_counting_allocator const& other) const noexcept
    {
        return state_ == other.state_;
    }
};

ex::task<int> coroutine_with_custom_alloc(
    std::allocator_arg_t, test_counting_allocator<std::byte>, int val)
{
    co_return val * 3;
}

struct member_coro_class
{
    ex::task<int> member_coroutine_with_alloc(
        std::allocator_arg_t, test_counting_allocator<std::byte>, int val)
    {
        co_return val * 4;
    }
};

struct custom_alloc_env
{
    using allocator_type = test_counting_allocator<std::byte>;
};

ex::task<int, custom_alloc_env> coroutine_checking_allocator(
    std::allocator_arg_t, test_counting_allocator<std::byte> alloc, int val)
{
    auto queried_alloc = co_await ex::get_allocator();
    HPX_TEST(queried_alloc == alloc);
    co_return val * 5;
}

void test_allocator_awareness()
{
    test_alloc_state state{};
    test_counting_allocator<std::byte> alloc(&state);

    {
        auto t = coroutine_with_custom_alloc(std::allocator_arg, alloc, 7);
        HPX_TEST(state.allocated > 0);
        auto res = tt::sync_wait(HPX_MOVE(t));
        HPX_TEST(res.has_value());
        HPX_TEST_EQ(hpx::get<0>(res.value()), 21);
    }

    // Member function coroutine using allocator
    {
        member_coro_class obj;
        auto t = obj.member_coroutine_with_alloc(std::allocator_arg, alloc, 5);
        auto res = tt::sync_wait(HPX_MOVE(t));
        HPX_TEST(res.has_value());
        HPX_TEST_EQ(hpx::get<0>(res.value()), 20);
    }

    // Test get_allocator query retrieves the exact allocator passed
    {
        auto t = coroutine_checking_allocator(std::allocator_arg, alloc, 3);
        auto res = tt::sync_wait(HPX_MOVE(t));
        HPX_TEST(res.has_value());
        HPX_TEST_EQ(hpx::get<0>(res.value()), 15);
    }

    HPX_TEST(state.deallocated > 0);
    HPX_TEST_EQ(state.allocated, state.deallocated);
}

///////////////////////////////////////////////////////////////////////////////
// 6. Exception Propagation and Error Channel
ex::task<int> throwing_coroutine()
{
    co_await ex::just();
    throw std::runtime_error("coroutine error");
    co_return 0;
}

ex::task<int> yielding_error_coroutine()
{
    co_await ex::just();
    co_yield ex::with_error(
        std::make_exception_ptr(std::runtime_error("yielded error")));
    co_return 0;
}

struct custom_error_type
{
    int code;
};

struct custom_error_task_env
{
    using error_types = ex::completion_signatures<
        ex::set_error_t(custom_error_type),
        ex::set_error_t(std::exception_ptr)>;
};

ex::task<int, custom_error_task_env> yielding_custom_error_coroutine()
{
    co_await ex::just();
    co_yield ex::with_error(custom_error_type{123});
    co_return 0;
}

struct test_custom_error_receiver
{
    using receiver_concept = ex::receiver_t;
    bool* error_called_;
    int* error_code_;

    void set_value(int) noexcept {}
    void set_stopped() noexcept {}

    void set_error(custom_error_type err) noexcept
    {
        *error_called_ = true;
        *error_code_ = err.code;
    }

    void set_error(std::exception_ptr) noexcept {}

    struct env
    {
    };

    env get_env() const noexcept
    {
        return {};
    }
};

void test_exception_handling()
{
    bool caught_exception = false;
    try
    {
        auto t = throwing_coroutine();
        (void) tt::sync_wait(HPX_MOVE(t));
    }
    catch (std::runtime_error const& e)
    {
        caught_exception = true;
        HPX_TEST_EQ(std::string(e.what()), std::string("coroutine error"));
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(caught_exception);

    // Test co_yield with_error with std::exception_ptr
    bool caught_yield_error = false;
    try
    {
        auto t = yielding_error_coroutine();
        (void) tt::sync_wait(HPX_MOVE(t));
    }
    catch (std::runtime_error const& e)
    {
        caught_yield_error = true;
        HPX_TEST_EQ(std::string(e.what()), std::string("yielded error"));
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(caught_yield_error);

    // Test co_yield with_error with custom typed error via connect/start
    {
        bool error_called = false;
        int error_code = 0;
        auto t = yielding_custom_error_coroutine();
        auto op = ex::connect(
            HPX_MOVE(t), test_custom_error_receiver{&error_called, &error_code});
        ex::start(op);
        HPX_TEST(error_called);
        HPX_TEST_EQ(error_code, 123);
    }
}

///////////////////////////////////////////////////////////////////////////////
// 7. Cancellation Protocol via in_place_stop_source & just_stopped
ex::task<int> cancellable_coroutine()
{
    co_await ex::just();
    co_return 42;
}

ex::task<int> task_awaiting_just_stopped()
{
    co_await ex::just_stopped();
    co_return 42;
}

ex::task<int> nested_inner_just_stopped()
{
    co_await ex::just_stopped();
    co_return 100;
}

ex::task<int> nested_outer_just_stopped()
{
    int val = co_await nested_inner_just_stopped();
    co_return val + 1;
}

struct wait_for_stop_sender
{
    using is_sender = void;
    using sender_concept = ex::sender_t;

    using completion_signatures =
        ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>;

    template <typename Receiver>
    struct operation_state
    {
        using receiver_type = std::decay_t<Receiver>;
        receiver_type rcvr_;

        using stop_token_type =
            ex::stop_token_of_t<ex::env_of_t<receiver_type>>;

        struct stop_cb
        {
            operation_state* op_;
            void operator()() noexcept
            {
                ex::set_stopped(HPX_MOVE(op_->rcvr_));
            }
        };

        using callback_type =
            typename stop_token_type::template callback_type<stop_cb>;
        std::optional<callback_type> cb_{};

        explicit operation_state(Receiver rcvr)
          : rcvr_(HPX_MOVE(rcvr))
        {
        }

        void start() & noexcept
        {
            auto token = ex::get_stop_token(ex::get_env(rcvr_));
            if (token.stop_requested())
            {
                ex::set_stopped(HPX_MOVE(rcvr_));
            }
            else if (token.stop_possible())
            {
                cb_.emplace(token, stop_cb{this});
            }
        }
    };

    template <typename Receiver>
    operation_state<Receiver> connect(Receiver&& rcvr) const
    {
        return operation_state<Receiver>{HPX_FORWARD(Receiver, rcvr)};
    }
};

ex::task<int> inflight_cancellable_coroutine()
{
    co_await wait_for_stop_sender{};
    co_return 99;
}

struct test_stopped_receiver
{
    using receiver_concept = ex::receiver_t;
    bool* stopped_called_;
    bool* value_called_{nullptr};
    int* value_{nullptr};
    hpx::experimental::in_place_stop_token token_;

    test_stopped_receiver(
        bool* stopped_called, hpx::experimental::in_place_stop_token token)
      : stopped_called_(stopped_called)
      , token_(token)
    {
    }

    test_stopped_receiver(bool* stopped_called, bool* value_called, int* value,
        hpx::experimental::in_place_stop_token token)
      : stopped_called_(stopped_called)
      , value_called_(value_called)
      , value_(value)
      , token_(token)
    {
    }

    template <typename... Args>
    void set_value(Args&&... args) noexcept
    {
        if (value_called_)
        {
            *value_called_ = true;
        }
        if (value_ && sizeof...(args) == 1)
        {
            *value_ = (args, ...);
        }
    }

    void set_error(std::exception_ptr) noexcept {}

    void set_stopped() noexcept
    {
        *stopped_called_ = true;
    }

    struct env
    {
        hpx::experimental::in_place_stop_token token_;

        auto query(ex::get_stop_token_t) const noexcept
        {
            return token_;
        }
    };

    env get_env() const noexcept
    {
        return env{token_};
    }
};

void test_cancellation()
{
    // Initial pre-cancelled stop source: task runs to completion and delivers value
    {
        hpx::experimental::in_place_stop_source stop_source;
        stop_source.request_stop();

        bool stopped_called = false;
        bool value_called = false;
        int value = 0;
        test_stopped_receiver rcvr{
            &stopped_called, &value_called, &value, stop_source.get_token()};

        auto t = cancellable_coroutine();
        auto op = ex::connect(HPX_MOVE(t), rcvr);
        ex::start(op);

        HPX_TEST(!stopped_called);
        HPX_TEST(value_called);
        HPX_TEST_EQ(value, 42);
    }

    // 1. A task that directly awaits just_stopped()
    {
        bool stopped_called = false;
        hpx::experimental::in_place_stop_source stop_source;
        test_stopped_receiver rcvr{&stopped_called, stop_source.get_token()};

        auto t = task_awaiting_just_stopped();
        auto op = ex::connect(HPX_MOVE(t), rcvr);
        ex::start(op);

        HPX_TEST(stopped_called);

        // Also verify via sync_wait
        auto res = tt::sync_wait(task_awaiting_just_stopped());
        HPX_TEST(!res.has_value());
    }

    // 2. Nested task: inner awaits just_stopped(), outer awaits inner
    {
        auto t = nested_outer_just_stopped();
        auto res = tt::sync_wait(HPX_MOVE(t));
        HPX_TEST(!res.has_value());
    }

    // 3. In-flight cancellation: request_stop() called after start()
    {
        hpx::experimental::in_place_stop_source stop_source;
        bool stopped_called = false;
        test_stopped_receiver rcvr{&stopped_called, stop_source.get_token()};

        auto t = inflight_cancellable_coroutine();
        auto op = ex::connect(HPX_MOVE(t), rcvr);
        ex::start(op);

        // Task suspended inside co_await wait_for_stop_sender{}
        HPX_TEST(!stopped_called);

        // Cancel in-flight
        stop_source.request_stop();
        HPX_TEST(stopped_called);
    }
}

ex::task<int&> reference_coroutine(int& val)
{
    co_return val;
}

ex::task<void> consumer_coroutine(int& val)
{
    int& r = co_await reference_coroutine(val);
    HPX_TEST_EQ(&r, &val);
    r = 456;
}

void test_reference_task()
{
    int val = 123;
    int* received_ptr = nullptr;
    auto t1 = reference_coroutine(val) | ex::then([&](int& r) {
        received_ptr = &r;
        HPX_TEST_EQ(r, 123);
    });
    auto res1 = tt::sync_wait(HPX_MOVE(t1));
    HPX_TEST(res1.has_value());
    HPX_TEST_EQ(received_ptr, &val);

    auto t2 = consumer_coroutine(val);
    auto res2 = tt::sync_wait(HPX_MOVE(t2));
    HPX_TEST(res2.has_value());
    HPX_TEST_EQ(val, 456);
}

///////////////////////////////////////////////////////////////////////////////
// 9. Environment Traits and Query Forwarding (P3552R3)
struct custom_task_env
{
    using allocator_type = std::allocator<int>;
    using scheduler_type = ex::task_scheduler;
    using stop_source_type = hpx::experimental::in_place_stop_source;
};

struct query_env
{
};

inline constexpr struct my_forwarding_query_t : ex::forwarding_query_t
{
    constexpr int operator()(query_env const&) const noexcept
    {
        return 42;
    }
} my_forwarding_query{};

inline constexpr struct my_non_forwarding_query_t
{
    constexpr int operator()(query_env const&) const noexcept
    {
        return 99;
    }
} my_non_forwarding_query{};

ex::task<int, query_env> query_env_coroutine()
{
    co_return 10;
}

void test_environment_traits()
{
    using default_traits = ex::task<int>::promise_type::traits_type;
    static_assert(std::is_same_v<default_traits::allocator_type,
        std::allocator<std::byte>>);
    static_assert(
        std::is_same_v<default_traits::scheduler_type, ex::task_scheduler>);
    static_assert(std::is_same_v<default_traits::stop_source_type,
        hpx::experimental::in_place_stop_source>);

    using custom_traits =
        ex::task<int, custom_task_env>::promise_type::traits_type;
    static_assert(
        std::is_same_v<custom_traits::allocator_type, std::allocator<int>>);
    static_assert(
        std::is_same_v<custom_traits::scheduler_type, ex::task_scheduler>);

    // Verify forwarding_query participation
    using promise_t = ex::task<int, query_env>::promise_type;
    static_assert(requires(promise_t::env_type const& e) {
        e.query(my_forwarding_query);
    });

    auto t = query_env_coroutine();
    auto res = tt::sync_wait(HPX_MOVE(t));
    HPX_TEST(res.has_value());
    HPX_TEST_EQ(hpx::get<0>(res.value()), 10);
}

int hpx_main()
{
    test_basic_task();
    test_sender_and_nested_await();
    test_concurrent_tasks();
    test_allocator_awareness();
    test_exception_handling();
    test_cancellation();
    test_reference_task();
    test_environment_traits();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
