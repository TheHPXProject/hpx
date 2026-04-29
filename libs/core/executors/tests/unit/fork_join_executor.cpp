//  Copyright (c)      2020 ETH Zurich
//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using hpx::execution::experimental::fork_join_executor;

static std::atomic<std::size_t> count1{0};
static std::atomic<std::size_t> count2{0};
static std::atomic<std::size_t> count3{0};
static std::atomic<std::size_t> count4{0};

///////////////////////////////////////////////////////////////////////////////
template <typename... ExecutorArgs>
void test_processing_mask(ExecutorArgs&&... args)
{
    std::cerr << "test_processing_mask\n";

    auto const& rp = hpx::resource::get_partitioner();
    auto const& expected_mask =
        rp.get_used_pus_mask(hpx::get_worker_thread_num());

    fork_join_executor exec{expected_mask, std::forward<ExecutorArgs>(args)...};
    auto const pus_mask =
        hpx::execution::experimental::get_processing_units_mask(exec);
    HPX_TEST(pus_mask == expected_mask);

    auto const cores_mask = hpx::execution::experimental::get_cores_mask(exec);
    HPX_TEST(cores_mask == expected_mask);

    auto const expected_pu_count = hpx::threads::count(pus_mask);
    HPX_TEST_EQ(expected_pu_count, hpx::threads::count(expected_mask));

    auto const pu_count =
        hpx::execution::experimental::processing_units_count(exec);
    HPX_TEST_EQ(pu_count, expected_pu_count);

    auto const first_core = hpx::execution::experimental::get_first_core(exec);
    HPX_TEST_EQ(first_core, hpx::threads::find_first(expected_mask));
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test(int, int passed_through)    //-V813
{
    ++count1;
    HPX_TEST_EQ(passed_through, 42);
}

template <typename... ExecutorArgs>
void test_bulk_sync(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_sync\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    hpx::parallel::execution::bulk_sync_execute(
        exec, hpx::bind(&bulk_test, _1, _2), v, 42);
    HPX_TEST_EQ(count1.load(), n);

    hpx::parallel::execution::bulk_sync_execute(exec, &bulk_test, v, 42);
    HPX_TEST_EQ(count1.load(), 2 * n);
}

template <typename... ExecutorArgs>
void test_bulk_async(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_async\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                      exec, hpx::bind(&bulk_test, _1, _2), v, 42))
        .get();
    HPX_TEST_EQ(count1.load(), n);

    hpx::when_all(
        hpx::parallel::execution::bulk_async_execute(exec, &bulk_test, v, 42))
        .get();
    HPX_TEST_EQ(count1.load(), 2 * n);
}

///////////////////////////////////////////////////////////////////////////////
int bulk_test_with_result(int val, int passed_through)    //-V813
{
    ++count1;
    HPX_TEST_EQ(passed_through, 42);
    return val;
}

template <typename... ExecutorArgs>
void test_bulk_sync_with_result(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_sync_with_result\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    std::vector<int> const result1 =
        hpx::parallel::execution::bulk_sync_execute(
            exec, hpx::bind(&bulk_test_with_result, _1, _2), v, 42);
    HPX_TEST_EQ(count1.load(), n);
    HPX_TEST(result1 == v);

    std::vector<int> const result2 =
        hpx::parallel::execution::bulk_sync_execute(
            exec, &bulk_test_with_result, v, 42);
    HPX_TEST_EQ(count1.load(), 2 * n);
    HPX_TEST(result2 == v);
}

template <typename... ExecutorArgs>
void test_bulk_async_with_result(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_async_with_result\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    std::vector<int> const result1 =
        hpx::parallel::execution::bulk_async_execute(
            exec, hpx::bind(&bulk_test_with_result, _1, _2), v, 42)
            .get();
    HPX_TEST_EQ(count1.load(), n);
    HPX_TEST(result1 == v);

    std::vector<int> const result2 =
        hpx::parallel::execution::bulk_async_execute(
            exec, &bulk_test_with_result, v, 42)
            .get();
    HPX_TEST_EQ(count1.load(), 2 * n);
    HPX_TEST(result2 == v);
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_exception(int, int passed_through)    //-V813
{
    HPX_TEST_EQ(passed_through, 42);
    throw std::runtime_error("test");
}

template <typename... ExecutorArgs>
void test_bulk_sync_exception(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_sync_exception\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    bool caught_exception = false;
    try
    {
        hpx::parallel::execution::bulk_sync_execute(
            exec, &bulk_test_exception, v, 42);

        HPX_TEST(false);
    }
    catch (std::runtime_error const& /*e*/)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename... ExecutorArgs>
void test_bulk_async_exception(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_async_exception\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    bool caught_exception = false;
    try
    {
        auto r = hpx::parallel::execution::bulk_async_execute(
            exec, &bulk_test_exception, v, 42);
        r.get();

        HPX_TEST(false);
    }
    catch (std::runtime_error const& /*e*/)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
int bulk_test_exception_with_result(int, int passed_through)    //-V813
{
    HPX_TEST_EQ(passed_through, 42);
    throw std::runtime_error("test");
}

template <typename... ExecutorArgs>
void test_bulk_sync_exception_with_result(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_sync_exception_with_result\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    bool caught_exception = false;
    try
    {
        [[maybe_unused]] std::vector<int> const result =
            hpx::parallel::execution::bulk_sync_execute(
                exec, &bulk_test_exception_with_result, v, 42);

        HPX_TEST(false);
    }
    catch (std::runtime_error const& /*e*/)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename... ExecutorArgs>
void test_bulk_async_exception_with_result(ExecutorArgs&&... args)
{
    std::cerr << "test_bulk_async_exception_with_result\n";

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    std::iota(std::begin(v), std::end(v), std::rand());

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
    bool caught_exception = false;
    try
    {
        auto r = hpx::parallel::execution::bulk_async_execute(
            exec, &bulk_test_exception_with_result, v, 42);
        [[maybe_unused]] std::vector<int> const result = r.get();

        HPX_TEST(false);
    }
    catch (std::runtime_error const& /*e*/)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
template <typename... ExecutorArgs>
void test_invoke_sync_homogeneous(ExecutorArgs&&... args)
{
    std::cerr << "test_invoke_sync_homogeneous\n";

    auto f1 = [] { ++count1; };

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

    count1 = 0;
    hpx::parallel::execution::sync_invoke(exec, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(1));

    count1 = 0;
    hpx::parallel::execution::sync_invoke(exec, f1, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(2));

    count1 = 0;
    hpx::parallel::execution::sync_invoke(exec, f1, f1, f1, f1, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(5));

    count1 = 0;
    hpx::parallel::execution::sync_invoke(
        exec, f1, f1, f1, f1, f1, f1, f1, f1, f1, f1, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(11));
}

template <typename... ExecutorArgs>
void test_invoke_sync(ExecutorArgs&&... args)
{
    std::cerr << "test_invoke_sync\n";

    auto f1 = [] { ++count1; };
    auto f2 = [] { ++count2; };
    auto f3 = [] { ++count3; };
    auto f4 = [] { ++count4; };

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

    count1 = 0;
    hpx::parallel::execution::sync_invoke(exec, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(1));

    count1 = 0;
    count2 = 0;
    hpx::parallel::execution::sync_invoke(exec, f1, f2);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(count2.load(), static_cast<std::size_t>(1));

    count1 = 0;
    count2 = 0;
    count3 = 0;
    count4 = 0;
    hpx::parallel::execution::sync_invoke(exec, f1, f2, f3, f4, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(count2.load(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(count3.load(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(count4.load(), static_cast<std::size_t>(1));

    count1 = 0;
    count2 = 0;
    count3 = 0;
    count4 = 0;
    hpx::parallel::execution::sync_invoke(
        exec, f1, f2, f3, f4, f1, f4, f1, f2, f3, f4, f1);
    HPX_TEST_EQ(count1.load(), static_cast<std::size_t>(4));
    HPX_TEST_EQ(count2.load(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(count3.load(), static_cast<std::size_t>(2));
    HPX_TEST_EQ(count4.load(), static_cast<std::size_t>(3));
}

template <typename... ExecutorArgs>
void test_invoke_sync_homogeneous_exception(ExecutorArgs&&... args)
{
    std::cerr << "test_invoke_sync_homogeneous_exception\n";

    auto f1 = [] { throw std::runtime_error("test"); };

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

    bool caught_exception = false;
    try
    {
        hpx::parallel::execution::sync_invoke(exec, f1, f1, f1);
        HPX_TEST(false);
    }
    catch (std::runtime_error const& /*e*/)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename... ExecutorArgs>
void test_invoke_sync_exception(ExecutorArgs&&... args)
{
    std::cerr << "test_invoke_sync_exception\n";

    auto f1 = [] {};
    auto f2 = [] { throw std::runtime_error("test"); };

    fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

    bool caught_exception = false;
    try
    {
        hpx::parallel::execution::sync_invoke(exec, f1, f2);
        HPX_TEST(false);
    }
    catch (std::runtime_error const& /*e*/)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

void static_check_executor()
{
    using namespace hpx::traits;

    static_assert(is_bulk_one_way_executor_v<fork_join_executor>,
        "is_bulk_one_way_executor_v<fork_join_executor>");
    static_assert(is_bulk_two_way_executor_v<fork_join_executor>,
        "is_bulk_two_way_executor_v<fork_join_executor>");
}

template <typename... ExecutorArgs>
void test_executor(hpx::threads::thread_priority priority,
    hpx::threads::thread_stacksize stacksize,
    fork_join_executor::loop_schedule schedule)
{
    std::cerr << "testing fork_join_executor with priority = " << priority
              << ", stacksize = " << stacksize << ", schedule = " << schedule
              << "\n";
    test_bulk_sync(priority, stacksize, schedule);
    test_bulk_async(priority, stacksize, schedule);
    test_bulk_sync_exception(priority, stacksize, schedule);
    test_bulk_async_exception(priority, stacksize, schedule);

    test_bulk_sync_with_result(priority, stacksize, schedule);
    test_bulk_async_with_result(priority, stacksize, schedule);
    test_bulk_sync_exception_with_result(priority, stacksize, schedule);
    test_bulk_async_exception_with_result(priority, stacksize, schedule);

    test_invoke_sync_homogeneous(priority, stacksize, schedule);
    test_invoke_sync(priority, stacksize, schedule);
    test_invoke_sync_homogeneous_exception(priority, stacksize, schedule);
    test_invoke_sync_exception(priority, stacksize, schedule);

    test_processing_mask(priority, stacksize, schedule);
}

void test_fork_join_static_large_range()
{
    // Regression test for #6922
    // Strategy- Sums indices straddling UINT32_MAX and compare
    // against closed-form expected arithmetic sum.

    fork_join_executor exec;

    auto const uint32_max =
        static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)());

    std::size_t const low = uint32_max - 500;
    std::size_t const high = uint32_max + 500;

    // sum of integers [lo, hi) = n*(lo + hi - 1)/2
    std::size_t const n = high - low;
    std::size_t const expected_sum = n * (low + high - 1) / 2;

    std::atomic<std::size_t> sum{0};

    hpx::for_each(hpx::execution::par.on(exec),
        hpx::util::counting_iterator<std::size_t>(low),
        hpx::util::counting_iterator<std::size_t>(high),
        [&sum](std::size_t i) { sum.fetch_add(i, std::memory_order_relaxed); });

    HPX_TEST_EQ(sum.load(), expected_sum);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    static_check_executor();

    // Call regression test for #6922
    test_fork_join_static_large_range();

    // Using thread_priority::low hangs for unknown reasons.
    for (auto const priority : {
             // hpx::threads::thread_priority::low,
             hpx::threads::thread_priority::normal,
             hpx::threads::thread_priority::high,
             hpx::threads::thread_priority::bound,
         })
    {
        for (auto const stacksize : {
                 hpx::threads::thread_stacksize::nostack,
                 hpx::threads::thread_stacksize::small_,
             })
        {
            for (auto const schedule : {
                     fork_join_executor::loop_schedule::static_,
                     fork_join_executor::loop_schedule::dynamic,
                 })
            {
                {
                    test_executor(priority, stacksize, schedule);
                }
            }
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
