//  Copyright (c) 2026 Shivansh Singh
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/experimental/run_on_all.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/task_block.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;
using hpx::experimental::define_task_block;
using hpx::experimental::define_task_block_restore_thread;
using hpx::experimental::task_group;

///////////////////////////////////////////////////////////////////////////////
// 1. Basic task spawning via P2300 scheduler (task_group::run(sched, f) + wait_as_sender())
void test_task_group_scheduler_basic()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> count{0};
    g.run(sched, [&count] { ++count; });
    g.run(sched, [&count] { ++count; });

    auto sender = g.wait_as_sender();
    tt::sync_wait(HPX_MOVE(sender));

    HPX_TEST_EQ(count.load(), 2);
}

///////////////////////////////////////////////////////////////////////////////
// 2. Multiple nested/recursive tasks using task_group with scheduler
int fib_sched(ex::thread_pool_scheduler sched, int n)
{
    if (n < 2)
    {
        return n;
    }

    int x = 0;
    int y = 0;

    task_group g;
    g.run(sched, [&x, sched, n] { x = fib_sched(sched, n - 1); });
    g.run(sched, [&y, sched, n] { y = fib_sched(sched, n - 2); });
    tt::sync_wait(g.wait_as_sender());

    return x + y;
}

void test_task_group_scheduler_fib()
{
    ex::thread_pool_scheduler sched{};
    HPX_TEST_EQ(fib_sched(sched, 10), 55);
}

///////////////////////////////////////////////////////////////////////////////
// 3. Reuse of task_group across multiple run() / wait_as_sender() cycles
void test_task_group_scheduler_reuse()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    int x = 0;
    int y = 0;

    g.run(sched, [&x] { x = 42; });
    tt::sync_wait(g.wait_as_sender());
    HPX_TEST_EQ(x, 42);

    g.run(sched, [&y] { y = 84; });
    tt::sync_wait(g.wait_as_sender());
    HPX_TEST_EQ(y, 84);
}

///////////////////////////////////////////////////////////////////////////////
// 4. Exception propagation through sender graph to wait()
void test_task_group_scheduler_exception()
{
    ex::thread_pool_scheduler sched{};
    bool caught_exception = false;

    try
    {
        task_group g;
        g.run(sched, [] { throw std::runtime_error("error1"); });
        g.run(sched, [] { throw std::runtime_error("error2"); });
        tt::sync_wait(g.wait_as_sender());
        g.wait();
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        HPX_TEST_EQ(e.size(), 2u);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
// 5. Move-only callable arguments with task_group::run(sched, f, args...)
struct move_only_payload
{
    int val;
    explicit move_only_payload(int v)
      : val(v)
    {
    }
    move_only_payload(move_only_payload&&) = default;
    move_only_payload& operator=(move_only_payload&&) = default;
    move_only_payload(move_only_payload const&) = delete;
    move_only_payload& operator=(move_only_payload const&) = delete;
};

void test_task_group_scheduler_move_only()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> result{0};
    g.run(
        sched, [&result](move_only_payload p) { result = p.val; },
        move_only_payload{123});

    tt::sync_wait(g.wait_as_sender());
    HPX_TEST_EQ(result.load(), 123);
}

///////////////////////////////////////////////////////////////////////////////
// 6. wait_as_sender() composability with sender algorithms (e.g. ex::then)
void test_task_group_scheduler_composability()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> val{10};
    g.run(sched, [&val] { val += 5; });

    auto sender =
        g.wait_as_sender() | ex::then([&val] { return val.load() * 2; });

    auto res = tt::sync_wait(HPX_MOVE(sender));
    HPX_TEST(res.has_value());
    HPX_TEST_EQ(hpx::get<0>(*res), 30);
}

///////////////////////////////////////////////////////////////////////////////
// 7. define_task_block(Scheduler&&, F&&) basic functionality
void test_define_task_block_scheduler()
{
    ex::thread_pool_scheduler sched{};
    std::atomic<int> count{0};

    // 2-argument callable: f(tr, s)
    define_task_block(sched, [&](auto& tr, auto& s) {
        tr.run(s, [&count] { ++count; });
        tr.run(s, [&count] { ++count; });
    });
    HPX_TEST_EQ(count.load(), 2);

    // 1-argument callable: f(tr)
    define_task_block(
        sched, [&](auto& tr) { tr.run(sched, [&count] { ++count; }); });
    HPX_TEST_EQ(count.load(), 3);
}

///////////////////////////////////////////////////////////////////////////////
// 8. define_task_block(Scheduler&&, F&&) exception handling
void test_define_task_block_scheduler_exception()
{
    ex::thread_pool_scheduler sched{};
    bool caught_exception = false;

    try
    {
        define_task_block(sched, [](auto& tr, auto& s) {
            tr.run(s, [] { throw std::runtime_error("tb_error"); });
        });
    }
    catch (hpx::exception_list const& el)
    {
        caught_exception = true;
        HPX_TEST_EQ(el.size(), 1u);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
// 9. define_task_block_restore_thread(Scheduler&&, F&&)
void test_define_task_block_restore_thread_scheduler()
{
    ex::thread_pool_scheduler sched{};
    auto original_id = hpx::this_thread::get_id();

    std::atomic<int> count{0};
    define_task_block_restore_thread(
        sched, [&](auto& tr, auto& s) { tr.run(s, [&count] { ++count; }); });

    HPX_TEST_EQ(count.load(), 1);
    HPX_TEST_EQ(hpx::this_thread::get_id(), original_id);
}

///////////////////////////////////////////////////////////////////////////////
// 10. Mixed legacy executor + P2300 scheduler usage on same task_group
void test_task_group_mixed_executor_scheduler()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> val1{0};
    std::atomic<int> val2{0};

    g.run(sched, [&val1] { val1 = 10; });
    g.run(hpx::execution::parallel_executor{}, [&val2] { val2 = 20; });

    // Mixing legacy executor tasks with wait() is fully supported and joins both
    g.wait();

    HPX_TEST_EQ(val1.load(), 10);
    HPX_TEST_EQ(val2.load(), 20);

    // Mixing legacy executor tasks with wait_as_sender() is explicitly rejected
    {
        task_group g_mixed;
        std::atomic<bool> flag{false};
        g_mixed.run(sched, [] {});
        g_mixed.run(hpx::execution::parallel_executor{}, [&flag] {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
            flag = true;
        });

        bool caught = false;
        try
        {
            tt::sync_wait(g_mixed.wait_as_sender());
        }
        catch (hpx::exception const& e)
        {
            caught = (e.get_error() == hpx::error::invalid_status);
        }
        HPX_TEST(caught);
        g_mixed.wait();
        HPX_TEST(flag.load());
    }
}

///////////////////////////////////////////////////////////////////////////////
// 11. run_on_all(Scheduler&&, F&&, Reductions&&...)
void test_run_on_all_scheduler()
{
    ex::thread_pool_scheduler sched{};
    auto cores = hpx::execution::experimental::processing_units_count(sched);

    // Single reduction
    {
        std::uint32_t n = 0;
        hpx::experimental::run_on_all(sched,
            hpx::experimental::reduction_plus(n),
            [](std::uint32_t& local_n) { ++local_n; });
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(cores));
    }

    // Multiple reductions
    {
        std::uint32_t n = 0;
        std::uint32_t m = 0;
        hpx::experimental::run_on_all(sched,
            hpx::experimental::reduction_plus(n),
            hpx::experimental::reduction_plus(m),
            [](std::uint32_t& local_n, std::uint32_t& local_m) {
                ++local_n;
                local_m += 2;
            });
        HPX_TEST_EQ(n, static_cast<std::uint32_t>(cores));
        HPX_TEST_EQ(m, static_cast<std::uint32_t>(2 * cores));
    }
}

///////////////////////////////////////////////////////////////////////////////
// 12. run_on_all worker mapping: verify each worker participates exactly once
void test_run_on_all_scheduler_workers()
{
    ex::thread_pool_scheduler sched{};
    auto const cores =
        hpx::execution::experimental::processing_units_count(sched);

    std::vector<std::atomic<int>> worker_hits(cores);
    for (auto& hit : worker_hits)
    {
        hit.store(0);
    }

    hpx::experimental::run_on_all(sched, [&worker_hits, cores]() {
        std::size_t worker_id = hpx::get_worker_thread_num();
        if (worker_id < cores)
        {
            ++worker_hits[worker_id];
        }
    });

    for (std::size_t i = 0; i < cores; ++i)
    {
        HPX_TEST_EQ(worker_hits[i].load(), 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// 13. task_group: legacy wait() with P2300 scheduler-run tasks
void test_task_group_scheduler_legacy_wait()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> count{0};
    g.run(sched, [&count] { ++count; });
    g.run(sched, [&count] { ++count; });

    // Call legacy wait() directly to join senders via sync_wait internally
    g.wait();

    HPX_TEST_EQ(count.load(), 2);
}

///////////////////////////////////////////////////////////////////////////////
// 14. task_group: recursive/dynamic task spawning on the same task_group
void test_task_group_recursive_spawning()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> count{0};

    // Root task dynamically spawns child tasks on the same task_group g
    g.run(sched, [&g, sched, &count] {
        ++count;
        g.run(sched, [&g, sched, &count] {
            ++count;
            g.run(sched, [&count] { ++count; });
        });
    });

    auto sender = g.wait_as_sender();
    tt::sync_wait(HPX_MOVE(sender));

    HPX_TEST_EQ(count.load(), 3);
}

void test_task_group_recursive_spawning_legacy_wait()
{
    ex::thread_pool_scheduler sched{};
    task_group g;

    std::atomic<int> count{0};

    // Root task dynamically spawns child tasks on the same task_group g
    g.run(sched, [&g, sched, &count] {
        ++count;
        g.run(sched, [&g, sched, &count] {
            ++count;
            g.run(sched, [&count] { ++count; });
        });
    });

    g.wait();

    HPX_TEST_EQ(count.load(), 3);
}

///////////////////////////////////////////////////////////////////////////////
// 15. task_group: shared lifetime safety when task_group goes out of scope
void test_task_group_lifetime_safety()
{
    ex::thread_pool_scheduler sched{};
    std::atomic<int> count{0};

    auto sender = [&]() {
        task_group g;
        g.run(sched, [&count] { ++count; });
        g.run(sched, [&count] { ++count; });
        return g.wait_as_sender();
    }();    // g is destroyed here, sender must keep shared state alive

    tt::sync_wait(HPX_MOVE(sender));
    HPX_TEST_EQ(count.load(), 2);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_task_group_scheduler_basic();
    test_task_group_scheduler_fib();
    test_task_group_scheduler_reuse();
    test_task_group_scheduler_exception();
    test_task_group_scheduler_move_only();
    test_task_group_scheduler_composability();
    test_define_task_block_scheduler();
    test_define_task_block_scheduler_exception();
    test_define_task_block_restore_thread_scheduler();
    test_task_group_mixed_executor_scheduler();
    test_run_on_all_scheduler();
    test_run_on_all_scheduler_workers();
    test_task_group_scheduler_legacy_wait();
    test_task_group_recursive_spawning();
    test_task_group_recursive_spawning_legacy_wait();
    test_task_group_lifetime_safety();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
