//  Copyright (c) 2024 Isidoros Tsaousis-Seiras
//  Copyright (c) 2022-2025 Hartmut Kaiser
//  Copyright (c) 2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/** Note
 * This file has been heavily modified to facilitate the testing of the R7+
 * iteration of P2300. There are a few differences between that paper and our
 * implementation. The biggest ones that affect this file are:
 *
 * 1. execution::sync_wait() no longer accepts a scheduler as an argument,
 *    in order to begin the execution of the resource to which it belongs.
 *
 * 2. execution::sync_wait() no longer supports a pipe operator| to be chained
 *    onto senders.
 *
 * To work around these changes, the following modifications have been made:
 *
 * 1. In the places that sync_wait was expected to begin the execution of a
 *    resource (e.g. by calling `loop.run()`) a separate hpx thread is created
 *    for the `run_loop` to run on. Also, the checks that the sender and caller
 *    of sync_wait are on the same thread have been adapted to reflect that the
 *    sender is now on the run loop's thread instead.
 *
 * 2. The syntax my_snd | sync_wait() has been replaced with sync_wait(my_snd).
 */

#include <hpx/config.hpp>

// Clang V11 ICE's on this test
#if !defined(HPX_CLANG_VERSION) || (HPX_CLANG_VERSION / 10000) != 11

#include <hpx/condition_variable.hpp>
#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

void test_concepts()
{
    ex::run_loop loop;

    [[maybe_unused]] auto sched = loop.get_scheduler();
    static_assert(ex::is_scheduler_v<decltype(sched)>,
        "ex::is_scheduler_v<decltype(sched)>");

    auto s = ex::schedule(sched);
    static_assert(
        std::is_same_v<ex::schedule_result_t<decltype(sched)>, decltype(s)>,
        "ex::schedule_result_t<decltype(sched)> must be result of "
        "ex::schedule(sched)");
    static_assert(ex::is_sender_v<decltype(s)>, "ex::is_sender_v<decltype(s)>");

    loop.finish();
    loop.run();
}

void test_execute()
{
    hpx::thread::id parent_id = hpx::this_thread::get_id();

    ex::run_loop loop;
    ex::execute(loop.get_scheduler(),
        [parent_id]() { HPX_TEST_EQ(hpx::this_thread::get_id(), parent_id); });

    loop.finish();
    loop.run();
}

struct check_context_receiver
{
    using receiver_concept = ex::receiver_t;

    hpx::thread::id parent_id;
    ex::run_loop& loop;
    bool& executed;

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
        HPX_TEST_EQ(r.parent_id, hpx::this_thread::get_id());
        HPX_TEST_NEQ(hpx::thread::id(hpx::threads::invalid_thread_id),
            hpx::this_thread::get_id());

        r.executed = true;
        r.loop.finish();
    }
};

void test_sender_receiver_basic()
{
    ex::run_loop loop;
    [[maybe_unused]] auto sched = loop.get_scheduler();

    hpx::thread::id parent_id = hpx::this_thread::get_id();
    bool executed{false};

    auto begin = ex::schedule(sched);
    auto os = ex::connect(
        std::move(begin), check_context_receiver{parent_id, loop, executed});
    ex::start(os);

    loop.run();

    HPX_TEST(executed);
}

hpx::thread::id sender_receiver_then_thread_id;

void test_sender_receiver_then()
{
    ex::run_loop loop;
    [[maybe_unused]] auto sched = loop.get_scheduler();

    hpx::thread::id parent_id = hpx::this_thread::get_id();
    bool executed{false};

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(std::move(begin), [=]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(sender_receiver_then_thread_id, parent_id);
    });
    auto work2 = ex::then(std::move(work1), []() {
        HPX_TEST_EQ(sender_receiver_then_thread_id, hpx::this_thread::get_id());
    });
    auto os = ex::connect(
        std::move(work2), check_context_receiver{parent_id, loop, executed});
    ex::start(os);

    loop.run();

    HPX_TEST(executed);
}

void test_sender_receiver_then_wait()
{
    ex::run_loop loop;

    auto t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();

    [[maybe_unused]] auto sched = loop.get_scheduler();

    std::atomic<std::size_t> then_count{0};
    bool executed{false};

    auto begin = ex::schedule(sched);
    // Native P2300R8 does not support sync_wait(scheduler, sender)
    auto compl_sched_begin =
        ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(begin));
    HPX_TEST(sched == compl_sched_begin);

    auto work1 = ex::then(std::move(begin), [&then_count, parent_id]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(sender_receiver_then_thread_id, parent_id);
        ++then_count;
    });

    auto compl_sched_work1 =
        ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(work1));
    HPX_TEST(sched == compl_sched_work1);

    auto work2 = ex::then(std::move(work1), [&then_count, &executed]() {
        HPX_TEST_EQ(sender_receiver_then_thread_id, hpx::this_thread::get_id());
        ++then_count;
        executed = true;
    });
    auto compl_sched_work2 =
        ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(work2));
    HPX_TEST(sched == compl_sched_work2);

    tt::sync_wait(std::move(work2));

    HPX_TEST_EQ(then_count, std::size_t(2));
    HPX_TEST(executed);

    loop.finish();
    t.join();
}

// NOLINTBEGIN(bugprone-unchecked-optional-access)
void test_sender_receiver_then_sync_wait()
{
    ex::run_loop loop;
    auto t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    std::atomic<std::size_t> then_count{0};

    auto begin = ex::schedule(sched);
    auto work = ex::then(std::move(begin), [&then_count, parent_id]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(sender_receiver_then_thread_id, parent_id);
        ++then_count;
        return 42;
    });
    auto work_result = tt::sync_wait(std::move(work));
    auto result = hpx::get<0>(*work_result);
    HPX_TEST_EQ(then_count, std::size_t(1));
    static_assert(
        std::is_same<int, typename std::decay<decltype(result)>::type>::value,
        "result should be an int");
    HPX_TEST_EQ(result, 42);
    loop.finish();
    t.join();
}

void test_sender_receiver_then_arguments()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    std::atomic<std::size_t> then_count{0};

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(std::move(begin), [&then_count, parent_id]() {
        sender_receiver_then_thread_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(sender_receiver_then_thread_id, parent_id);
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
    auto work_result = tt::sync_wait(std::move(work3));
    auto result = hpx::get<0>(*work_result);
    HPX_TEST_EQ(then_count, std::size_t(3));
    static_assert(std::is_same<std::size_t,
                      typename std::decay<decltype(result)>::type>::value,
        "result should be a std::size_t");
    HPX_TEST_EQ(result, std::size_t(12));
    loop.finish();
    t.join();
}

void test_transfer_basic()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    hpx::thread::id current_id;

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(begin, [=, &current_id]() {
        current_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(current_id, parent_id);
    });
    auto work2 = ex::then(work1, [=, &current_id]() {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
    });
    auto transfer1 = ex::transfer(work2, sched);
    auto work3 = ex::then(transfer1, [=, &current_id]() {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_EQ(current_id, parent_id);
    });
    auto work4 = ex::then(work3, [=, &current_id]() {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
    });
    auto transfer2 = ex::transfer(work4, sched);
    auto work5 = ex::then(transfer2, [=, &current_id]() {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_EQ(current_id, parent_id);
    });

    tt::sync_wait(work5);
    loop.finish();
    t.join();
}

void test_transfer_arguments()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    hpx::thread::id current_id;

    auto begin = ex::schedule(sched);
    auto work1 = ex::then(begin, [=, &current_id]() {
        current_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(current_id, parent_id);
        return 3;
    });
    auto work2 = ex::then(work1, [=, &current_id](int x) {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
        return x / 2.0;
    });
    auto transfer1 = ex::transfer(work2, sched);
    auto work3 = ex::then(transfer1, [=, &current_id](double x) {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_EQ(current_id, parent_id);
        return x / 2;
    });
    auto work4 = ex::then(work3, [=, &current_id](int x) {
        HPX_TEST_EQ(current_id, hpx::this_thread::get_id());
        return "result: " + std::to_string(x);
    });
    auto transfer2 = ex::transfer(work4, sched);
    auto work5 = ex::then(transfer2, [=, &current_id](std::string s) {
        hpx::thread::id new_id = hpx::this_thread::get_id();
        HPX_TEST_EQ(current_id, new_id);
        current_id = new_id;
        HPX_TEST_EQ(current_id, parent_id);
        return s + "!";
    });

    auto work_result = tt::sync_wait(work5);
    auto result = hpx::get<0>(*work_result);
    static_assert(std::is_same_v<std::string, std::decay_t<decltype(result)>>,
        "result should be a std::string");
    HPX_TEST_EQ(result, std::string("result: 0!"));
    loop.finish();
    t.join();
}

void test_just_void()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();

    auto begin = ex::just();
    auto transfer1 = ex::transfer(begin, loop.get_scheduler());
    auto work1 = ex::then(transfer1,
        [parent_id]() { HPX_TEST_EQ(parent_id, hpx::this_thread::get_id()); });

    tt::sync_wait(work1);
    loop.finish();
    t.join();
}

void test_just_one_arg()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();

    auto begin = ex::just(3);
    auto transfer1 = ex::transfer(begin, loop.get_scheduler());
    auto work1 = ex::then(transfer1, [parent_id](int x) {
        HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
        HPX_TEST_EQ(x, 3);
    });

    tt::sync_wait(work1);
    loop.finish();
    t.join();
}

void test_just_two_args()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();

    auto begin = ex::just(3, std::string("hello"));
    auto transfer1 = ex::transfer(begin, loop.get_scheduler());
    auto work1 = ex::then(transfer1, [parent_id](int x, std::string y) {
        HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
        HPX_TEST_EQ(x, 3);
        HPX_TEST_EQ(y, std::string("hello"));
    });

    tt::sync_wait(work1);
    loop.finish();
    t.join();
}

void test_transfer_just_void()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    auto begin = ex::transfer_just(sched);
    auto work1 = ex::then(begin,
        [parent_id]() { HPX_TEST_EQ(parent_id, hpx::this_thread::get_id()); });

    tt::sync_wait(work1);
    loop.finish();
    t.join();
}

void test_transfer_just_one_arg()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    auto begin = ex::transfer_just(sched, 3);
    auto work1 = ex::then(begin, [parent_id](int x) {
        HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
        HPX_TEST_EQ(x, 3);
    });

    tt::sync_wait(work1);
    loop.finish();
    t.join();
}

void test_transfer_just_two_args()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    auto begin = ex::transfer_just(sched, 3, std::string("hello"));
    auto work1 = ex::then(begin, [parent_id](int x, std::string y) {
        HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
        HPX_TEST_EQ(x, 3);
        HPX_TEST_EQ(y, std::string("hello"));
    });

    tt::sync_wait(work1);
    loop.finish();
    t.join();
}

// Note: when_all does not propagate the completion scheduler, for this reason
// any senders coming after it need to be explicitly provided with the required
// scheduler again.
void test_when_all()
{
    ex::run_loop loop;
    hpx::thread t = hpx::thread([&] { loop.run(); });
    hpx::thread::id parent_id = t.get_id();
    [[maybe_unused]] auto sched = loop.get_scheduler();

    {
        auto work1 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            return 42;
        });

        auto work2 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            return std::string("hello");
        });

        auto work3 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            return 3.14;
        });

        auto when1 =
            ex::when_all(std::move(work1), std::move(work2), std::move(work3));

        bool executed{false};

        tt::sync_wait(std::move(when1) |
            ex::then([parent_id, &executed](int x, std::string y, double z) {
                HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
                HPX_TEST_EQ(x, 42);
                HPX_TEST_EQ(y, std::string("hello"));
                HPX_TEST_EQ(z, 3.14);
                executed = true;
            }));

        HPX_TEST(executed);
    }

    {
        // The exception is likely to be thrown before set_value from the second
        // sender is called because the second sender sleeps.
        auto work1 = ex::schedule(sched) | ex::then([parent_id]() -> int {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            throw std::runtime_error("error");
        });

        auto work2 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            return std::string("hello");
        });

        bool exception_thrown = false;

        try
        {
            tt::sync_wait(ex::when_all(std::move(work1), std::move(work2)) |
                ex::then([parent_id](int x, std::string y) {
                    HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
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
        // The exception is likely to be thrown after set_value from the second
        // sender is called because the first sender sleeps before throwing.
        auto work1 = ex::schedule(sched) | ex::then([parent_id]() -> int {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            throw std::runtime_error("error");
        });

        auto work2 = ex::schedule(sched) | ex::then([parent_id]() {
            HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            return std::string("hello");
        });

        bool exception_thrown = false;

        try
        {
            tt::sync_wait(ex::when_all(std::move(work1), std::move(work2)) |
                ex::then([parent_id](int x, std::string y) {
                    HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
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
    loop.finish();
    t.join();
}

// Note: make_future does not propagate the completion scheduler, for this
// reason any senders coming after it need to be explicitly provided with the
// required scheduler again.
void test_future_sender()
{
    std::cout << "1\n";
    // senders as futures
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s = ex::transfer_just(sched, 3);
        auto f = ex::make_future(std::move(s));
        HPX_TEST_EQ(f.get(), 3);
    }

    std::cout << "2\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto f = ex::transfer_just(sched, 3) | ex::make_future();
        HPX_TEST_EQ(f.get(), 3);
    }

    std::cout << "3\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto f = ex::just(3) | ex::make_future(sched);
        HPX_TEST_EQ(f.get(), 3);
    }

    std::cout << "4\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<bool> called{false};
        auto s = ex::schedule(sched) | ex::then([&] { called = true; });
        auto f = ex::make_future(std::move(s));
        f.get();
        HPX_TEST(called);
    }

    std::cout << "5\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s1 = ex::transfer_just(sched, std::size_t(42));
        auto s2 = ex::transfer_just(sched, 3.14);
        auto s3 = ex::transfer_just(sched, std::string("hello"));
        auto f = ex::make_future(sched,
            ex::then(ex::when_all(std::move(s1), std::move(s2), std::move(s3)),
                [](std::size_t x, double, std::string z) {
                    return z.size() + x;
                }));
        HPX_TEST_EQ(f.get(), std::size_t(47));
    }

    // mixing senders and futures
    std::cout << "6\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = tt::sync_wait(
            ex::as_sender(ex::make_future(ex::transfer_just(sched, 42))));
        HPX_TEST_EQ(hpx::get<0>(*result), 42);
    }

    std::cout << "7\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto f = hpx::async([]() {
            hpx::this_thread::sleep_for(std::chrono::seconds(1));
            return 42;
        });

        HPX_TEST_EQ(
            ex::make_future(ex::transfer(ex::as_sender(std::move(f)), sched))
                .get(),
            42);
    }

    std::cout << "8\n";
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s1 = ex::transfer_just(sched, std::size_t(42));
        auto s2 = ex::transfer_just(sched, 3.14);
        auto s3 = ex::transfer_just(sched, std::string("hello"));
        auto f = ex::make_future(sched,
            ex::then(ex::when_all(std::move(s1), std::move(s2), std::move(s3)),
                [](std::size_t x, double, std::string z) {
                    return z.size() + x;
                }));
        auto sf = f.then([](auto&& f) { return f.get() - 40; }).share();
        auto t1 = sf.then([](auto&& sf) { return sf.get() + 1; });
        auto t2 = sf.then([](auto&& sf) { return sf.get() + 2; });
        auto t1s = ex::then(
            ex::as_sender(std::move(t1)), [](std::size_t x) { return x + 1; });
        auto t1f = ex::make_future(sched, std::move(t1s));
        auto last = hpx::dataflow(
            hpx::unwrapping([](std::size_t x, std::size_t y) { return x + y; }),
            t1f, t2);

        HPX_TEST_EQ(last.get(), std::size_t(18));
    }
}

void test_ensure_started()
{
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        tt::sync_wait(ex::schedule(sched) | ex::ensure_started());
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s = ex::transfer_just(sched, 42) | ex::ensure_started();
        auto result = tt::sync_wait(std::move(s));
        HPX_TEST_EQ(hpx::get<0>(*result), 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s = ex::transfer_just(sched, 42) | ex::ensure_started() |
            ex::transfer(sched);
        auto result = tt::sync_wait(std::move(s));
        HPX_TEST_EQ(hpx::get<0>(*result), 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto t = hpx::thread([&] { loop.run(); });
        auto s =
            ex::transfer_just(sched, 42) | ex::ensure_started() | ex::split();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
        loop.finish();
        t.join();
    }
}

void test_ensure_started_when_all()
{
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        }) | ex::ensure_started();
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return started; });
        }

        // P2300R7:
        // The ensure_started sender is move-only and single-shot, so there will
        // only ever be one operation state to be notified on completion.
        auto split_s = ex::split(std::move(s));
        auto succ1 = split_s | ex::then([&]() {
            ++successor_task_calls;
            return 1;
        });
        auto succ2 = split_s | ex::then([&]() {
            ++successor_task_calls;
            return 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*(tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; })))),
            3);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        }) | ex::ensure_started();
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return started; });
        }
        HPX_TEST_EQ(first_task_calls, std::size_t(1));

        auto split_s = ex::split(std::move(s));

        auto succ1 = split_s | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = split_s | ex::then([&](int const& x) {
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*(tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; })))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        }) | ex::ensure_started();
        {
            std::unique_lock l{mtx};
            cond.wait(l, [&]() { return started; });
        }
        auto split_s = ex::split(std::move(s));
        auto succ1 =
            split_s | ex::transfer(sched) | ex::then([&](int const& x) {
                ++successor_task_calls;
                return x + 1;
            });
        auto succ2 =
            split_s | ex::transfer(sched) | ex::then([&](int const& x) {
                ++successor_task_calls;
                return x + 2;
            });
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
        loop.finish();
        t.join();
    }
}

void test_split()
{
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        tt::sync_wait(ex::schedule(sched) | ex::split());
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s = ex::transfer_just(sched, 42) | ex::split();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s =
            ex::transfer_just(sched, 42) | ex::split() | ex::transfer(sched);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto s = ex::transfer_just(sched, 42) | ex::split();
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(s)), 42);
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(std::move(s))), 42);
        loop.finish();
        t.join();
    }
}

void test_split_when_all()
{
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        auto s = ex::schedule(sched) | ex::then([&]() {
            HPX_TEST_EQ(first_task_calls, std::size_t(0));
            HPX_TEST_EQ(successor_task_calls, std::size_t(0));
            ++first_task_calls;
        }) | ex::split();
        auto succ1 = s | ex::then([&]() {
            HPX_TEST_EQ(first_task_calls, std::size_t(1));
            ++successor_task_calls;
            return 1;
        });
        auto succ2 = s | ex::then([&]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            HPX_TEST_EQ(first_task_calls, std::size_t(1));
            ++successor_task_calls;
            return 2;
        });
        /* I cannot decipher the standard enough
         * to figure out if it specifies the expected order
         * of execution within a when_all call, so I have
         * removed the order checks.*/
        HPX_TEST_EQ(
            hpx::get<0>(*tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; }))),
            3);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        auto s = ex::schedule(sched) | ex::then([&]() {
            HPX_TEST_EQ(first_task_calls, std::size_t(0));
            HPX_TEST_EQ(successor_task_calls, std::size_t(0));
            ++first_task_calls;
            return 3;
        }) | ex::split();
        auto succ1 = s | ex::then([&](int const& x) {
            HPX_TEST_EQ(first_task_calls, std::size_t(1));
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = s | ex::then([&](int const& x) {
            HPX_TEST_EQ(first_task_calls, std::size_t(1));
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*(tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; })))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<std::size_t> first_task_calls{0};
        std::atomic<std::size_t> successor_task_calls{0};
        auto s = ex::schedule(sched) | ex::then([&]() {
            HPX_TEST_EQ(first_task_calls, std::size_t(0));
            HPX_TEST_EQ(successor_task_calls, std::size_t(0));
            ++first_task_calls;
            return 3;
        }) | ex::split();
        auto succ1 = s | ex::transfer(sched) | ex::then([&](int const& x) {
            HPX_TEST_EQ(first_task_calls, std::size_t(1));
            ++successor_task_calls;
            return x + 1;
        });
        auto succ2 = s | ex::transfer(sched) | ex::then([&](int const& x) {
            HPX_TEST_EQ(first_task_calls, std::size_t(1));
            ++successor_task_calls;
            return x + 2;
        });
        HPX_TEST_EQ(
            hpx::get<0>(*(tt::sync_wait(ex::when_all(succ1, succ2) |
                ex::then([](int const& x, int const& y) { return x + y; })))),
            9);
        HPX_TEST_EQ(first_task_calls, std::size_t(1));
        HPX_TEST_EQ(successor_task_calls, std::size_t(2));
        loop.finish();
        t.join();
    }
}

void test_let_value()
{
    // void predecessor
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(ex::schedule(sched) |
            ex::let_value([]() { return ex::just(42); })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(ex::schedule(sched) |
            ex::let_value([=]() { return ex::transfer_just(sched, 42); })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(ex::just() |
            ex::let_value([=]() { return ex::transfer_just(sched, 42); })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    // int predecessor, value ignored
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*(tt::sync_wait(ex::transfer_just(sched, 43) |
            ex::let_value([](int&) { return ex::just(42); }))));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*(tt::sync_wait(ex::transfer_just(sched, 43) |
            ex::let_value(
                [=](int&) { return ex::transfer_just(sched, 42); }))));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result =
            hpx::get<0>(*(tt::sync_wait(ex::just(43) | ex::let_value([=](int&) {
                return ex::transfer_just(sched, 42);
            }))));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    // int predecessor, value used
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(
            ex::transfer_just(sched, 43) | ex::let_value([](int& x) {
                return ex::just(42) | ex::then([&](int y) { return x + y; });
            })));
        HPX_TEST_EQ(result, 85);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(
            ex::transfer_just(sched, 43) | ex::let_value([=](int& x) {
                return ex::transfer_just(sched, 42) |
                    ex::then([&](int y) { return x + y; });
            })));
        HPX_TEST_EQ(result, 85);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(
            *tt::sync_wait(ex::just(43) | ex::let_value([=](int& x) {
                return ex::transfer_just(sched, 42) |
                    ex::then([&](int y) { return x + y; });
            })));
        HPX_TEST_EQ(result, 85);
        loop.finish();
        t.join();
    }

    // predecessor throws, let sender is ignored
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        bool exception_thrown = false;

        try
        {
            tt::sync_wait(ex::transfer_just(sched, 43) |
                ex::then(
                    [](int) -> int { throw std::runtime_error("error"); }) |
                ex::let_value([](int&) {
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
        loop.finish();
        t.join();
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
    // void predecessor
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<bool> called{false};
        tt::sync_wait(ex::schedule(sched) | ex::then([]() {
            throw std::runtime_error("error");
        }) | ex::let_error([&called](std::exception_ptr& ep) {
            called = true;
            check_exception_ptr_message(ep, "error");
            return ex::just();
        }));
        HPX_TEST(called);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<bool> called{false};
        tt::sync_wait(ex::schedule(sched) | ex::then([]() {
            throw std::runtime_error("error");
        }) | ex::let_error([=, &called](std::exception_ptr& ep) {
            called = true;
            check_exception_ptr_message(ep, "error");
            return ex::transfer_just(sched);
        }));
        HPX_TEST(called);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        std::atomic<bool> called{false};
        tt::sync_wait(ex::just() | ex::then([]() {
            throw std::runtime_error("error");
        }) | ex::let_error([=, &called](std::exception_ptr& ep) {
            called = true;
            check_exception_ptr_message(ep, "error");
            return ex::transfer_just(sched);
        }));
        HPX_TEST(called);
        loop.finish();
        t.join();
    }

    // int predecessor
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result =
            hpx::get<0>(*tt::sync_wait(ex::schedule(sched) | ex::then([]() {
                throw std::runtime_error("error");
                return 43;
            }) | ex::let_error([](std::exception_ptr& ep) {
                check_exception_ptr_message(ep, "error");
                return ex::just(42);
            })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result =
            hpx::get<0>(*tt::sync_wait(ex::schedule(sched) | ex::then([]() {
                throw std::runtime_error("error");
                return 43;
            }) | ex::let_error([=](std::exception_ptr& ep) {
                check_exception_ptr_message(ep, "error");
                return ex::transfer_just(sched, 42);
            })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(ex::just() | ex::then([]() {
            throw std::runtime_error("error");
            return 43;
        }) | ex::let_error([=](std::exception_ptr& ep) {
            check_exception_ptr_message(ep, "error");
            return ex::transfer_just(sched, 42);
        })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    // predecessor doesn't throw, let sender is ignored
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(ex::transfer_just(sched, 42) |
            ex::let_error([](std::exception_ptr) {
                HPX_TEST(false);
                return ex::just(43);
            })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto result = hpx::get<0>(*tt::sync_wait(ex::transfer_just(sched, 42) |
            ex::let_error([=](std::exception_ptr) {
                HPX_TEST(false);
                return ex::transfer_just(sched, 43);
            })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto result = hpx::get<0>(*tt::sync_wait(
            ex::just(42) | ex::let_error([=](std::exception_ptr) {
                HPX_TEST(false);
                return ex::transfer_just(sched, 43);
            })));
        HPX_TEST_EQ(result, 42);
        loop.finish();
        t.join();
    }
}

void test_detach()
{
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        loop.finish();
        t.join();
    }

    // Values passed to set_value are ignored
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        loop.finish();
        t.join();
    }
}

void test_keep_future_sender()
{
    // the future should be passed to then, not it's contained value
    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<void>()) |
            ex::then([](hpx::future<void>&& f) { HPX_TEST(f.is_ready()); }));
    }

    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<void>().share()) |
            ex::then(
                [](hpx::shared_future<void>&& f) { HPX_TEST(f.is_ready()); }));
    }

    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<int>(42)) |
            ex::then([](hpx::future<int>&& f) {
                HPX_TEST(f.is_ready());
                HPX_TEST_EQ(f.get(), 42);
            }));
    }

    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        tt::sync_wait(ex::keep_future(hpx::make_ready_future<int>(42).share()) |
            ex::then([](hpx::shared_future<int>&& f) {
                HPX_TEST(f.is_ready());
                HPX_TEST_EQ(f.get(), 42);
            }));
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();

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
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto f = hpx::async([&]() { return 42; });
        auto r = hpx::get<0>(*tt::sync_wait(
            ex::keep_future(std::move(f)) | ex::transfer(sched)));
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get(), 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        [[maybe_unused]] auto sched = loop.get_scheduler();
        auto t = hpx::thread([&] { loop.run(); });
        auto sf = hpx::async([&]() { return 42; }).share();
        auto r = hpx::get<0>(*tt::sync_wait(
            ex::keep_future(std::move(sf)) | ex::transfer(sched)));
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get(), 42);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto sf = hpx::async([&]() {
            return custom_type_non_default_constructible_non_copyable{42};
        }).share();

        // NOTE: Without keep_future this should fail to compile, since
        // sync_wait would receive a const& to the value which requires a copy
        // or storing a const&. The copy is not possible because the type is
        // noncopyable, and storing a reference is not acceptable since the
        // reference may outlive the value.
        auto r = hpx::get<0>(*tt::sync_wait(
            ex::keep_future(std::move(sf)) | ex::transfer(sched)));
        HPX_TEST(r.is_ready());
        HPX_TEST_EQ(r.get().x, 42);
        loop.finish();
        t.join();
    }

    // Use unwrapping with keep_future
    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto f = hpx::async([]() { return 42; });
        auto sf = hpx::async([]() { return 3.14; }).share();

        auto fun = hpx::unwrapping(
            [](int&& x, double const& y) { return x * 2 + (int(y) / 2); });
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(
                        ex::when_all(ex::keep_future(std::move(f)),
                            ex::keep_future(std::move(sf))) |
                        ex::then(fun))),
            85);
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        auto f = hpx::async([]() { return 42; });
        auto sf = hpx::async([]() { return 3.14; }).share();

        auto fun = hpx::unwrapping(
            [](int&& x, double const& y) { return x * 2 + (int(y) / 2); });
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(
                        ex::when_all(ex::keep_future(std::move(f)),
                            ex::keep_future(sf)) |
                        ex::transfer(sched) | ex::then(fun))),
            85);

        loop.finish();
        t.join();
    }
}

void test_bulk()
{
    std::vector<int> const ns = {0, 1, 10, 43};

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        for (int n : ns)
        {
            std::vector<int> v(n, 0);
            hpx::thread::id parent_id = t.get_id();
            tt::sync_wait(ex::schedule(sched) | ex::bulk(n, [&](int i) {
                ++v[i];
                HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
            }));
            for (int i = 0; i < n; ++i)
            {
                HPX_TEST_EQ(v[i], 1);
            }
        }
        loop.finish();
        t.join();
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        for (auto n : ns)
        {
            std::vector<int> v(n, -1);
            hpx::thread::id parent_id = t.get_id();
            auto v_out = hpx::get<0>(
                *tt::sync_wait(ex::transfer_just(sched, std::move(v)) |
                    ex::bulk(n, [&parent_id](int i, std::vector<int>& v) {
                        v[i] = i;
                        HPX_TEST_EQ(parent_id, hpx::this_thread::get_id());
                    })));

            for (int i = 0; i < n; ++i)
            {
                HPX_TEST_EQ(v_out[i], i);
            }
        }
        loop.finish();
        t.join();
    }

    {
    }

    {
        ex::run_loop loop;
        auto t = hpx::thread([&] { loop.run(); });
        [[maybe_unused]] auto sched = loop.get_scheduler();

        for (auto n : ns)
        {
            int i_fail = 3;

            std::vector<int> v(n, -1);
            bool const expect_exception = n > i_fail;

            try
            {
                tt::sync_wait(
                    ex::transfer_just(sched) | ex::bulk(n, [&v, i_fail](int i) {
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

                HPX_TEST_EQ(std::string(e.what()), std::string("error"));
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
        loop.finish();
        t.join();
    }
}
// NOLINTEND(bugprone-unchecked-optional-access)

void test_completion_scheduler()
{
    ex::run_loop loop;
    [[maybe_unused]] auto sched = loop.get_scheduler();

    {
        auto sender = ex::schedule(sched);
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                decltype(sched)>,
            "the completion scheduler should be a run_pool_scheduler");
    }

    {
        auto sender = ex::then(ex::schedule(sched), []() {});
        using hpx::functional::tag_invoke;
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                decltype(sched)>,
            "the completion scheduler should be a run_pool_scheduler");
    }

    {
        auto sender = ex::transfer_just(sched, 42);
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                decltype(sched)>,
            "the completion scheduler should be a run_pool_scheduler");
    }

    {
        auto sender = ex::bulk(ex::schedule(sched), 10, [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                decltype(sched)>,
            "the completion scheduler should be a run_pool_scheduler");
    }

    {
        auto sender = ex::then(
            ex::bulk(ex::transfer_just(sched, 42), 10, [](int, int) {}),
            [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                decltype(sched)>,
            "the completion scheduler should be a run_pool_scheduler");
    }

    {
        auto sender =
            ex::bulk((ex::transfer_just(sched, 42) | ex::then([](int) {})), 10,
                [](int) {});
        auto completion_scheduler =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(sender));
        static_assert(
            std::is_same_v<std::decay_t<decltype(completion_scheduler)>,
                decltype(sched)>,
            "the completion scheduler should be a run_pool_scheduler");
    }

    loop.finish();
    loop.run();
}

void do_run_test(void (*func)(), char const* func_name)
{
    std::cout << func_name << "\n";
    func();
}

#define RUN_TEST(func)                                                         \
    std::cout << "Running test: ";                                             \
    do_run_test(&func, HPX_PP_STRINGIZE(func));

int hpx_main()
{
    RUN_TEST(test_concepts);
    RUN_TEST(test_execute);
    RUN_TEST(test_sender_receiver_basic);
    RUN_TEST(test_sender_receiver_then);
    RUN_TEST(test_sender_receiver_then_wait);
    RUN_TEST(test_sender_receiver_then_sync_wait);
    RUN_TEST(test_sender_receiver_then_arguments);
    RUN_TEST(test_transfer_basic);
    RUN_TEST(test_transfer_arguments);
    RUN_TEST(test_just_void);
    RUN_TEST(test_just_one_arg);
    RUN_TEST(test_just_two_args);
    RUN_TEST(test_transfer_just_void);
    RUN_TEST(test_transfer_just_one_arg);
    RUN_TEST(test_transfer_just_two_args);
    RUN_TEST(test_when_all);
    RUN_TEST(test_future_sender);
    RUN_TEST(test_keep_future_sender);
    RUN_TEST(test_ensure_started);
    RUN_TEST(test_ensure_started_when_all);
    RUN_TEST(test_split);
    RUN_TEST(test_split_when_all);
    RUN_TEST(test_let_value);
    RUN_TEST(test_let_error);
    RUN_TEST(test_detach);
    RUN_TEST(test_bulk);

    RUN_TEST(test_completion_scheduler);

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

int main(int, char*[])
{
    return 0;
}

#endif
