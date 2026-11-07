//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit tests for P2300 scheduler support in task_block and task_group

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/task_block.hpp>

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace ex = hpx::execution::experimental;

///////////////////////////////////////////////////////////////////////////////
// Test 1: Basic fork-join task spawning with P2300 scheduler
void test_define_task_block_scheduler_basic()
{
    bool parent_flag = false;
    std::atomic<bool> task1_flag{false};
    std::atomic<bool> task2_flag{false};
    std::atomic<bool> task3_flag{false};

    ex::thread_pool_scheduler sched{};

    hpx::experimental::define_task_block(sched, [&](auto& tr) {
        parent_flag = true;

        tr.run([&]() { task1_flag.store(true); });

        tr.run(sched, [&]() { task2_flag.store(true); });

        tr.run([&]() { task3_flag.store(true); });
    });

    HPX_TEST(parent_flag);
    HPX_TEST(task1_flag.load());
    HPX_TEST(task2_flag.load());
    HPX_TEST(task3_flag.load());
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: Nested define_task_block with P2300 scheduler
void test_define_task_block_scheduler_nested()
{
    std::string s("test");

    bool parent_flag = false;
    std::atomic<bool> task1_flag{false};
    std::atomic<bool> task2_flag{false};
    std::atomic<bool> task21_flag{false};

    ex::thread_pool_scheduler sched{};

    hpx::experimental::define_task_block(sched, [&](auto& tr) {
        parent_flag = true;

        tr.run([&]() {
            task1_flag.store(true);
            s += " passed";
        });

        tr.run([&]() {
            task2_flag.store(true);

            // Nested task block with scheduler
            hpx::experimental::define_task_block(sched, [&](auto& tr2) {
                tr2.run([&]() { task21_flag.store(true); });
            });
        });
    });

    HPX_TEST(parent_flag);
    HPX_TEST(task1_flag.load());
    HPX_TEST(task2_flag.load());
    HPX_TEST(task21_flag.load());
    HPX_TEST_EQ(s, std::string("test passed"));
}

///////////////////////////////////////////////////////////////////////////////
// Test 3: Explicit tr.wait() synchronization with P2300 scheduler
void test_define_task_block_scheduler_wait()
{
    std::atomic<int> counter{0};

    ex::thread_pool_scheduler sched{};

    hpx::experimental::define_task_block(sched, [&](auto& tr) {
        tr.run([&]() { counter.fetch_add(1); });

        tr.run([&]() { counter.fetch_add(1); });

        // Wait for the above tasks to complete
        tr.wait();

        // At this point, counter should be exactly 2
        HPX_TEST_EQ(counter.load(), 2);

        // Spawn more tasks after wait
        tr.run([&]() { counter.fetch_add(1); });
    });

    HPX_TEST_EQ(counter.load(), 3);
}

///////////////////////////////////////////////////////////////////////////////
// Test 4: Exception handling with P2300 scheduler
void test_define_task_block_scheduler_exceptions()
{
    ex::thread_pool_scheduler sched{};

    try
    {
        hpx::experimental::define_task_block(sched, [](auto& tr) {
            tr.run([]() { throw std::runtime_error("scheduler_error_1"); });

            tr.run([]() { throw std::runtime_error("scheduler_error_2"); });

            throw std::runtime_error("scheduler_error_parent");
        });

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        HPX_TEST_EQ(e.size(), 3u);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Test 5: define_task_block_restore_thread with P2300 scheduler
void test_define_task_block_restore_thread_scheduler()
{
    auto const original_thread = hpx::threads::get_self_id();

    std::atomic<bool> task_ran{false};

    ex::thread_pool_scheduler sched{};

    hpx::experimental::define_task_block_restore_thread(
        sched, [&](auto& tr) { tr.run([&]() { task_ran.store(true); }); });

    HPX_TEST(task_ran.load());

    // Verify we returned on the same HPX thread
    HPX_TEST_EQ(hpx::threads::get_self_id(), original_thread);
}

///////////////////////////////////////////////////////////////////////////////
// Test 6: task_group directly with P2300 scheduler
void test_task_group_scheduler_basic()
{
    std::atomic<int> sum{0};

    ex::thread_pool_scheduler sched{};

    hpx::experimental::task_group g;
    g.run(sched, [&]() { sum.fetch_add(1); });
    g.run(sched, [&]() { sum.fetch_add(2); });
    g.run(sched, [&]() { sum.fetch_add(3); });
    g.wait();

    HPX_TEST_EQ(sum.load(), 6);
}

///////////////////////////////////////////////////////////////////////////////
// Test 7: task_group scheduler with arguments
void test_task_group_scheduler_args()
{
    std::atomic<int> result{0};

    ex::thread_pool_scheduler sched{};

    hpx::experimental::task_group g;
    g.run(sched, [&](int a, int b) { result.store(a + b); }, 3, 4);
    g.wait();

    HPX_TEST_EQ(result.load(), 7);
}

///////////////////////////////////////////////////////////////////////////////
// Test 8: task_group scheduler exception propagation
void test_task_group_scheduler_exception()
{
    bool caught_exception = false;
    try
    {
        ex::thread_pool_scheduler sched{};

        hpx::experimental::task_group g;
        g.run(sched, []() { throw std::runtime_error("sched_test_error_1"); });
        g.run(sched, []() { throw std::runtime_error("sched_test_error_2"); });

        g.wait();
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& l)
    {
        caught_exception = true;
        HPX_TEST_EQ(l.size(), std::size_t(2));
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
// Test 9: Mixed executor + scheduler tasks on task_group
void test_task_group_mixed_executor_scheduler()
{
    std::atomic<int> sum{0};

    ex::thread_pool_scheduler sched{};
    hpx::execution::parallel_executor exec{};

    hpx::experimental::task_group g;
    g.run(sched, [&]() { sum.fetch_add(10); });
    g.run(exec, [&]() { sum.fetch_add(20); });
    g.run(sched, [&]() { sum.fetch_add(30); });
    g.wait();

    HPX_TEST_EQ(sum.load(), 60);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_define_task_block_scheduler_basic();
    test_define_task_block_scheduler_nested();
    test_define_task_block_scheduler_wait();
    test_define_task_block_scheduler_exceptions();
    test_define_task_block_restore_thread_scheduler();
    test_task_group_scheduler_basic();
    test_task_group_scheduler_args();
    test_task_group_scheduler_exception();
    test_task_group_mixed_executor_scheduler();

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
