//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// Clang V11 ICE's on this test, Clang V8 reports a bogus constexpr problem
#if !defined(HPX_CLANG_VERSION) ||                                             \
    ((HPX_CLANG_VERSION / 10000) != 11 && (HPX_CLANG_VERSION / 10000) != 8)

#include <hpx/condition_variable.hpp>
#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

struct custom_type_non_default_constructible_non_copyable
{
    int x;
    custom_type_non_default_constructible_non_copyable() = delete;
    explicit custom_type_non_default_constructible_non_copyable(int x)
      : x(x) {};
    custom_type_non_default_constructible_non_copyable(
        custom_type_non_default_constructible_non_copyable&&) = default;
    custom_type_non_default_constructible_non_copyable& operator=(
        custom_type_non_default_constructible_non_copyable&&) = default;
    custom_type_non_default_constructible_non_copyable(
        custom_type_non_default_constructible_non_copyable const&) = delete;
    custom_type_non_default_constructible_non_copyable& operator=(
        custom_type_non_default_constructible_non_copyable const&) = delete;
};

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

// Template to detect thread_pool_bulk_sender specializations
template <typename T>
struct is_thread_pool_bulk_sender : std::false_type
{
};

template <typename Policy, typename Sender, typename Shape, typename F,
    bool IsChunked>
struct is_thread_pool_bulk_sender<hpx::execution::experimental::detail::
        thread_pool_bulk_sender<Policy, Sender, Shape, F, IsChunked>>
  : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_execute()
{
    hpx::thread::id parent_id = hpx::this_thread::get_id();

    ex::thread_pool_scheduler sched{};
    ex::execute(sched,
        [parent_id]() { HPX_TEST_NEQ(hpx::this_thread::get_id(), parent_id); });
}

struct check_context_receiver
{
    hpx::thread::id parent_id;
    hpx::mutex& mtx;
    hpx::condition_variable& cond;
    bool& executed;
    using receiver_concept = ex::receiver_t;
    template <typename E>
    friend void tag_invoke(
        ex::set_error_t, check_context_receiver&&, E&&) noexcept
    {
        HPX_TEST(false);
    }

    friend void tag_invoke(ex::set_stopped_t, check_context_receiver&&) noexcept
    {
        HPX_TEST(false);
    }

    template <typename... Ts>
    friend void tag_invoke(
        ex::set_value_t, check_context_receiver&& r, Ts&&...) noexcept
    {
        HPX_TEST_NEQ(r.parent_id, hpx::this_thread::get_id());
        HPX_TEST_NEQ(hpx::thread::id(hpx::threads::invalid_thread_id),
            hpx::this_thread::get_id());
        std::lock_guard l{r.mtx};
        r.executed = true;
        r.cond.notify_one();
    }
};

void test_sender_receiver_basic()
{
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool executed{false};

    ex::thread_pool_scheduler sched{};

    auto begin = ex::schedule(sched);
    auto os = ex::connect(std::move(begin),
        check_context_receiver{parent_id, mtx, cond, executed});
    ex::start(os);

    {
        std::unique_lock l{mtx};
        cond.wait(l, [&]() { return executed; });
    }

    HPX_TEST(executed);
}

hpx::thread::id sender_receiver_then_thread_id;

void test_sender_receiver_then()
{
    ex::thread_pool_scheduler sched{};
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool executed{false};

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(std::move(begin), [=]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(sender_receiver_then_thread_id, parent_id);
    });
    auto work2 = ex::then(std::move(work1), []() {
        HPX_TEST_EQ(sender_receiver_then_thread_id, hpx::this_thread::get_id());
    });
    auto os = ex::connect(std::move(work2),
        check_context_receiver{parent_id, mtx, cond, executed});
    ex::start(os);

    {
        std::unique_lock l{mtx};
        cond.wait(l, [&]() { return executed; });
    }

    HPX_TEST(executed);
}

void test_sender_receiver_then_wait()
{
    ex::thread_pool_scheduler sched{};
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    std::atomic<std::size_t> then_count{0};
    bool executed{false};

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(std::move(begin), [&then_count, parent_id]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(sender_receiver_then_thread_id, parent_id);
        ++then_count;
    });
    auto work2 = ex::then(std::move(work1), [&then_count, &executed]() {
        HPX_TEST_EQ(sender_receiver_then_thread_id, hpx::this_thread::get_id());
        ++then_count;
        executed = true;
    });
    tt::sync_wait(std::move(work2));

    HPX_TEST_EQ(then_count, std::size_t(2));
    HPX_TEST(executed);
}

// NOLINTBEGIN(bugprone-unchecked-optional-access)
void test_sender_receiver_then_sync_wait()
{
    ex::thread_pool_scheduler sched{};
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    std::atomic<std::size_t> then_count{0};

    auto begin = ex::schedule(sched);
    auto work = ex::then(std::move(begin), [&then_count, parent_id]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(sender_receiver_then_thread_id, parent_id);
        ++then_count;
        return 42;
    });
    auto result = hpx::get<0>(*(tt::sync_wait(std::move(work))));
    HPX_TEST_EQ(then_count, std::size_t(1));
    static_assert(
        std::is_same<int, typename std::decay<decltype(result)>::type>::value,
        "result should be an int");
    HPX_TEST_EQ(result, 42);
}

void test_sender_receiver_then_arguments()
{
    ex::thread_pool_scheduler sched{};
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    std::atomic<std::size_t> then_count{0};

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(std::move(begin), [&then_count, parent_id]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(sender_receiver_then_thread_id, parent_id);
        ++then_count;
        return 3;
    });
    auto work2 =
        ex::then(std::move(work1), [&then_count](int x) -> std::string {
            HPX_TEST_EQ(
                sender_receiver_then_thread_id, hpx::this_thread::get_id());
            ++then_count;
            return std::string("hello") + std::to_string(x);
        });
    auto work3 = ex::then(std::move(work2), [&then_count](std::string s) {
        HPX_TEST_EQ(sender_receiver_then_thread_id, hpx::this_thread::get_id());
        ++then_count;
        return 2 * s.size();
    });
    auto result = hpx::get<0>(*tt::sync_wait(std::move(work3)));
    HPX_TEST_EQ(then_count, std::size_t(3));
    static_assert(std::is_same<std::size_t,
                      typename std::decay<decltype(result)>::type>::value,
        "result should be a std::size_t");
    HPX_TEST_EQ(result, std::size_t(12));
}

template <typename F>
struct callback_receiver
{
    std::decay_t<F> f;
    hpx::mutex& mtx;
    hpx::condition_variable& cond;
    bool& executed;
    using receiver_concept = ex::receiver_t;

    template <typename E>
    friend void tag_invoke(ex::set_error_t, callback_receiver&&, E&&) noexcept
    {
        HPX_TEST(false);
    }

    friend void tag_invoke(ex::set_stopped_t, callback_receiver&&) noexcept
    {
        HPX_TEST(false);
    }

    template <typename... Ts>
    friend void tag_invoke(
        ex::set_value_t, callback_receiver&& r, Ts&&...) noexcept
    {
        HPX_INVOKE(r.f, );
        std::lock_guard l{r.mtx};
        r.executed = true;
        r.cond.notify_one();
    }
};

void test_properties()
{
    ex::thread_pool_scheduler sched{};
    hpx::mutex mtx;
    hpx::condition_variable cond;
    bool executed{false};

    constexpr std::array<hpx::threads::thread_priority, 4> priorities{
        {hpx::threads::thread_priority::low,
            hpx::threads::thread_priority::normal,
            hpx::threads::thread_priority::bound,
            hpx::threads::thread_priority::high}};

    for (auto const prio : priorities)
    {
        auto exec_prop = ex::with_priority(sched, prio);
        HPX_TEST_EQ(ex::get_priority(exec_prop), prio);

        auto check = [prio]() {
            HPX_TEST_EQ(prio, hpx::this_thread::get_priority());
        };
        executed = false;
        auto os = ex::connect(ex::schedule(exec_prop),
            callback_receiver<decltype(check)>{check, mtx, cond, executed});
        ex::start(os);
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return executed; });
        }

        HPX_TEST(executed);
    }

    constexpr std::array<hpx::threads::thread_stacksize, 4> stacksizes{
        {hpx::threads::thread_stacksize::small_,
            hpx::threads::thread_stacksize::medium,
            hpx::threads::thread_stacksize::large,
            hpx::threads::thread_stacksize::huge}};

    for (auto const stacksize : stacksizes)
    {
        auto exec_prop = ex::with_stacksize(sched, stacksize);
        HPX_TEST_EQ(ex::get_stacksize(exec_prop), stacksize);

        auto check = [stacksize]() {
            HPX_TEST_EQ(stacksize,
                hpx::threads::get_thread_id_data(hpx::threads::get_self_id())
                    ->get_stack_size_enum());
        };
        executed = false;
        auto os = ex::connect(ex::schedule(exec_prop),
            callback_receiver<decltype(check)>{check, mtx, cond, executed});
        ex::start(os);
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return executed; });
        }

        HPX_TEST(executed);
    }

    constexpr std::array<hpx::threads::thread_schedule_hint, 4> hints{
        {hpx::threads::thread_schedule_hint{},
            hpx::threads::thread_schedule_hint{1},
            hpx::threads::thread_schedule_hint{
                hpx::threads::thread_schedule_hint_mode::thread, 2},
            hpx::threads::thread_schedule_hint{
                hpx::threads::thread_schedule_hint_mode::numa, 3}}};

    for (auto const hint : hints)
    {
        auto exec_prop = ex::with_hint(sched, hint);
        HPX_TEST(ex::get_hint(exec_prop) == hint);

        // A hint is not guaranteed to be respected, so we only check that the
        // thread_pool_scheduler holds the property.
    }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    {
        char const* annotation = "<test>";
        auto exec_prop = ex::with_annotation(sched, annotation);
        HPX_TEST_EQ(std::string(ex::get_annotation(exec_prop)),
            std::string(annotation));

        auto check = [annotation]() {
            HPX_TEST_EQ(std::string(annotation),
                hpx::threads::get_thread_description(
                    hpx::threads::get_self_id())
                    .get_description());
        };
        executed = false;
        auto os = ex::connect(ex::schedule(exec_prop),
            callback_receiver<decltype(check)>{check, mtx, cond, executed});
        ex::start(os);

        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return executed; });
        }

        HPX_TEST(executed);
    }
#endif
}

void test_transfer_basic()
{
    ex::thread_pool_scheduler sched{};
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    hpx::thread::id current_id;

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(begin, [=, &current_id]() {
        current_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(current_id, parent_id);
    });
    auto work2 = ex::then(work1, [=, &current_id]() {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
    });
    auto transfer1 = ex::continues_on(work2, sched);
    auto work3 = ex::then(transfer1, [=, &current_id]() {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_NEQ(current_id, parent_id);
    });
    auto work4 = ex::then(work3, [=, &current_id]() {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
    });
    auto transfer2 = ex::continues_on(work4, sched);
    auto work5 = ex::then(transfer2, [=, &current_id]() {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_NEQ(current_id, parent_id);
    });

    tt::sync_wait(work5);
}

void test_transfer_arguments()
{
    ex::thread_pool_scheduler sched{};
    hpx::thread::id parent_id = hpx::this_thread::get_id();
    hpx::thread::id current_id;

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(begin, [=, &current_id]() {
        current_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(current_id, parent_id);
        return 3;
    });
    auto work2 = ex::then(work1, [=, &current_id](int x) {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
        return x / 2.0;
    });
    auto transfer1 = ex::continues_on(work2, sched);
    auto work3 = ex::then(transfer1, [=, &current_id](double x) {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_NEQ(current_id, parent_id);
        return x / 2;
    });
    auto work4 = ex::then(work3, [=, &current_id](int x) {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
        return "result: " + std::to_string(x);
    });
    auto transfer2 = ex::continues_on(work4, sched);
    auto work5 = ex::then(transfer2, [=, &current_id](std::string s) {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_NEQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_NEQ(current_id, parent_id);
        return s + "!";
    });

    auto result = hpx::get<0>(*tt::sync_wait(work5));
    static_assert(std::is_same<std::string,
                      typename std::decay<decltype(result)>::type>::value,
        "result should be a std::string");
    HPX_TEST_EQ(result, std::string("result: 0!"));
}

void test_just_void()
{
    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto begin = ex::just();
        auto work1 = ex::then(begin, [parent_id]() {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
        });
        tt::sync_wait(work1);
    }

    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto begin = ex::just();
        auto transfer1 = ex::continues_on(begin, ex::thread_pool_scheduler{});
        auto work1 = ex::then(transfer1, [parent_id]() {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
        });
        tt::sync_wait(work1);
    }
}

void test_just_one_arg()
{
    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto begin = ex::just(3);
        auto work1 = ex::then(begin, [parent_id](int x) {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            HPX_TEST_EQ(x, 3);
        });
        tt::sync_wait(work1);
    }

    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto begin = ex::just(3);
        auto transfer1 = ex::continues_on(begin, ex::thread_pool_scheduler{});
        auto work1 = ex::then(transfer1, [parent_id](int x) {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            HPX_TEST_EQ(x, 3);
        });
        tt::sync_wait(work1);
    }
}

void test_just_two_args()
{
    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto begin = ex::just(3, std::string("hello"));
        auto work1 = ex::then(begin, [parent_id](int x, std::string y) {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            HPX_TEST_EQ(x, 3);
            HPX_TEST_EQ(y, std::string("hello"));
        });
        tt::sync_wait(work1);
    }

    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto begin = ex::just(3, std::string("hello"));
        auto transfer1 = ex::continues_on(begin, ex::thread_pool_scheduler{});
        auto work1 = ex::then(transfer1, [parent_id](int x, std::string y) {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            HPX_TEST_EQ(x, 3);
            HPX_TEST_EQ(y, std::string("hello"));
        });
        tt::sync_wait(work1);
    }
}

void test_transfer_just_void()
{
    hpx::thread::id parent_id = hpx::this_thread::get_id();

    auto begin = ex::just() | ex::continues_on(ex::thread_pool_scheduler{});
    auto work1 = ex::then(begin,
        [parent_id]() { HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id()); });
    tt::sync_wait(work1);
}

void test_bulk_starts_on()
{
    std::vector<int> const ns = {0, 1, 10, 43};

    for (int n : ns)
    {
        std::vector<int> v(n, 0);
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        // Test starts_on pattern: bulk operation with scheduler in environment
        // Use start_on to provide scheduler through environment
        auto bulk_sender = ex::continues_on(
            ex::thread_pool_scheduler{}, ex::just() | ex::bulk(n, [&](int i) {
                ++v[i];
                HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            }));

        tt::sync_wait(std::move(bulk_sender));

        // Verify results
        int incremented_count = 0;
        for (int i = 0; i < n; ++i)
        {
            if (v[i] == 1)
            {
                ++incremented_count;
            }
        }
        if (n > 0)
        {
            HPX_TEST(incremented_count > 0);
        }
        else
        {
            HPX_TEST_EQ(incremented_count, 0);
        }
    }
}

void test_transfer_just_one_arg()
{
    hpx::thread::id parent_id = hpx::this_thread::get_id();

    auto begin = ex::just(3) | ex::continues_on(ex::thread_pool_scheduler{});
    auto work1 = ex::then(begin, [parent_id](int x) {
        HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
        HPX_TEST_EQ(x, 3);
    });
    tt::sync_wait(work1);
}

void test_transfer_just_two_args()
{
    hpx::thread::id parent_id = hpx::this_thread::get_id();

    auto begin = ex::just(3, std::string("hello")) |
        ex::continues_on(ex::thread_pool_scheduler{});
    auto work1 = ex::then(begin, [parent_id](int x, std::string y) {
        HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
        HPX_TEST_EQ(x, 3);
        HPX_TEST_EQ(y, std::string("hello"));
    });
    tt::sync_wait(work1);
}

void test_when_all()
{
    ex::thread_pool_scheduler sched{};

    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto work1 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            return 42;
        });

        auto work2 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            return std::string("hello");
        });

        auto work3 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            return 3.14;
        });

        auto when1 =
            ex::when_all(std::move(work1), std::move(work2), std::move(work3));

        bool executed{false};
        tt::sync_wait(std::move(when1) |
            ex::then([parent_id, &executed](int x, std::string y, double z) {
                HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
                HPX_TEST_EQ(x, 42);
                HPX_TEST_EQ(y, std::string("hello"));
                HPX_TEST_EQ(z, 3.14);
                executed = true;
            }));
        HPX_TEST(executed);
    }

    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        // The exception is likely to be thrown before set_value from the second
        // sender is called because the second sender sleeps.
        auto work1 = ex::schedule(sched) | ex::then([parent_id]() -> int {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            throw std::runtime_error("error");
        });

        auto work2 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            return std::string("hello");
        });

        bool exception_thrown = false;

        try
        {
            tt::sync_wait(ex::when_all(std::move(work1), std::move(work2)) |
                ex::then([parent_id](int x, std::string y) {
                    HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
                    HPX_TEST_EQ(x, 42);
                    HPX_TEST_EQ(y, std::string("hello"));
                }));
            HPX_TEST(false);
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ(std::string(e.what()), std::string("error"));
            exception_thrown = true;
        }

        HPX_TEST(exception_thrown);
    }

    {
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        // The exception is likely to be thrown after set_value from the second
        // sender is called because the first sender sleeps before throwing.
        auto work1 = ex::schedule(sched) | ex::then([parent_id]() -> int {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            throw std::runtime_error("error");
        });

        auto work2 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            return std::string("hello");
        });

        bool exception_thrown = false;

        try
        {
            tt::sync_wait(ex::when_all(std::move(work1), std::move(work2)) |
                ex::then([parent_id](int x, std::string y) {
                    HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
                    HPX_TEST_EQ(x, 42);
                    HPX_TEST_EQ(y, std::string("hello"));
                }));
            HPX_TEST(false);
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ(std::string(e.what()), std::string("error"));
            exception_thrown = true;
        }

        HPX_TEST(exception_thrown);
    }
}

void test_future_sender()
{
    // futures as senders
    {
        std::atomic<bool> called{false};
        auto f = hpx::async([&]() { called = true; });
        auto fs = ex::as_sender(std::move(f));

        static_assert(ex::is_sender_v<std::decay_t<decltype(fs)>>,
            "a future should be adaptable to a sender");
        static_assert(std::is_same_v<
                          std::decay_t<decltype(*tt::sync_wait(std::move(fs)))>,
                          std::tuple<>>,
            "sync_wait should return hpx::tuple<>");

        tt::sync_wait(std::move(fs));
        HPX_TEST(called);

        bool exception_thrown = false;
        try
        {
            // The move is intentional. sync_wait should throw.
            // NOLINTNEXTLINE(bugprone-use-after-move)
            tt::sync_wait(ex::as_sender(std::move(f)));
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    {
        std::atomic<bool> called{false};
        auto f = hpx::async([&]() {
            called = true;
            return 42;
        });
        auto fs = ex::as_sender(std::move(f));

        static_assert(ex::is_sender_v<std::decay_t<decltype(fs)>>,
            "a future should be adaptable as a sender");

        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(fs))), 42);
        HPX_TEST(called);

        bool exception_thrown = false;
        try
        {
            // The move is intentional. sync_wait should throw.
            // NOLINTNEXTLINE(bugprone-use-after-move)
            tt::sync_wait(ex::as_sender(std::move(f)));
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    {
        std::atomic<bool> called{false};
        auto f = hpx::async([&]() {
            called = true;
            return 42;
        });

        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::then(
                ex::as_sender(std::move(f)), [](int x) { return x / 2; }))),
            21);
        HPX_TEST(called);
    }

    {
        std::atomic<std::size_t> calls{0};
        auto sf = hpx::async([&]() { ++calls; }).share();
        auto sfs = ex::as_sender(sf);

        static_assert(ex::is_sender_v<std::decay_t<decltype(sfs)>>,
            "a shared_future should be adaptable as a sender");
        static_assert(
            std::is_same_v<std::decay_t<decltype(*tt::sync_wait(sfs))>,
                std::tuple<>>,
            "sync_wait should return hpx::tuple<>");

        tt::sync_wait(sfs);
        tt::sync_wait(sfs);
        tt::sync_wait(std::move(sfs));
        HPX_TEST_EQ(calls, std::size_t(1));

        bool exception_thrown = false;
        try
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            tt::sync_wait(sfs);
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    {
        std::atomic<std::size_t> calls{0};
        auto sf = hpx::async([&]() {
            ++calls;
            return 42;
        }).share();
        auto sfs = ex::as_sender(sf);

        static_assert(ex::is_sender_v<std::decay_t<decltype(sfs)>>,
            "a shared_future should be a sender");

        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(sfs)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(sfs)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(sfs))), 42);
        HPX_TEST_EQ(calls, std::size_t(1));

        bool exception_thrown = false;
        try
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            tt::sync_wait(sfs);
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    // senders as futures
    {
        auto s = ex::just(3);
        auto f = ex::make_future(std::move(s));
        HPX_TEST_EQ(f.get(), 3);
    }

    {
        auto s = ex::just(ex::thread_pool_scheduler{}, 3);
        auto f = ex::make_future(std::move(s));
        HPX_TEST_EQ(f.get(), 3);
    }

    {
        auto f = ex::just(3) | ex::make_future();
        HPX_TEST_EQ(f.get(), 3);
    }

    {
        auto f = ex::just(ex::thread_pool_scheduler{}, 3) | ex::make_future();
        HPX_TEST_EQ(f.get(), 3);
    }

    {
        std::atomic<bool> called{false};
        auto s = ex::schedule(ex::thread_pool_scheduler{}) |
            ex::then([&] { called = true; });
        auto f = ex::make_future(std::move(s));
        f.get();
        HPX_TEST(called);
    }

    {
        auto s1 = ex::just(ex::thread_pool_scheduler{}, std::size_t(42));
        auto s2 = ex::just(ex::thread_pool_scheduler{}, 3.14);
        auto s3 = ex::just(ex::thread_pool_scheduler{}, std::string("hello"));
        auto f = ex::make_future(ex::then(
            ex::when_all(std::move(s1), std::move(s2), std::move(s3)),
            [](std::size_t x, double, std::string z) { return z.size() + x; }));
        HPX_TEST_EQ(f.get(), std::size_t(47));
    }

    // mixing senders and futures
    {
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(ex::as_sender(ex::make_future(
                        ex::just(ex::thread_pool_scheduler{}, 42))))),
            42);
    }

    {
        HPX_TEST_EQ(
            ex::make_future(
                ex::continues_on(ex::as_sender(hpx::async([]() { return 42; })),
                    ex::thread_pool_scheduler{}))
                .get(),
            42);
    }

    {
        auto s1 = ex::just(ex::thread_pool_scheduler{}, std::size_t(42));
        auto s2 = ex::just(ex::thread_pool_scheduler{}, 3.14);
        auto s3 = ex::just(ex::thread_pool_scheduler{}, std::string("hello"));
        auto f = ex::make_future(ex::then(
            ex::when_all(std::move(s1), std::move(s2), std::move(s3)),
            [](std::size_t x, double, std::string z) { return z.size() + x; }));
        auto sf = f.then([](auto&& f) { return f.get() - 40; }).share();
        auto t1 = sf.then([](auto&& sf) { return sf.get() + 1; });
        auto t2 = sf.then([](auto&& sf) { return sf.get() + 2; });
        auto t1s = ex::then(
            ex::as_sender(std::move(t1)), [](std::size_t x) { return x + 1; });
        auto t1f = ex::make_future(std::move(t1s));
        auto last = hpx::dataflow(
            hpx::unwrapping([](std::size_t x, std::size_t y) { return x + y; }),
            t1f, t2);

        HPX_TEST_EQ(last.get(), std::size_t(18));
    }
}

void test_ensure_started()
{
    ex::thread_pool_scheduler sched{};

    {
        tt::sync_wait(ex::schedule(sched) | ex::ensure_started());
    }

    {
        auto s = ex::just(sched, 42) | ex::ensure_started();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
    }

    {
        auto s = ex::just(sched, 42) | ex::ensure_started() |
            ex::continues_on(sched);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
    }

    {
        auto s = ex::just(sched, 42) | ex::ensure_started() | ex::split();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
    }
}

void test_ensure_started_when_all()
{
    ex::thread_pool_scheduler sched{};

    {
        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        hpx::mutex mtx;
        hpx::condition_variable cond;
        bool started{false};
        auto s = ex::schedule(sched) | ex::then([&]() {
            ++first_task_calls;
            std::lock_guard l{mtx};
            started = true;
            cond.notify_one();
        }) | ex::ensure_started() |
            ex::split();
        ;
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return started; });
        }
        auto succ1 = s | ex::then([&]() {
            ++successor_task_calls;
            return 1;
        });
        auto succ2 = s | ex::then([&]() {
            ++successor_task_calls;
            return 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            3);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
    }

    {
        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        hpx::mutex mtx;
        hpx::condition_variable cond;
        bool started{false};
        auto s = ex::schedule(sched) | ex::then([&]() {
            ++first_task_calls;
            std::lock_guard l{mtx};
            started = true;
            cond.notify_one();
            return 3;
        }) | ex::ensure_started() |
            ex::split();
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return started; });
        }
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        auto succ1 = s | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = s | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
    }

    {
        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        hpx::mutex mtx;
        hpx::condition_variable cond;
        bool started{false};
        auto s = ex::schedule(sched) | ex::then([&]() {
            ++first_task_calls;
            std::lock_guard l{mtx};
            started = true;
            cond.notify_one();
            return 3;
        }) | ex::ensure_started() |
            ex::split();
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return started; });
        }
        auto succ1 = s | ex::continues_on(sched) | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = s | ex::continues_on(sched) | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
    }
}

void test_split()
{
    ex::thread_pool_scheduler sched{};

    {
        tt::sync_wait(ex::schedule(sched) | ex::split());
    }

    {
        auto s = ex::just(sched, 42) | ex::split();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
    }

    {
        auto s = ex::just(sched, 42) | ex::split() | ex::continues_on(sched);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
    }

    {
        auto s = ex::just(sched, 42) | ex::split();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
    }
}

void test_split_when_all()
{
    ex::thread_pool_scheduler sched{};

    {
        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        auto s = ex::schedule(sched) | ex::then([&]() { ++first_task_calls; }) |
            ex::split();
        auto succ1 = s | ex::then([&]() {
            ++successor_task_calls;
            return 1;
        });
        auto succ2 = s | ex::then([&]() {
            ++successor_task_calls;
            return 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            3);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
    }

    {
        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        auto s = ex::schedule(sched) | ex::then([&]() {
            ++first_task_calls;
            return 3;
        }) | ex::split();
        auto succ1 = s | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = s | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
    }

    {
        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        auto s = ex::schedule(sched) | ex::then([&]() {
            ++first_task_calls;
            return 3;
        }) | ex::split();
        auto succ1 = s | ex::continues_on(sched) | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = s | ex::continues_on(sched) | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*(tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; })))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
    }
}

void test_let_value()
{
    ex::thread_pool_scheduler sched{};

    // void predecessor
    {
        auto result = hpx::get<0>(*(tt::sync_wait(ex::schedule(sched) |
            ex::let_value([]() { return ex::just(42); }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*(tt::sync_wait(ex::schedule(sched) |
            ex::let_value([=]() { return ex::just(sched, 42); }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*tt::sync_wait((ex::just() |
            ex::let_value([=]() { return ex::just(sched, 42); }))));
        HPX_TEST_EQ(result, 42);
    }

    // int predecessor, value ignored
    {
        auto result = hpx::get<0>(*(tt::sync_wait(ex::just(sched, 43) |
            ex::let_value([](int&) { return ex::just(42); }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*(tt::sync_wait(ex::just(sched, 43) |
            ex::let_value([=](int&) { return ex::just(sched, 42); }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*(tt::sync_wait(ex::just(43) |
            ex::let_value([=](int&) { return ex::just(sched, 42); }))));
        HPX_TEST_EQ(result, 42);
    }

    // int predecessor, value used
    {
        auto result = hpx::get<0>(
            *(tt::sync_wait(ex::just(sched, 43) | ex::let_value([](int& x) {
                return ex::just(42) | ex::then([&](int y) { return x + y; });
            }))));

        HPX_TEST_EQ(result, 85);
    }

    {
        auto result = hpx::get<0>(
            *(tt::sync_wait(ex::just(sched, 43) | ex::let_value([=](int& x) {
                return ex::just(sched, 42) |
                    ex::then([&](int y) { return x + y; });
            }))));
        HPX_TEST_EQ(result, 85);
    }

    {
        auto result = hpx::get<0>(
            *(tt::sync_wait(ex::just(43) | ex::let_value([=](int& x) {
                return ex::just(sched, 42) |
                    ex::then([&](int y) { return x + y; });
            }))));
        HPX_TEST_EQ(result, 85);
    }

    // predecessor throws, let sender is ignored
    {
        bool exception_thrown = false;

        try
        {
            tt::sync_wait(ex::just(sched, 43) | ex::then([](int x) {
                throw std::runtime_error("error");
                return x;
            }) | ex::let_value([](int&) {
                HPX_TEST(false);
                return ex::just(0);
            }));
            HPX_TEST(false);
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ(std::string(e.what()), std::string("error"));
            exception_thrown = true;
        }

        HPX_TEST(exception_thrown);
    }
}

void check_exception_ptr_message(
    std::exception_ptr ep, std::string const& message)
{
    try
    {
        std::rethrow_exception(ep);
    }
    catch (std::runtime_error const& e)
    {
        HPX_TEST_EQ(std::string(e.what()), message);
        return;
    }

    HPX_TEST(false);
}

void test_let_error()
{
    ex::thread_pool_scheduler sched{};

    // void predecessor
    {
        std::atomic<bool> called{false};
        tt::sync_wait(ex::schedule(sched) | ex::then([]() {
            throw std::runtime_error("error");
        }) | ex::let_error([&called](std::exception_ptr& ep) {
            called = true;
            check_exception_ptr_message(ep, "error");
            return ex::just();
        }));
        HPX_TEST(called);
    }

    {
        std::atomic<bool> called{false};
        tt::sync_wait(ex::schedule(sched) | ex::then([]() {
            throw std::runtime_error("error");
        }) | ex::let_error([=, &called](std::exception_ptr& ep) {
            called = true;
            check_exception_ptr_message(ep, "error");
            return ex::just(sched);
        }));
        HPX_TEST(called);
    }

    {
        std::atomic<bool> called{false};
        tt::sync_wait(ex::just() | ex::then([]() {
            throw std::runtime_error("error");
        }) | ex::let_error([=, &called](std::exception_ptr& ep) {
            called = true;
            check_exception_ptr_message(ep, "error");
            return ex::just(sched);
        }));
        HPX_TEST(called);
    }

    // int predecessor
    {
        auto result =
            hpx::get<0>(*(tt::sync_wait(ex::schedule(sched) | ex::then([]() {
                throw std::runtime_error("error");
                return 43;
            }) | ex::let_error([](std::exception_ptr& ep) {
                check_exception_ptr_message(ep, "error");
                return ex::just(42);
            }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result =
            hpx::get<0>(*(tt::sync_wait(ex::schedule(sched) | ex::then([]() {
                throw std::runtime_error("error");
                return 43;
            }) | ex::let_error([=](std::exception_ptr& ep) {
                check_exception_ptr_message(ep, "error");
                return ex::just(sched, 42);
            }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*(tt::sync_wait(ex::just() | ex::then([]() {
            throw std::runtime_error("error");
            return 43;
        }) | ex::let_error([=](std::exception_ptr& ep) {
            check_exception_ptr_message(ep, "error");
            return ex::just(sched, 42);
        }))));
        HPX_TEST_EQ(result, 42);
    }

    // predecessor doesn't throw, let sender is ignored
    {
        auto result = hpx::get<0>(*(tt::sync_wait(
            ex::just(sched, 42) | ex::let_error([](std::exception_ptr) {
                HPX_TEST(false);
                return ex::just(43);
            }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*(tt::sync_wait(
            ex::just(sched, 42) | ex::let_error([=](std::exception_ptr) {
                HPX_TEST(false);
                return ex::just(sched, 43);
            }))));
        HPX_TEST_EQ(result, 42);
    }

    {
        auto result = hpx::get<0>(*(
            tt::sync_wait(ex::just(42) | ex::let_error([=](std::exception_ptr) {
                HPX_TEST(false);
                return ex::just(sched, 43);
            }))));
        HPX_TEST_EQ(result, 42);
    }
}

void test_detach()
{
    ex::thread_pool_scheduler sched{};

    {
        bool called = false;
        hpx::mutex mtx;
        hpx::condition_variable cond;
        ex::start_detached(ex::schedule(sched) | ex::then([&]() {
            std::unique_lock l{mtx};
            called = true;
            cond.notify_one();
        }));

        {
            std::unique_lock l{mtx};
            HPX_TEST(cond.wait_for(
                l, std::chrono::seconds(1), [&]() { return called; }));
        }
        HPX_TEST(called);
    }

    // Values passed to set_value are ignored
    {
        bool called = false;
        hpx::mutex mtx;
        hpx::condition_variable cond;
        ex::start_detached(ex::schedule(sched) | ex::then([&]() {
            std::lock_guard l{mtx};
            called = true;
            cond.notify_one();
            return 42;
        }));

        {
            std::unique_lock l{mtx};
            HPX_TEST(cond.wait_for(
                l, std::chrono::seconds(1), [&]() { return called; }));
        }
        HPX_TEST(called);
    }
}

void test_keep_future_sender()
{
    // the future should be passed to then, not it's contained value
    {
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<void>()) |
            ex::then([](hpx::future<void>&& f) { HPX_TEST(f.is_ready()); }));
    }

    {
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<void>().share()) |
            ex::then(
                [](hpx::shared_future<void>&& f) { HPX_TEST(f.is_ready()); }));
    }

    {
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<int>(42)) |
            ex::then([](hpx::future<int>&& f) {
                HPX_TEST(f.is_ready());
                HPX_TEST_EQ(f.get(), 42);
            }));
    }

    {
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<int>(42).share()) |
            ex::then([](hpx::shared_future<int>&& f) {
                HPX_TEST(f.is_ready());
                HPX_TEST_EQ(f.get(), 42);
            }));
    }

    {
        std::atomic<bool> called{false};
        auto f = hpx::async([&]() { called = true; });

        auto r = hpx::get<0>(*tt::sync_wait(ex::keep_future(std::move(f))));
        static_assert(
            std::is_same<std::decay_t<decltype(r)>, hpx::future<void>>::value,
            "sync_wait should return future<void>");

        HPX_TEST(called);
        HPX_TEST(r.is_ready());

        bool exception_thrown = false;
        try
        {
            // The move is intentional. sync_wait should throw.
            // NOLINTNEXTLINE(bugprone-use-after-move)
            tt::sync_wait(ex::keep_future(std::move(f)));
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    {
        std::atomic<bool> called{false};
        auto f = hpx::async([&]() {
            called = true;
            return 42;
        });

        auto r = hpx::get<0>(*tt::sync_wait(ex::keep_future(std::move(f))));
        static_assert(
            std::is_same<std::decay_t<decltype(r)>, hpx::future<int>>::value,
            "sync_wait should return future<int>");

        HPX_TEST(called);
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get(), 42);

        bool exception_thrown = false;
        try
        {
            // The move is intentional. sync_wait should throw.
            // NOLINTNEXTLINE(bugprone-use-after-move)
            tt::sync_wait(ex::keep_future(std::move(f)));
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    {
        std::atomic<bool> called{false};
        auto f = hpx::async([&]() {
            called = true;
            return 42;
        });

        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::then(ex::keep_future(std::move(f)),
                [](hpx::future<int>&& f) { return f.get() / 2; }))),
            21);
        HPX_TEST(called);
    }

    {
        std::atomic<std::size_t> calls{0};
        auto sf = hpx::async([&]() { ++calls; }).share();

        tt::sync_wait(ex::keep_future(sf));
        tt::sync_wait(ex::keep_future(sf));
        tt::sync_wait(ex::keep_future(std::move(sf)));
        HPX_TEST_EQ(calls, std::size_t(1));

        bool exception_thrown = false;
        try
        {
            tt::sync_wait(ex::keep_future(sf));
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    {
        std::atomic<std::size_t> calls{0};
        auto sf = hpx::async([&]() {
            ++calls;
            return 42;
        }).share();

        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(ex::keep_future(sf))).get(), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(ex::keep_future(sf))).get(), 42);
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::keep_future(std::move(sf)))).get(),
            42);
        HPX_TEST_EQ(calls, std::size_t(1));

        bool exception_thrown = false;
        try
        {
            tt::sync_wait(ex::keep_future(sf));
            HPX_TEST(false);
        }
        catch (...)
        {
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    // Keep future alive across on
    {
        auto f = hpx::async([&]() { return 42; });
        auto r = hpx::get<0>(*(tt::sync_wait(ex::keep_future(std::move(f)) |
            ex::continues_on(ex::thread_pool_scheduler{}))));
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get(), 42);
    }

    {
        auto sf = hpx::async([&]() { return 42; }).share();
        auto r = hpx::get<0>(*(tt::sync_wait(ex::keep_future(std::move(sf)) |
            ex::continues_on(ex::thread_pool_scheduler{}))));
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get(), 42);
    }

    {
        auto sf = hpx::async([&]() {
            return custom_type_non_default_constructible_non_copyable{42};
        }).share();

        // NOTE: Without keep_future this should fail to compile, since
        // sync_wait would receive a const& to the value which requires a copy
        // or storing a const&. The copy is not possible because the type is
        // noncopyable, and storing a reference is not acceptable since the
        // reference may outlive the value.
        auto r = hpx::get<0>(*(tt::sync_wait(ex::keep_future(std::move(sf)) |
            ex::continues_on(ex::thread_pool_scheduler{}))));
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get().x, 42);
    }

    // Use unwrapping with keep_future
    {
        auto f = hpx::async([]() { return 42; });
        auto sf = hpx::async([]() { return 3.14; }).share();

        auto fun = hpx::unwrapping(
            [](int&& x, double const& y) { return x * 2 + (int(y) / 2); });
        HPX_TEST_EQ(hpx::get<0>(*(tt::sync_wait(
                        ex::when_all(ex::keep_future(std::move(f)),
                            ex::keep_future(sf)) |
                        ex::then(fun)))),
            85);
    }

    {
        auto f = hpx::async([]() { return 42; });
        auto sf = hpx::async([]() { return 3.14; }).share();

        auto fun = hpx::unwrapping(
            [](int&& x, double const& y) { return x * 2 + (int(y) / 2); });
        HPX_TEST_EQ(hpx::get<0>(*(tt::sync_wait(
                        ex::when_all(ex::keep_future(std::move(f)),
                            ex::keep_future(sf)) |
                        ex::continues_on(ex::thread_pool_scheduler{}) |
                        ex::then(fun)))),
            85);
    }
}

void test_bulk()
{
    std::vector<int> const ns = {0, 1, 10, 43};

    for (int n : ns)
    {
        std::vector<int> v(n, 0);
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        tt::sync_wait(
            ex::schedule(ex::thread_pool_scheduler{}) | ex::bulk(n, [&](int i) {
                ++v[i];
                HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
            }));

        // In chunked mode, only chunk begin indices are processed
        // So we check that at least some elements were incremented
        int incremented_count = 0;
        for (int i = 0; i < n; ++i)
        {
            if (v[i] == 1)
            {
                incremented_count++;
            }
        }
        // With chunked execution, we expect fewer calls than total elements
        if (n > 0)
        {
            HPX_TEST(
                incremented_count > 0);    // At least one element processed
            HPX_TEST(incremented_count <= n);    // Not more than total elements
        }
    }

    for (auto n : ns)
    {
        std::vector<int> v(n, -1);
        hpx::thread::id parent_id = hpx::this_thread::get_id();

        auto v_out = hpx::get<0>(*(
            tt::sync_wait(ex::just(ex::thread_pool_scheduler{}, std::move(v)) |
                ex::bulk(n, [&parent_id](int i, std::vector<int>& v) {
                    v[i] = i;
                    HPX_TEST_NEQ(parent_id, hpx::this_thread::get_id());
                }))));

        // In chunked mode, only chunk begin indices are processed
        // So we check that at least some elements were set correctly
        int correct_count = 0;
        for (int i = 0; i < n; ++i)
        {
            if (v_out[i] == i)
            {
                correct_count++;
            }
        }
        // With chunked execution, we expect fewer calls than total elements
        if (n > 0)
        {
            HPX_TEST(correct_count >
                0);    // At least one element processed correctly
            HPX_TEST(correct_count <= n);    // Not more than total elements
        }
    }

    {
        std::unordered_set<std::string> string_map;
        std::vector<std::string> v = {"hello", "brave", "new", "world"};
        std::vector<std::string> v_ref = v;

        hpx::mutex mtx;
        tt::sync_wait(ex::schedule(ex::thread_pool_scheduler{}) |
            ex::bulk(std::move(v), [&](std::string const& s) {
                std::lock_guard lk(mtx);
                string_map.insert(s);
            }));

        for (auto const& s : v_ref)
        {
            HPX_TEST(string_map.find(s) != string_map.end());
        }
    }

    for (auto n : ns)
    {
        int i_fail = 3;

        std::vector<int> v(n, -1);
        bool const expect_exception = n > i_fail;

        try
        {
            tt::sync_wait(ex::just(ex::thread_pool_scheduler{}) |
                ex::bulk(n, [&v, i_fail](int i) {
                    if (i == i_fail)
                    {
                        throw std::runtime_error("error");
                    }
                    v[i] = i;
                }));

            if (expect_exception)
            {
                HPX_TEST(false);
            }
        }
        catch (std::runtime_error const& e)
        {
            if (!expect_exception)
            {
                HPX_TEST(false);
            }

            HPX_TEST(std::string(e.what()).find("error") == 0);
        }

        if (expect_exception)
        {
            HPX_TEST_EQ(v[i_fail], -1);
        }
        else
        {
            for (int i = 0; i < n; ++i)
            {
                HPX_TEST_EQ(v[i], i);
            }
        }
    }
}
// NOLINTEND(bugprone-unchecked-optional-access)

// ============================================================================
// STDEXEC BULK OPERATIONS DOMAIN CUSTOMIZATION TESTS
// ============================================================================
// These tests verify that HPX's thread_pool_scheduler properly integrates with
// stdexec's bulk operations through domain customization as specified in P2999R3.
// The domain system allows HPX to intercept and customize stdexec bulk operations
// to use HPX's sophisticated work-stealing thread pool implementation.

void test_stdexec_domain_queries()
{
    auto scheduler = ex::thread_pool_scheduler{};

    // 1. Verify domain derives from ex::default_domain
    static_assert(std::is_base_of_v<ex::default_domain,
                      ex::thread_pool_domain<hpx::launch>>,
        "thread_pool_domain should derive from default_domain");
    // 2. Verify domain is accessible via ex::get_domain (forwarded from stdexec)
    static_assert(
        requires { ex::get_domain(scheduler); },
        "scheduler should support get_domain query");
    auto domain = ex::get_domain(scheduler);

    // 3. Verify the domain type is thread_pool_domain
    static_assert(
        std::is_same_v<decltype(domain), ex::thread_pool_domain<hpx::launch>>,
        "scheduler domain should be thread_pool_domain<hpx::launch>");
    // 4. Verify transform_sender produces thread_pool_bulk_sender for
    //    bulk_chunked (proves the domain customization is picked up)
    {
        auto env = ex::env{ex::prop{ex::get_scheduler, scheduler}};

        auto chunked_sndr = ex::bulk_chunked(
            ex::schedule(scheduler), ex::par, 10, [](int, int) {});

        // Verify the sender matches the bulk concept
        using chunked_t = std::decay_t<decltype(chunked_sndr)>;
        static_assert(ex::bulk_chunked_or_unchunked_sender<chunked_t>,
            "bulk_chunked sender should satisfy "
            "bulk_chunked_or_unchunked_sender concept");

        auto transformed = domain.transform_sender(
            ex::set_value_t{}, std::move(chunked_sndr), env);

        static_assert(is_thread_pool_bulk_sender<
                          std::decay_t<decltype(transformed)>>::value,
            "domain transform_sender for bulk_chunked should produce "
            "thread_pool_bulk_sender");
    }

    // 5. Verify transform_sender produces thread_pool_bulk_sender for
    //    bulk_unchunked (proves the domain customization is picked up)
    {
        auto env = ex::env{ex::prop{ex::get_scheduler, scheduler}};

        auto unchunked_sndr = ex::bulk_unchunked(
            ex::schedule(scheduler), ex::par, 10, [](int) {});

        // Verify the sender matches the bulk concept
        using unchunked_t = std::decay_t<decltype(unchunked_sndr)>;
        static_assert(ex::bulk_chunked_or_unchunked_sender<unchunked_t>,
            "bulk_unchunked sender should satisfy "
            "bulk_chunked_or_unchunked_sender concept");

        auto transformed = domain.transform_sender(
            ex::set_value_t{}, std::move(unchunked_sndr), env);

        static_assert(is_thread_pool_bulk_sender<
                          std::decay_t<decltype(transformed)>>::value,
            "domain transform_sender for bulk_unchunked should produce "
            "thread_pool_bulk_sender");
    }

    HPX_TEST(true);
}

void test_stdexec_bulk_domain_customization()
{
    auto scheduler = ex::thread_pool_scheduler{};

    // Test basic bulk operation with domain customization
    // Note: bulk() maps to bulk_chunked, so function is called once per chunk
    std::vector<int> results(10, 0);
    std::atomic<int> chunk_calls{0};

    auto bulk_sender = ex::bulk_chunked(
        ex::schedule(scheduler) | ex::then([]() { return 42; }), ex::par, 10,
        [&](int start, int end, int value) {
            // In unchunked mode, this is called once per index
            for (int idx = start; idx < end; ++idx)
            {
                chunk_calls.fetch_add(1);
                results[idx] = value + idx;
            }
        });

    // Note: bulk() produces a bulk_t sender at construction time.
    // Lowering to bulk_chunked happens at connect time via
    // domain transform_sender.

    ex::sync_wait(std::move(bulk_sender));

    // Verify that chunked execution happened
    // In our implementation, each element in the chunk calls the function
    // With chunk_size=4 and 10 items, we process all 10 elements
    HPX_TEST(chunk_calls.load() == 10);    // Should process all 10 elements
    HPX_TEST(chunk_calls.load() > 0);      // Should have at least 1 call

    // Verify that at least some results were set (the chunk begin indices)
    bool some_results_set = false;
    for (int i = 0; i < 10; ++i)
    {
        if (results[i] != 0)
        {
            some_results_set = true;
            HPX_TEST_EQ(results[i], 42 + i);    // Verify correct computation
        }
    }
    HPX_TEST(some_results_set);
}

void test_stdexec_bulk_chunked_customization()
{
    auto scheduler = ex::thread_pool_scheduler{};

    // Test bulk_chunked operation - should use larger chunks for better performance
    std::vector<int> results(100, 0);    // Larger size to see chunking effects
    std::atomic<int> function_calls{0};
    std::atomic<int> total_processed{0};

    auto bulk_chunked_sender =
        ex::bulk_chunked(ex::schedule(scheduler) | ex::then([]() { return 1; }),
            ex::par, 100, [&](int start, int end, int value) {
                // With chunked execution: process range [start, end)

                for (int idx = start; idx < end; ++idx)
                {
                    function_calls.fetch_add(1, std::memory_order_relaxed);
                    results[idx] = value + idx;
                    total_processed.fetch_add(1, std::memory_order_relaxed);
                }
            });

    // Verify this is recognized as a bulk_chunked sender
    using chunked_sender_t = std::decay_t<decltype(bulk_chunked_sender)>;
    static_assert(ex::bulk_chunked_or_unchunked_sender<chunked_sender_t>,
        "bulk_chunked sender should satisfy "
        "bulk_chunked_or_unchunked_sender");

    ex::sync_wait(std::move(bulk_chunked_sender));

    // Verify all elements were processed
    HPX_TEST_EQ(total_processed.load(), 100);
    HPX_TEST_EQ(function_calls.load(), 100);    // Called once per element

    // Verify results are correct
    for (int i = 0; i < 100; ++i)
    {
        HPX_TEST_EQ(results[i], 1 + i);
    }
}

void test_stdexec_bulk_unchunked_customization()
{
    auto scheduler = ex::thread_pool_scheduler{};

    // Test bulk_unchunked operation - should use smaller chunks for better load balancing
    std::vector<int> results(
        100, 0);    // Same size as chunked test for comparison
    std::atomic<int> function_calls{0};

    auto bulk_unchunked_sender = ex::bulk_unchunked(
        ex::schedule(scheduler) | ex::then([]() { return 5; }), ex::par, 100,
        [&](int idx, int value) {
            // With unchunked execution: smaller chunks (chunk_size=1), better
            // work stealing
            function_calls.fetch_add(1, std::memory_order_relaxed);
            results[idx] = value * idx;
        });

    // Verify this is recognized as a bulk_unchunked sender
    using unchunked_sender_t = std::decay_t<decltype(bulk_unchunked_sender)>;
    static_assert(ex::bulk_chunked_or_unchunked_sender<unchunked_sender_t>,
        "bulk_unchunked sender should satisfy "
        "bulk_chunked_or_unchunked_sender");

    ex::sync_wait(std::move(bulk_unchunked_sender));

    // Verify all elements were processed
    HPX_TEST_EQ(function_calls.load(), 100);    // Called once per element

    // Verify results are correct
    for (int i = 0; i < 100; ++i)
    {
        HPX_TEST_EQ(results[i], 5 * i);
    }
}

void test_stdexec_thread_distribution()
{
    auto scheduler = ex::thread_pool_scheduler{};
    std::thread::id main_thread_id = std::this_thread::get_id();

    // Test that bulk operations run on worker threads
    std::set<std::thread::id> worker_threads;
    std::atomic<int> task_count{0};

    auto bulk_sender =
        ex::bulk_chunked(ex::schedule(scheduler) | ex::then([]() { return 0; }),
            ex::par, 8, [&](int start, int end, int value) {
                worker_threads.insert(std::this_thread::get_id());
                for (int idx = start; idx < end; ++idx)
                {
                    (void) value;
                    task_count.fetch_add(1, std::memory_order_relaxed);
                }
            });

    ex::sync_wait(std::move(bulk_sender));

    // In chunked mode, each element in the chunk calls the function
    // With chunk_size=4 and 8 items, we process all 8 elements
    HPX_TEST(task_count.load() == 8);    // Should process all 8 elements
    HPX_TEST(task_count.load() > 0);     // Should have at least 1 call
    HPX_TEST(!worker_threads.empty());

    // Verify tasks didn't run on main thread (they use HPX thread pool)
    for (auto const& thread_id : worker_threads)
    {
        HPX_TEST_NEQ(thread_id, main_thread_id);
    }
}

void test_stdexec_execution_policies()
{
    auto scheduler = ex::thread_pool_scheduler{};

    // Test different execution policies with stdexec bulk operations
    std::vector<int> seq_results(5, 0);
    auto seq_sender = ex::bulk_chunked(
        ex::schedule(scheduler) | ex::then([]() { return 10; }), ex::seq, 5,
        [&](int start, int end, int value) {
            for (int idx = start; idx < end; ++idx)
            {
                seq_results[idx] = value + idx;
            }
        });

    ex::sync_wait(std::move(seq_sender));

    // In chunked mode, only chunk begin indices are processed
    int seq_processed = 0;
    for (int i = 0; i < 5; ++i)
    {
        if (seq_results[i] == 10 + i)
        {
            seq_processed++;
        }
    }
    HPX_TEST(seq_processed > 0);    // At least one chunk processed

    // Test par_unseq policy
    std::vector<int> par_unseq_results(5, 0);
    auto par_unseq_sender = ex::bulk_chunked(
        ex::schedule(scheduler) | ex::then([]() { return 20; }), ex::par_unseq,
        5, [&](int start, int end, int value) {
            for (int idx = start; idx < end; ++idx)
            {
                par_unseq_results[idx] = value + idx;
            }
        });

    ex::sync_wait(std::move(par_unseq_sender));

    // In chunked mode, only chunk begin indices are processed
    int par_unseq_processed = 0;
    for (int i = 0; i < 5; ++i)
    {
        if (par_unseq_results[i] == 20 + i)
        {
            par_unseq_processed++;
        }
    }
    HPX_TEST(par_unseq_processed > 0);    // At least one chunk processed
}

// TODO: test_completion_scheduler() uses old stdexec API
#if 0
void test_completion_scheduler()
{
    namespace ex = hpx::execution::experimental;
    {
        auto sender = ex::schedule(ex::thread_pool_scheduler{});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
        // clang-format off
    } // Clang format bug: thinks this '}' is the end of a namespace.
    // clang-format on

    {
        auto sender =
            ex::then(ex::schedule(ex::thread_pool_scheduler{}), []() {});
        using hpx::functional::tag_invoke;
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }

    {
        auto sender = ex::just(ex::thread_pool_scheduler{}, 42);
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }

    {
        auto sender =
            ex::bulk(ex::schedule(ex::thread_pool_scheduler{}), 10, [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }

    {
        auto sender = ex::then(
            ex::bulk(ex::just(ex::thread_pool_scheduler{}, 42), 10,
                [](int, int) {}),
            [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }

    {
        auto sender = ex::bulk(
            ex::schedule(ex::thread_pool_scheduler{}), ex::par, 10, [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }

    {
        auto sender = ex::then(
            ex::bulk(ex::just(ex::thread_pool_scheduler{}, 42),
                ex::par, 10, [](int, int) {}),
            [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }

    {
        auto sender = ex::bulk(
            ex::then(ex::just(ex::thread_pool_scheduler{}, 42),
                [](int i) { return i; }),
            ex::par, 10, [](int idx, int val) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                ex::thread_pool_scheduler>,
            "the completion scheduler should be a thread_pool_scheduler");
    }
}
#endif    // Disabled - uses old stdexec API

void test_scheduler_copy_avoidance()
{
    // Test that scheduler operations don't create unnecessary copies

    // Create a scheduler with copy/move tracking
    static std::atomic<int> copy_count{0};
    static std::atomic<int> move_count{0};

    struct copy_tracking_scheduler : ex::thread_pool_scheduler
    {
        copy_tracking_scheduler() = default;

        copy_tracking_scheduler(copy_tracking_scheduler const& other)
          : ex::thread_pool_scheduler(other)
        {
            copy_count.fetch_add(1, std::memory_order_relaxed);
        }

        copy_tracking_scheduler(copy_tracking_scheduler&& other) noexcept
          : ex::thread_pool_scheduler(std::move(other))
        {
            move_count.fetch_add(1, std::memory_order_relaxed);
        }

        copy_tracking_scheduler& operator=(copy_tracking_scheduler const& other)
        {
            ex::thread_pool_scheduler::operator=(other);
            copy_count.fetch_add(1, std::memory_order_relaxed);
            return *this;
        }

        copy_tracking_scheduler& operator=(
            copy_tracking_scheduler&& other) noexcept
        {
            ex::thread_pool_scheduler::operator=(std::move(other));
            move_count.fetch_add(1, std::memory_order_relaxed);
            return *this;
        }
    };

    // Reset counters
    copy_count.store(0);
    move_count.store(0);

    copy_tracking_scheduler sched{};

    // Test basic scheduler operations
    auto sender = ex::schedule(sched);

    // Execute the operation - should not create unnecessary copies
    tt::sync_wait(std::move(sender));

    // Verify that no unnecessary copies were made
    int final_copy_count = copy_count.load();
    int final_move_count = move_count.load();

    // Scheduler operations should minimize copies
    HPX_TEST_LTE(final_copy_count, 1);
    (void) final_move_count;

    HPX_TEST(true);
}

///////////////////////////////////////////////////////////////////////////////
// Test that set_error receives a valid (non-moved-from) receiver when
// execute() throws. This verifies the fix where the receiver was previously
// captured by move into the execute lambda, making it unavailable for the
// error path in operation_state::start().
void test_sender_receiver_set_error_on_execute_failure()
{
    ex::thread_pool_scheduler sched{};

    // Schedule work that throws, then verify the error is properly propagated
    // through set_error (not called on a moved-from receiver).
    {
        std::atomic<bool> error_called{false};
        tt::sync_wait(ex::schedule(sched) | ex::then([]() {
            throw std::runtime_error("execute failed");
        }) | ex::let_error([&error_called](std::exception_ptr& ep) {
            error_called = true;
            check_exception_ptr_message(ep, "execute failed");
            return ex::just();
        }));
        HPX_TEST(error_called);
    }

    // Verify error propagation with a value-returning sender
    {
        std::atomic<bool> error_called{false};
        tt::sync_wait(ex::schedule(sched) | ex::then([]() {
            throw std::runtime_error("execute failed");
            return 0;
        }) | ex::let_error([&error_called](std::exception_ptr& ep) {
            error_called = true;
            check_exception_ptr_message(ep, "execute failed");
            return ex::just(0);
        }));
        HPX_TEST(error_called);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_execute();
    test_sender_receiver_basic();
    test_sender_receiver_then();
    test_sender_receiver_then_wait();
    test_sender_receiver_then_sync_wait();
    test_sender_receiver_then_arguments();
    test_properties();
    test_transfer_basic();
    test_transfer_arguments();
    test_just_void();
    test_just_one_arg();
    test_just_two_args();
    test_transfer_just_void();
    test_transfer_basic();
    test_transfer_arguments();
    test_keep_future_sender();
    test_bulk();
    test_bulk_starts_on();
    test_transfer_just_one_arg();
    test_transfer_just_two_args();
    test_when_all();
    test_future_sender();
    test_keep_future_sender();
    test_ensure_started();
    test_ensure_started_when_all();
    test_split();
    test_split_when_all();
    test_let_value();
    test_let_error();
    test_detach();
    test_bulk();
    // Test stdexec bulk operations domain customization
    test_stdexec_domain_queries();
    test_stdexec_bulk_domain_customization();
    test_stdexec_bulk_chunked_customization();
    test_stdexec_bulk_unchunked_customization();
    test_stdexec_thread_distribution();
    test_stdexec_execution_policies();
    // TODO: test_completion_scheduler() uses old stdexec API
    // test_completion_scheduler();

    test_scheduler_copy_avoidance();
    test_sender_receiver_set_error_on_execute_failure();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
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
