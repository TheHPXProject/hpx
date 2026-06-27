//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id async_test(int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

void apply_test(hpx::latch& l, hpx::thread::id& id, int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    id = hpx::this_thread::get_id();
    l.count_down(1);
}

void async_bulk_test(
    int, hpx::thread::id const& tid, int passed_through)    //-V813
{
    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_apply(Executor& exec)
{
    hpx::latch l(2);
    hpx::thread::id id;

    hpx::parallel::execution::post(
        exec, &apply_test, std::ref(l), std::ref(id), 42);
    l.arrive_and_wait();

    HPX_TEST_NEQ(id, hpx::this_thread::get_id());
}

template <typename Executor>
void test_sync(Executor&& exec)
{
    HPX_TEST(hpx::parallel::execution::sync_execute(exec, &async_test, 42) !=
        hpx::this_thread::get_id());
}

template <typename Executor>
void test_async(Executor&& exec)
{
    HPX_TEST(
        hpx::parallel::execution::async_execute(exec, &async_test, 42).get() !=
        hpx::this_thread::get_id());
}

template <typename Executor>
void test_bulk_sync(Executor&& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    hpx::parallel::execution::bulk_sync_execute(
        exec, hpx::bind(&async_bulk_test, _1, tid, _2), v, 42);
    hpx::parallel::execution::bulk_sync_execute(
        exec, &async_bulk_test, v, tid, 42);
}

template <typename Executor>
void test_bulk_async(Executor&& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                      exec, hpx::bind(&async_bulk_test, _1, tid, _2), v, 42))
        .get();
    hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                      exec, &async_bulk_test, v, tid, 42))
        .get();
}

std::atomic<std::size_t> count_apply(0);
std::atomic<std::size_t> count_sync(0);
std::atomic<std::size_t> count_async(0);
std::atomic<std::size_t> count_bulk_sync(0);
std::atomic<std::size_t> count_bulk_async(0);

template <typename Executor>
void test_executor(std::array<std::size_t, 5> expected)
{
    using execution_category =
        hpx::traits::executor_execution_category_t<Executor>;

    HPX_TEST((std::is_same_v<hpx::execution::parallel_execution_tag,
        execution_category>) );

    count_apply.store(0);
    count_sync.store(0);
    count_async.store(0);
    count_bulk_sync.store(0);
    count_bulk_async.store(0);

    Executor exec;

    test_apply(exec);
    test_sync(exec);
    test_async(exec);
    test_bulk_sync(exec);
    test_bulk_async(exec);

    HPX_TEST_EQ(expected[0], count_apply.load());
    HPX_TEST_EQ(expected[1], count_sync.load());
    HPX_TEST_EQ(expected[2], count_async.load());
    HPX_TEST_EQ(expected[3], count_bulk_sync.load());
    HPX_TEST_EQ(expected[4], count_bulk_async.load());
}

///////////////////////////////////////////////////////////////////////////////
struct test_async_executor1
{
    using execution_category = hpx::execution::parallel_execution_tag;

    template <typename F, typename... Ts>
    decltype(auto) async_execute(F&& f, Ts&&... ts) const
    {
        ++count_async;

        auto policy = hpx::launch::async;
        auto hint = policy.hint();
        hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
        policy.set_hint(hint);

        return hpx::async(policy, std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<test_async_executor1>
  : std::true_type
{
};

struct test_async_executor2 : test_async_executor1
{
    using execution_category = hpx::execution::parallel_execution_tag;

    template <typename F, typename... Ts>
    decltype(auto) sync_execute(F&& f, Ts&&... ts) const
    {
        ++count_sync;

        auto policy = hpx::launch::async;
        auto hint = policy.hint();
        hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
        policy.set_hint(hint);

        return hpx::async(policy, std::forward<F>(f), std::forward<Ts>(ts)...)
            .get();
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<test_async_executor2>
  : std::true_type
{
};

struct test_async_executor3 : test_async_executor1
{
    using execution_category = hpx::execution::parallel_execution_tag;

    template <typename F, typename Shape, typename... Ts>
    decltype(auto) bulk_sync_execute(F f, Shape const& shape, Ts&&... ts) const
    {
        ++count_bulk_sync;

        auto policy = hpx::launch::async;
        auto hint = policy.hint();
        hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
        policy.set_hint(hint);

        std::vector<hpx::future<void>> results;
        for (auto const& elem : shape)
        {
            results.push_back(hpx::async(policy, f, elem, ts...));
        }
        hpx::when_all(results).get();
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<test_async_executor3>
  : std::true_type
{
};

struct test_async_executor4 : test_async_executor1
{
    using execution_category = hpx::execution::parallel_execution_tag;

    template <typename F, typename Shape, typename... Ts>
    decltype(auto) bulk_async_execute(F f, Shape const& shape, Ts&&... ts) const
    {
        ++count_bulk_async;

        auto policy = hpx::launch::async;
        auto hint = policy.hint();
        hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
        policy.set_hint(hint);

        std::vector<hpx::future<void>> results;
        for (auto const& elem : shape)
        {
            results.push_back(hpx::async(policy, f, elem, ts...));
        }
        return results;
    }
};

namespace hpx::execution::experimental {

    template <>
    struct is_two_way_executor<test_async_executor4> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<test_async_executor4> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

struct test_async_executor5 : test_async_executor1
{
    using execution_category = hpx::execution::parallel_execution_tag;

    template <typename F, typename... Ts>
    decltype(auto) post(F&& f, Ts&&... ts) const
    {
        ++count_apply;
        hpx::post(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<test_async_executor5>
  : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_executor<test_async_executor1>({{0, 0, 431, 0, 0}});
    test_executor<test_async_executor2>({{0, 1, 430, 0, 0}});
    test_executor<test_async_executor3>({{0, 0, 217, 2, 0}});
    test_executor<test_async_executor4>({{0, 0, 217, 0, 2}});
    test_executor<test_async_executor5>({{1, 0, 430, 0, 0}});

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
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
