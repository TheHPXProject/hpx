//  Copyright (c) 2025 Harith Reddy
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/experimental/run_on_all.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

void test_sync_exception()
{
    using namespace hpx::experimental;
    std::uint32_t n = 0;
    bool caught = false;
    std::atomic<int> count{0};
    try
    {
        run_on_all(
            hpx::execution::par, reduction_plus(n), [&count](std::uint32_t&) {
                if (count++ == 0)
                    throw std::runtime_error("test sync");
            });
    }
    catch (std::exception const&)
    {
        caught = true;
    }
    HPX_TEST(caught);
}

void test_async_exception()
{
    using namespace hpx::experimental;
    std::uint32_t n = 0;
    bool caught = false;
    std::atomic<int> count{0};
    try
    {
        auto f = run_on_all(hpx::execution::par(hpx::execution::task),
            reduction_plus(n), [&count](std::uint32_t&) {
                if (count++ == 0)
                    throw std::runtime_error("test async");
            });
        f.get();
    }
    catch (std::exception const&)
    {
        caught = true;
    }
    HPX_TEST(caught);
}

void test_scheduler_exception()
{
    using namespace hpx::experimental;
    std::uint32_t n = 0;
    bool caught = false;
    std::atomic<int> count{0};
    try
    {
        auto sched = hpx::execution::experimental::thread_pool_scheduler{};
        auto s = run_on_all(sched, reduction_plus(n), [&count](std::uint32_t&) {
            if (count++ == 0)
                throw std::runtime_error("test scheduler");
        });
        hpx::this_thread::experimental::sync_wait(std::move(s));
    }
    catch (std::exception const&)
    {
        caught = true;
    }
    HPX_TEST(caught);
}

int main()
{
    using namespace hpx::experimental;

    // Test basic functionality with reduction
    {
        std::uint32_t n = 0;
        run_on_all(
            reduction_plus(n), [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
    }

    // Test with specific number of tasks
    {
        auto policy = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, 2);

        std::uint32_t n = 0;
        run_on_all(policy, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(2));
    }

    {
        auto policy = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, 2);

        std::uint32_t n = 0;
        auto f = run_on_all(policy(hpx::execution::task), reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        f.get();
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(2));
    }

    // Test with sequential execution policy
    {
        std::uint32_t n = 0;
        run_on_all(hpx::execution::seq, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(1));
    }

    // Test with parallel execution policy
    {
        std::uint32_t n = 0;
        run_on_all(hpx::execution::par, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
    }

    {
        std::uint32_t n = 0;
        auto f = run_on_all(hpx::execution::par(hpx::execution::task),
            reduction_plus(n), [](std::uint32_t& local_n) { ++local_n; });
        f.get();
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
    }

    // Test with parallel unsequenced execution policy
    {
        std::uint32_t n = 0;
        run_on_all(hpx::execution::par_unseq, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
    }

    // Test with multiple arguments
    {
        std::uint32_t n = 0;
        std::uint32_t m = 0;
        run_on_all(reduction_plus(n), reduction_plus(m),
            [](std::uint32_t& local_n, std::uint32_t& local_m) {
                ++local_n;
                local_m += 2;
            });
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
        HPX_TEST_EQ(
            m, static_cast<std::uint32_t>(2 * hpx::get_num_worker_threads()));
    }

    {
        std::uint32_t n = 0;
        std::uint32_t m = 0;
        run_on_all(hpx::execution::par, reduction_plus(n), reduction_plus(m),
            [](std::uint32_t& local_n, std::uint32_t& local_m) {
                ++local_n;
                local_m += 2;
            });
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
        HPX_TEST_EQ(
            m, static_cast<std::uint32_t>(2 * hpx::get_num_worker_threads()));
    }

    {
        std::uint32_t n = 0;
        std::uint32_t m = 0;
        auto f = run_on_all(hpx::execution::par(hpx::execution::task),
            reduction_plus(n), reduction_plus(m),
            [](std::uint32_t& local_n, std::uint32_t& local_m) {
                ++local_n;
                local_m += 2;
            });
        f.get();
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
        HPX_TEST_EQ(
            m, static_cast<std::uint32_t>(2 * hpx::get_num_worker_threads()));
    }

    // Test with atomic operations
    {
        std::atomic<std::uint32_t> n(0);
        run_on_all([&] { ++n; });
        HPX_TEST_EQ(n.load(),
            static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
    }

    // Test with P2300 scheduler
    {
        auto sched = hpx::execution::experimental::thread_pool_scheduler{};
        std::uint32_t n = 0;
        auto s = run_on_all(sched, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        hpx::this_thread::experimental::sync_wait(std::move(s));
        HPX_TEST_EQ(
            n, static_cast<std::uint32_t>(hpx::get_num_worker_threads()));
    }

    // Test with different number of tasks
    {
        auto policy = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, 1);

        std::uint32_t n = 0;
        run_on_all(policy, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(1));
    }

    {
        auto policy = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, 4);

        std::uint32_t n = 0;
        run_on_all(policy, reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(4));
    }

    test_sync_exception();
    test_async_exception();
    test_scheduler_exception();

    return hpx::util::report_errors();
}
