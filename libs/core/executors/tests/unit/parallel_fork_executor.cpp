//  Copyright (c) 2007-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id test(int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

void test_sync()
{
    typedef hpx::execution::parallel_executor executor;

    executor exec(hpx::launch::fork);
    HPX_TEST(hpx::parallel::execution::sync_execute(exec, &test, 42) ==
        hpx::this_thread::get_id());
}

void test_async()
{
    typedef hpx::execution::parallel_executor executor;

    executor exec(hpx::launch::fork);
    HPX_TEST(hpx::parallel::execution::async_execute(exec, &test, 42).get() !=
        hpx::this_thread::get_id());
}

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id test_f(hpx::future<void> f, int passed_through)
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

void test_then()
{
    typedef hpx::execution::parallel_executor executor;

    hpx::future<void> f = hpx::make_ready_future();

    executor exec(hpx::launch::fork);
    HPX_TEST(
        hpx::parallel::execution::then_execute(exec, &test_f, std::move(f), 42)
            .get() != hpx::this_thread::get_id());
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test(int, hpx::thread::id tid, int passed_through)    //-V813
{
    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

void test_bulk_sync()
{
    typedef hpx::execution::parallel_executor executor;

    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executor exec(hpx::launch::fork);

    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.sharing_mode(hpx::threads::thread_sharing_hint::do_not_share_function |
        hpx::threads::thread_sharing_hint::do_not_combine_tasks);
    auto no_sharing_exec = hpx::execution::to_hierarchical_spawning(
        hpx::execution::experimental::with_hint(exec, hint));

    hpx::parallel::execution::bulk_sync_execute(
        no_sharing_exec, hpx::bind(&bulk_test, _1, tid, _2), v, 42);
    hpx::parallel::execution::bulk_sync_execute(
        no_sharing_exec, &bulk_test, v, tid, 42);
}

void test_bulk_async()
{
    typedef hpx::execution::parallel_executor executor;

    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executor exec(hpx::launch::fork);
    hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                      exec, hpx::bind(&bulk_test, _1, tid, _2), v, 42))
        .get();
    hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                      exec, &bulk_test, v, tid, 42))
        .get();
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_f(int, hpx::shared_future<void> f, hpx::thread::id tid,
    int passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

void test_bulk_then()
{
    typedef hpx::execution::parallel_executor executor;

    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;

    hpx::shared_future<void> f = hpx::make_ready_future();

    executor exec(hpx::launch::fork);
    hpx::parallel::execution::bulk_then_execute(
        exec, hpx::bind(&bulk_test_f, _1, _2, tid, _3), v, f, 42)
        .get();
    hpx::parallel::execution::bulk_then_execute(
        exec, &bulk_test_f, v, f, tid, 42)
        .get();
}

void static_check_executor()
{
    using namespace hpx::traits;
    using executor = hpx::execution::parallel_executor;

    static_assert(
        is_one_way_executor_v<executor>, "is_one_way_executor_v<executor>");
    static_assert(is_never_blocking_one_way_executor_v<executor>,
        "is_never_blocking_one_way_executor_v<executor>");
    static_assert(
        is_two_way_executor_v<executor>, "is_two_way_executor_v<executor>");
    static_assert(is_bulk_two_way_executor_v<executor>,
        "is_bulk_two_way_executor_v<executor>");
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    static_check_executor();

    test_sync();
    test_async();
    test_then();

    test_bulk_sync();
    test_bulk_async();
    test_bulk_then();

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
