//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono;

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id sync_test(int passed_through)
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

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_timed_apply(Executor& exec)
{
    {
        hpx::latch l(2);
        hpx::thread::id id;

        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, milliseconds(10));

        hpx::parallel::execution::post(
            timed_exec, &apply_test, std::ref(l), std::ref(id), 42);

        l.arrive_and_wait();

        HPX_TEST_EQ(id, hpx::this_thread::get_id());
    }

    {
        hpx::latch l(2);
        hpx::thread::id id;

        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, steady_clock::now() + milliseconds(10));

        hpx::parallel::execution::post(
            timed_exec, &apply_test, std::ref(l), std::ref(id), 42);

        l.arrive_and_wait();

        HPX_TEST_EQ(id, hpx::this_thread::get_id());
    }
}

template <typename Executor>
void test_timed_sync(Executor& exec)
{
    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, milliseconds(10));

        HPX_TEST(hpx::parallel::execution::sync_execute(
                     timed_exec, &sync_test, 42) == hpx::this_thread::get_id());
    }

    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, steady_clock::now() + milliseconds(10));

        HPX_TEST(hpx::parallel::execution::sync_execute(
                     timed_exec, &sync_test, 42) == hpx::this_thread::get_id());
    }
}

template <typename Executor>
void test_timed_async(Executor& exec)
{
    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, milliseconds(10));

        HPX_TEST(
            hpx::parallel::execution::async_execute(timed_exec, &sync_test, 42)
                .get() == hpx::this_thread::get_id());
    }

    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, steady_clock::now() + milliseconds(10));

        HPX_TEST(
            hpx::parallel::execution::async_execute(timed_exec, &sync_test, 42)
                .get() == hpx::this_thread::get_id());
    }
}

std::atomic<std::size_t> count_sync(0);
std::atomic<std::size_t> count_apply(0);
std::atomic<std::size_t> count_sync_at(0);
std::atomic<std::size_t> count_apply_at(0);

template <typename Executor>
void test_timed_executor(std::array<std::size_t, 4> expected)
{
    typedef typename hpx::traits::executor_execution_category<Executor>::type
        execution_category;

    HPX_TEST((std::is_same<hpx::execution::sequenced_execution_tag,
        execution_category>::value));

    count_sync.store(0);
    count_apply.store(0);
    count_sync_at.store(0);
    count_apply_at.store(0);

    Executor exec;

    test_timed_apply(exec);
    test_timed_sync(exec);
    test_timed_async(exec);

    HPX_TEST_EQ(expected[0], count_sync.load());
    HPX_TEST_EQ(expected[1], count_apply.load());
    HPX_TEST_EQ(expected[2], count_sync_at.load());
    HPX_TEST_EQ(expected[3], count_apply_at.load());
}

///////////////////////////////////////////////////////////////////////////////
struct test_sync_executor1
{
    typedef hpx::execution::sequenced_execution_tag execution_category;

    template <typename F, typename... Ts>
    decltype(auto) sync_execute(F&& f, Ts&&... ts) const
    {
        ++count_sync;
        return hpx::invoke(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

struct test_timed_sync_executor1 : test_sync_executor1
{
    typedef hpx::execution::sequenced_execution_tag execution_category;

    template <typename F, typename... Ts>
    decltype(auto) sync_execute_at(
        hpx::chrono::steady_time_point const& abs_time, F&& f, Ts&&... ts) const
    {
        ++count_sync_at;
        hpx::this_thread::sleep_until(abs_time);
        return hpx::invoke(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

namespace hpx::execution::experimental {
    template <>
    struct is_one_way_executor<test_sync_executor1> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<test_timed_sync_executor1> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

struct test_sync_executor2 : test_sync_executor1
{
    typedef hpx::execution::sequenced_execution_tag execution_category;

    template <typename F, typename... Ts>
    decltype(auto) post(F&& f, Ts&&... ts) const
    {
        ++count_apply;
        hpx::invoke(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

struct test_timed_sync_executor2 : test_sync_executor2
{
    template <typename F, typename... Ts>
    void post_at(
        hpx::chrono::steady_time_point const& abs_time, F&& f, Ts&&... ts) const
    {
        ++count_apply_at;
        hpx::this_thread::sleep_until(abs_time);
        hpx::invoke(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

namespace hpx::execution::experimental {
    template <>
    struct is_one_way_executor<test_sync_executor2> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<test_timed_sync_executor2> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_timed_executor<test_sync_executor1>({{6, 0, 0, 0}});
    test_timed_executor<test_sync_executor2>({{4, 2, 0, 0}});

    test_timed_executor<test_timed_sync_executor1>({{4, 0, 2, 0}});
    test_timed_executor<test_timed_sync_executor2>({{4, 0, 0, 2}});

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
