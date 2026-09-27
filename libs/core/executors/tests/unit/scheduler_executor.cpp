//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// Clang V11 ICE's on this test, Clang V8 reports a bogus constexpr problem
#if !defined(HPX_CLANG_VERSION) ||                                             \
    ((HPX_CLANG_VERSION / 10000) != 11 && (HPX_CLANG_VERSION / 10000) != 8)

#include "bulk_shape.hpp"

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <stdexec/execution.hpp>

#include <array>
#include <atomic>
#include <cstdlib>
#include <forward_list>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
bool executed = false;

void test_post_f(int passed_through, hpx::latch& l)
{
    HPX_TEST_EQ(passed_through, 42);

    executed = true;

    l.count_down(1);
}

template <typename Executor>
void test_post(Executor&& exec)
{
    executed = false;

    hpx::latch l(2);
    hpx::parallel::execution::post(exec, &test_post_f, 42, std::ref(l));
    l.arrive_and_wait();

    HPX_TEST(executed);
}

///////////////////////////////////////////////////////////////////////////////
void test(int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);

    executed = true;
}

template <typename Executor>
void test_sync(Executor&& exec)
{
    executed = false;

    hpx::parallel::execution::sync_execute(exec, &test, 42);

    HPX_TEST(executed);
}

template <typename Executor>
void test_async(Executor&& exec)
{
    executed = false;

    hpx::parallel::execution::async_execute(exec, &test, 42).get();

    HPX_TEST(executed);
}

///////////////////////////////////////////////////////////////////////////////
void test_f(hpx::future<void> f, int passed_through)
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);

    executed = true;
}

template <typename Executor>
void test_then(Executor&& exec)
{
    hpx::future<void> f = hpx::make_ready_future();

    executed = false;

    hpx::parallel::execution::then_execute(exec, &test_f, std::move(f), 42)
        .get();

    HPX_TEST(executed);
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_void(int seq, int passed_through)    //-V813
{
    HPX_TEST_EQ(passed_through, 42);

    if (seq == 0)
    {
        executed = true;
    }
}

int bulk_test(int seq, int passed_through)    //-V813
{
    HPX_TEST_EQ(passed_through, 42);

    if (seq == 0)
    {
        executed = true;
    }

    return seq;
}

template <typename Executor>
void test_bulk_sync_void(Executor&& exec)
{
    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executed = false;

    hpx::parallel::execution::bulk_sync_execute(
        exec, hpx::bind(&bulk_test, _1, _2), 107, 42);

    HPX_TEST(executed);

    executed = false;

    hpx::parallel::execution::bulk_sync_execute(exec, &bulk_test, 107, 42);

    HPX_TEST(executed);
}

template <typename Executor>
void test_bulk_async_void(Executor&& exec)
{
    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executed = false;

    auto result = hpx::parallel::execution::bulk_async_execute(
        exec, hpx::bind(&bulk_test, _1, _2), 107, 42);
    hpx::when_all(std::move(result)).get();

    HPX_TEST(executed);

    executed = false;

    hpx::when_all(
        hpx::parallel::execution::bulk_async_execute(exec, &bulk_test, 107, 42))
        .get();

    HPX_TEST(executed);
}

template <typename Executor>
void test_bulk_async(Executor&& exec)
{
    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executed = false;
    int const n = 107;

    auto fut_result = hpx::parallel::execution::bulk_async_execute(
        exec, hpx::bind(&bulk_test, _1, _2), n, 42);
    auto result = hpx::when_all(std::move(fut_result)).get();

    for (int i = 0; i < n; ++i)
    {
        HPX_TEST_EQ(i, result[i].get());
    }

    HPX_TEST(executed);

    executed = false;

    hpx::when_all(
        hpx::parallel::execution::bulk_async_execute(exec, &bulk_test, 107, 42))
        .get();

    HPX_TEST(executed);
}

///////////////////////////////////////////////////////////////////////////////
int bulk_test_f(int seq, hpx::shared_future<void> f,
    int passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);

    if (seq == 0)
    {
        executed = true;
    }

    return seq;
}

template <typename Executor>
void test_bulk_then(Executor&& exec)
{
    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;

    hpx::shared_future<void> f = hpx::make_ready_future();

    {
        executed = false;

        auto result = hpx::parallel::execution::bulk_then_execute(
            exec, hpx::bind(&bulk_test_f, _1, _2, _3), 107, f, 42)
                          .get();

        HPX_TEST(executed);
        HPX_TEST(result.size() == 107);

        int expected = 0;
        for (auto i : result)
        {
            HPX_TEST_EQ(i, expected);
            ++expected;
        }
    }

    {
        executed = false;

        auto result = hpx::parallel::execution::bulk_then_execute(
            exec, &bulk_test_f, 107, f, 42)
                          .get();

        HPX_TEST(executed);
        HPX_TEST(result.size() == 107);

        int expected = 0;
        for (auto i : result)
        {
            HPX_TEST_EQ(i, expected);
            ++expected;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_f_void(int seq, hpx::shared_future<void> f,
    int passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);

    if (seq == 0)
    {
        executed = true;
    }
}

template <typename Executor>
void test_bulk_then_void(Executor&& exec)
{
    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;

    hpx::shared_future<void> f = hpx::make_ready_future();

    executed = false;

    hpx::parallel::execution::bulk_then_execute(
        exec, hpx::bind(&bulk_test_f_void, _1, _2, _3), 107, f, 42)
        .get();

    HPX_TEST(executed);

    executed = false;

    hpx::parallel::execution::bulk_then_execute(
        exec, &bulk_test_f_void, 107, f, 42)
        .get();

    HPX_TEST(executed);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_unsized_shape(Executor& exec)
{
    std::forward_list<int> input{0, 1, 2};
    hpx::util::iterator_range shape(input);
    static_assert(!std::ranges::sized_range<decltype(shape)>);
    std::array<std::atomic<int>, 3> visits{};
    auto work = hpx::parallel::execution::bulk_async_execute(
        exec, [&visits](int i) { ++visits[i]; }, shape);
    hpx::wait_all(work);
    for (auto const& count : visits)
        HPX_TEST_EQ(count.load(), 1);
}

template <typename Executor>
void test_executor(Executor&& exec)
{
    executor_test::test_bulk_shape<true>(exec);
    test_unsized_shape(exec);
    test_post(exec);

    test_sync(exec);
    test_async(exec);
    test_then(exec);

    test_bulk_sync_void(exec);
    test_bulk_async_void(exec);
    test_bulk_async(exec);
    test_bulk_then(exec);
    test_bulk_then_void(exec);
}

int hpx_main()
{
    using namespace hpx::execution::experimental;

    scheduler_executor exec(thread_pool_scheduler{});

    test_executor(exec);

    scheduler_executor generic_exec(stdexec::inline_scheduler{});
    executor_test::test_bulk_shape<true>(generic_exec);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#else
int main()
{
    return 0;
}
#endif
