//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The tests of fork_join_executor, run with suspending_fork_join_executor,
// and tests of what is particular to suspending_fork_join_executor (lazy
// initialization, callers outside the executor's PUs).

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using hpx::execution::experimental::suspending_fork_join_executor;

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

    suspending_fork_join_executor exec{
        expected_mask, std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};
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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

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

    suspending_fork_join_executor exec{std::forward<ExecutorArgs>(args)...};

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

    static_assert(is_bulk_one_way_executor_v<suspending_fork_join_executor>,
        "is_bulk_one_way_executor_v<suspending_fork_join_executor>");
    static_assert(is_bulk_two_way_executor_v<suspending_fork_join_executor>,
        "is_bulk_two_way_executor_v<suspending_fork_join_executor>");
    static_assert(is_one_way_executor_v<suspending_fork_join_executor>,
        "is_one_way_executor_v<suspending_fork_join_executor>");
    static_assert(is_two_way_executor_v<suspending_fork_join_executor>,
        "is_two_way_executor_v<suspending_fork_join_executor>");
}

// async_execute runs the task as a region of one index and returns a ready
// future
void test_async_execute()
{
    suspending_fork_join_executor const exec;

    hpx::future<int> f = hpx::parallel::execution::async_execute(
        exec, [](int i) { return i + 1; }, 41);
    HPX_TEST(f.is_ready());
    HPX_TEST_EQ(f.get(), 42);

    std::atomic<int> calls{0};
    hpx::future<void> g =
        hpx::parallel::execution::async_execute(exec, [&] { ++calls; });
    HPX_TEST(g.is_ready());
    g.get();
    HPX_TEST_EQ(calls.load(), 1);

    hpx::future<void> h = hpx::parallel::execution::async_execute(
        exec, [] { throw std::runtime_error("test"); });
    HPX_TEST(h.is_ready());
    bool caught_exception = false;
    try
    {
        h.get();
    }
    catch (std::runtime_error const&)
    {
        caught_exception = true;
    }
    HPX_TEST(caught_exception);
}

// sync_execute runs the task on the calling thread, and a loop that gets one
// chunk takes that path instead of running a region
void test_sync_execute()
{
    suspending_fork_join_executor const exec;

    auto const caller = std::this_thread::get_id();
    int const result = hpx::parallel::execution::sync_execute(
        exec,
        [&](int i) {
            HPX_TEST(std::this_thread::get_id() == caller);
            return i + 1;
        },
        41);
    HPX_TEST_EQ(result, 42);

    std::vector<int> v(10, 1);
    std::atomic<std::size_t> others{0};
    hpx::for_each(hpx::execution::par.on(exec).with(
                      hpx::execution::experimental::static_chunk_size(10),
                      hpx::execution::experimental::num_cores(1)),
        v.begin(), v.end(), [&](int& x) {
            if (std::this_thread::get_id() != caller)
            {
                ++others;
            }
            ++x;
        });
    HPX_TEST_EQ(others.load(), std::size_t(0));
    HPX_TEST(std::all_of(v.begin(), v.end(), [](int x) { return x == 2; }));
}

void test_executor(hpx::threads::thread_priority priority,
    hpx::threads::thread_stacksize stacksize,
    suspending_fork_join_executor::loop_schedule schedule,
    std::chrono::nanoseconds spin_duration)
{
    std::cerr << "testing suspending_fork_join_executor with priority = "
              << priority << ", stacksize = " << stacksize
              << ", schedule = " << schedule
              << ", spin duration = " << spin_duration.count() << " ns\n";

    auto const args =
        std::make_tuple(priority, stacksize, schedule, spin_duration);
    auto run = [&](auto f) { std::apply(f, args); };

    run([](auto... a) { test_bulk_sync(a...); });
    run([](auto... a) { test_bulk_async(a...); });
    run([](auto... a) { test_bulk_sync_exception(a...); });
    run([](auto... a) { test_bulk_async_exception(a...); });

    run([](auto... a) { test_bulk_sync_with_result(a...); });
    run([](auto... a) { test_bulk_async_with_result(a...); });
    run([](auto... a) { test_bulk_sync_exception_with_result(a...); });
    run([](auto... a) { test_bulk_async_exception_with_result(a...); });

    run([](auto... a) { test_invoke_sync_homogeneous(a...); });
    run([](auto... a) { test_invoke_sync(a...); });
    run([](auto... a) { test_invoke_sync_homogeneous_exception(a...); });
    run([](auto... a) { test_invoke_sync_exception(a...); });

    run([](auto... a) { test_processing_mask(a...); });
}

void test_static_large_range()
{
    // Regression test for #6922
    // Strategy- Sums indices straddling UINT32_MAX and compare
    // against closed-form expected arithmetic sum.

    suspending_fork_join_executor exec;

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
void test_get_scheduler()
{
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    std::cerr << "test_get_scheduler\n";

    // Test 1: get_scheduler returns a valid scheduler
    suspending_fork_join_executor exec{};
    auto sched = ex::get_scheduler(exec);

    // Test 2: scheduler can be used to schedule work via then + sync_wait
    auto result = hpx::get<0>(
        *(tt::sync_wait(ex::then(ex::schedule(sched), []() { return 42; }))));
    HPX_TEST_EQ(result, 42);

    // Test 3: scheduled work runs on an HPX worker thread
    hpx::thread::id scheduled_thread_id{};
    tt::sync_wait(ex::then(ex::schedule(sched),
        [&]() { scheduled_thread_id = hpx::this_thread::get_id(); }));
    HPX_TEST(scheduled_thread_id != hpx::thread::id{});
}

///////////////////////////////////////////////////////////////////////////////
// The executor is constexpr default constructible, its worker threads are
// created on first use.
static_assert(
    std::is_nothrow_default_constructible_v<suspending_fork_join_executor>);

constinit suspending_fork_join_executor global_exec{};

void test_lazy_initialization()
{
    std::cerr << "test_lazy_initialization\n";

    // queries work before the worker threads exist
    std::size_t const pu_count =
        hpx::execution::experimental::processing_units_count(global_exec);
    HPX_TEST_EQ(pu_count, hpx::get_num_worker_threads());

    count1 = 0;
    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    hpx::parallel::execution::bulk_sync_execute(global_exec, &bulk_test, v, 42);
    HPX_TEST_EQ(count1.load(), n);
    HPX_TEST_EQ(
        hpx::execution::experimental::processing_units_count(global_exec),
        pu_count);

    // copies share the worker threads
    suspending_fork_join_executor const copy = global_exec;
    HPX_TEST(copy == global_exec);
    hpx::parallel::execution::bulk_sync_execute(copy, &bulk_test, v, 42);
    HPX_TEST_EQ(count1.load(), 2 * n);

    // an executor that is copied into an execution policy before its first
    // use creates its worker threads when it is copied
    suspending_fork_join_executor fresh{};
    hpx::for_each(hpx::execution::par.on(fresh), v.begin(), v.end(),
        [](int) { ++count1; });
    HPX_TEST_EQ(count1.load(), 3 * n);

    // release the worker threads of the global executor while the runtime
    // is still running
    global_exec = suspending_fork_join_executor{};
}

///////////////////////////////////////////////////////////////////////////////
hpx::threads::mask_type mask_of_workers(
    std::size_t const first, std::size_t const last, std::size_t const skip)
{
    auto const& rp = hpx::resource::get_partitioner();

    hpx::threads::mask_type mask = hpx::threads::mask_type();
    hpx::threads::resize(mask, hpx::threads::hardware_concurrency());
    for (std::size_t i = first; i != last; ++i)
    {
        if (i != skip)
        {
            hpx::threads::set(mask, rp.get_pu_num(i));
        }
    }
    return mask;
}

// Run regions and check that they don't run anything on the calling thread.
void check_caller_not_participating(suspending_fork_join_executor const& exec,
    std::size_t const expected_pu_count)
{
    HPX_TEST_EQ(hpx::execution::experimental::processing_units_count(exec),
        expected_pu_count);

    auto const caller = hpx::this_thread::get_id();
    std::atomic<std::size_t> on_caller{0};
    std::atomic<std::size_t> count{0};

    constexpr std::size_t n = 107;
    std::vector<int> v(n);
    for (int i = 0; i != 10; ++i)
    {
        hpx::parallel::execution::bulk_sync_execute(
            exec,
            [&](int) {
                ++count;
                if (hpx::this_thread::get_id() == caller)
                    ++on_caller;
            },
            v);
    }
    HPX_TEST_EQ(count.load(), 10 * n);
    HPX_TEST_EQ(on_caller.load(), static_cast<std::size_t>(0));
}

// The PU of the calling thread is not in the executor's PU mask.
void test_caller_not_in_mask(
    suspending_fork_join_executor::loop_schedule const schedule)
{
    std::cerr << "test_caller_not_in_mask: " << schedule << "\n";

    std::size_t const num_threads = hpx::get_num_worker_threads();
    if (num_threads < 2)
        return;

    suspending_fork_join_executor const exec(
        mask_of_workers(0, num_threads, hpx::get_worker_thread_num()),
        hpx::threads::thread_priority::bound,
        hpx::threads::thread_stacksize::small_, schedule);

    check_caller_not_participating(exec, num_threads - 1);
}

// The calling thread runs on a worker thread whose number is not smaller
// than the number of PUs of the executor (see #8230).
void test_caller_on_higher_worker(
    suspending_fork_join_executor::loop_schedule const schedule)
{
    std::cerr << "test_caller_on_higher_worker: " << schedule << "\n";

    std::size_t const num_threads = hpx::get_num_worker_threads();
    if (num_threads < 3)
        return;

    std::size_t const caller_worker = num_threads - 1;
    hpx::execution::parallel_executor const pexec(
        hpx::threads::thread_priority::bound,
        hpx::threads::thread_stacksize::default_,
        hpx::threads::thread_schedule_hint(
            static_cast<std::int16_t>(caller_worker)));

    hpx::async(pexec, [&] {
        HPX_TEST_EQ(hpx::get_worker_thread_num(), caller_worker);

        suspending_fork_join_executor const exec(
            mask_of_workers(0, 2, static_cast<std::size_t>(-1)),
            hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::small_, schedule);

        check_caller_not_participating(exec, 2);
    }).get();
}

// The calling thread is not an HPX thread.
void test_external_caller(
    suspending_fork_join_executor::loop_schedule const schedule)
{
    std::cerr << "test_external_caller: " << schedule << "\n";

    std::atomic<bool> done{false};
    std::atomic<std::size_t> count{0};
    constexpr std::size_t n = 107;

    std::thread t([&] {
        {
            suspending_fork_join_executor const exec(
                hpx::threads::thread_priority::bound,
                hpx::threads::thread_stacksize::small_, schedule);

            std::vector<int> v(n);
            for (int i = 0; i != 10; ++i)
            {
                hpx::parallel::execution::bulk_sync_execute(
                    exec, [&](int) { ++count; }, v);
            }
        }

        // the executor waits in its destructor for its worker threads
        done = true;
    });

    // The core of this thread has a worker thread of the executor as well,
    // so this thread must not block it.
    while (!done)
    {
        hpx::this_thread::yield();
    }
    t.join();

    HPX_TEST_EQ(count.load(), 10 * n);
}

///////////////////////////////////////////////////////////////////////////////
void test_nested_use()
{
    std::cerr << "test_nested_use\n";

    suspending_fork_join_executor const exec;
    std::atomic<std::size_t> caught{0};

    constexpr std::size_t n = 16;
    std::vector<int> v(n);
    hpx::parallel::execution::bulk_sync_execute(
        exec,
        [&](int) {
            try
            {
                hpx::parallel::execution::bulk_sync_execute(
                    exec, [](int) {}, std::vector<int>(4));
            }
            catch (hpx::exception const& e)
            {
                if (e.get_error() == hpx::error::bad_request)
                    ++caught;
            }
        },
        v);
    HPX_TEST_EQ(caught.load(), n);

    // the executor can still be used
    count1 = 0;
    hpx::parallel::execution::bulk_sync_execute(exec, &bulk_test, v, 42);
    HPX_TEST_EQ(count1.load(), n);
}

void test_nostack_rejected()
{
    std::cerr << "test_nostack_rejected\n";

    suspending_fork_join_executor const exec(
        hpx::threads::thread_priority::bound,
        hpx::threads::thread_stacksize::nostack);

    bool caught = false;
    try
    {
        hpx::parallel::execution::bulk_sync_execute(
            exec, [](int) {}, std::vector<int>(4));
    }
    catch (hpx::exception const& e)
    {
        caught = e.get_error() == hpx::error::bad_parameter;
    }
    HPX_TEST(caught);
}

// Work bound to each of the cores runs while the executor is idle.
void test_idle_cores_run_other_work()
{
    std::cerr << "test_idle_cores_run_other_work\n";

    suspending_fork_join_executor const exec;
    std::vector<int> v(64);
    hpx::parallel::execution::bulk_sync_execute(exec, [](int) {}, v);

    std::size_t const num_threads = hpx::get_num_worker_threads();
    std::vector<hpx::future<std::size_t>> futures;
    futures.reserve(num_threads);
    for (std::size_t i = 0; i != num_threads; ++i)
    {
        hpx::execution::parallel_executor const pexec(
            hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(static_cast<std::int16_t>(i)));
        futures.push_back(
            hpx::async(pexec, [] { return hpx::get_worker_thread_num(); }));
    }
    for (std::size_t i = 0; i != num_threads; ++i)
    {
        HPX_TEST_EQ(futures[i].get(), i);
    }

    // and the executor still works afterwards
    count1 = 0;
    hpx::parallel::execution::bulk_sync_execute(exec, &bulk_test, v, 42);
    HPX_TEST_EQ(count1.load(), v.size());
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    static_check_executor();

    test_lazy_initialization();

    // P2300 get_scheduler bridge test
    test_get_scheduler();
    test_sync_execute();
    test_async_execute();

    test_static_large_range();

    for (auto const schedule : {
             suspending_fork_join_executor::loop_schedule::static_,
             suspending_fork_join_executor::loop_schedule::dynamic,
         })
    {
        test_caller_not_in_mask(schedule);
        test_caller_on_higher_worker(schedule);
        test_external_caller(schedule);
    }

    test_nested_use();
    test_nostack_rejected();
    test_idle_cores_run_other_work();

    for (auto const priority : {
             hpx::threads::thread_priority::low,
             hpx::threads::thread_priority::normal,
             hpx::threads::thread_priority::high,
             hpx::threads::thread_priority::bound,
         })
    {
        for (auto const schedule : {
                 suspending_fork_join_executor::loop_schedule::static_,
                 suspending_fork_join_executor::loop_schedule::dynamic,
             })
        {
            // a spin duration of zero suspends on every wait
            for (auto const spin_duration : {
                     std::chrono::nanoseconds(0),
                     suspending_fork_join_executor::default_spin_duration,
                 })
            {
                test_executor(priority, hpx::threads::thread_stacksize::small_,
                    schedule, spin_duration);
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
