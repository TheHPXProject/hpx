//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
decltype(auto) disable_run_as_child(Executor&& exec)
{
    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);

    return hpx::experimental::prefer(hpx::execution::experimental::with_hint,
        HPX_FORWARD(Executor, exec), hint);
}

hpx::thread::id test(int const passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

template <typename Policy>
void test_sync()
{
    using executor = hpx::execution::parallel_policy_executor<Policy>;

    executor exec;
    HPX_TEST(hpx::parallel::execution::sync_execute(exec, &test, 42) ==
        hpx::this_thread::get_id());
}

template <typename Policy>
void test_async(bool const sync)
{
    using executor = hpx::execution::parallel_policy_executor<Policy>;

    executor exec;
    bool const result = hpx::parallel::execution::async_execute(
                            disable_run_as_child(exec), &test, 42)
                            .get() == hpx::this_thread::get_id();

    HPX_TEST_EQ(sync, result);
}

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id test_f(hpx::future<void> f, int const passed_through)
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

template <typename Policy>
void test_then(bool const sync)
{
    using executor = hpx::execution::parallel_policy_executor<Policy>;

    hpx::future<void> f = hpx::make_ready_future();

    executor exec;
    bool const result =
        hpx::parallel::execution::then_execute(exec, &test_f, std::move(f), 42)
            .get() == hpx::this_thread::get_id();

    HPX_TEST_EQ(sync, result);
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_s(
    int, hpx::thread::id const& tid, int const passed_through)    //-V813
{
    HPX_TEST_EQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

std::atomic<int> count_inline(0);

void bulk_test_a(
    int, hpx::thread::id const& tid, int const passed_through)    //-V813
{
    if (tid == hpx::this_thread::get_id())
    {
        ++count_inline;
    }
    HPX_TEST_EQ(passed_through, 42);
}

template <typename Policy>
void test_bulk_sync(bool const sync)
{
    using executor = hpx::execution::parallel_policy_executor<Policy>;

    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executor exec;

    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.sharing_mode(hpx::threads::thread_sharing_hint::do_not_share_function |
        hpx::threads::thread_sharing_hint::do_not_combine_tasks);
    auto no_sharing_exec = hpx::execution::to_hierarchical_spawning(
        hpx::execution::experimental::with_hint(exec, hint));

    count_inline.store(0);
    hpx::parallel::execution::bulk_sync_execute(no_sharing_exec,
        hpx::bind(sync ? &bulk_test_s : &bulk_test_a, _1, tid, _2), v, 42);
    HPX_TEST_LTE(count_inline.load(), 1);

    count_inline.store(0);
    hpx::parallel::execution::bulk_sync_execute(
        no_sharing_exec, sync ? &bulk_test_s : &bulk_test_a, v, tid, 42);
    HPX_TEST_LTE(count_inline.load(), 1);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Policy>
void test_bulk_async(bool const sync)
{
    using executor = hpx::execution::parallel_policy_executor<Policy>;

    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executor exec;

    count_inline.store(0);
    hpx::when_all(
        hpx::parallel::execution::bulk_async_execute(disable_run_as_child(exec),
            hpx::bind(sync ? &bulk_test_s : &bulk_test_a, _1, tid, _2), v, 42))
        .get();
    HPX_TEST_LTE(count_inline.load(), 1);

    count_inline.store(0);
    hpx::when_all(
        hpx::parallel::execution::bulk_async_execute(disable_run_as_child(exec),
            sync ? &bulk_test_s : &bulk_test_a, v, tid, 42))
        .get();
    HPX_TEST_LTE(count_inline.load(), 1);
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_f_s(int, hpx::shared_future<void> const& f,
    hpx::thread::id const& tid,
    int const passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

void bulk_test_f_a(int, hpx::shared_future<void> const& f,
    hpx::thread::id const& tid,
    int const passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

template <typename Policy>
void test_bulk_then(bool const sync)
{
    using executor = hpx::execution::parallel_policy_executor<Policy>;

    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;

    hpx::shared_future<void> f = hpx::make_ready_future();

    executor exec;
    hpx::parallel::execution::bulk_then_execute(exec,
        hpx::bind(sync ? &bulk_test_f_s : &bulk_test_f_a, _1, _2, tid, _3), v,
        f, 42)
        .get();
    hpx::parallel::execution::bulk_then_execute(
        exec, sync ? &bulk_test_f_s : &bulk_test_f_a, v, f, tid, 42)
        .get();
}

template <typename Policy>
void static_check_executor()
{
    using namespace hpx::traits;
    using executor = hpx::execution::parallel_policy_executor<Policy>;

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
template <typename Policy>
void policy_test(bool const sync = false)
{
    static_check_executor<Policy>();

    test_sync<Policy>();
    test_async<Policy>(sync);
    test_then<Policy>(sync);

    test_bulk_sync<Policy>(sync);
    test_bulk_async<Policy>(sync);
    test_bulk_then<Policy>(sync);
}

int hpx_main()
{
    policy_test<hpx::launch>();

    policy_test<hpx::launch::async_policy>();
    policy_test<hpx::launch::sync_policy>(true);
    policy_test<hpx::launch::fork_policy>();
    policy_test<hpx::launch::deferred_policy>(true);

    return hpx::local::finalize();
}

int main(int const argc, char* argv[])
{
    // By default, this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
