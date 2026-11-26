//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A shared_suspending_fork_join_executor can be used from many threads at the
// same time and from inside its own regions: a region that finds the worker
// threads busy runs through a parallel_executor instead.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using hpx::execution::experimental::shared_suspending_fork_join_executor;

// the executor can be a constexpr variable
constexpr shared_suspending_fork_join_executor global_exec{};

#if defined(HPX_HAVE_SUSPENDING_FORK_JOIN_EXECUTOR_AS_DEFAULT)
static_assert(std::is_same_v<hpx::execution::parallel_policy::executor_type,
    shared_suspending_fork_join_executor>);
static_assert(
    std::is_same_v<hpx::execution::parallel_unsequenced_policy::executor_type,
        shared_suspending_fork_join_executor>);
#else
static_assert(std::is_same_v<hpx::execution::parallel_policy::executor_type,
    hpx::execution::parallel_executor>);
static_assert(
    std::is_same_v<hpx::execution::parallel_unsequenced_policy::executor_type,
        hpx::execution::parallel_executor>);
#endif

// the task policies always use parallel_executor, also when mapped from a
// policy with a shared_suspending_fork_join_executor
static_assert(
    std::is_same_v<hpx::execution::parallel_task_policy::executor_type,
        hpx::execution::parallel_executor>);
static_assert(std::is_same_v<
    hpx::execution::parallel_unsequenced_task_policy::executor_type,
    hpx::execution::parallel_executor>);
static_assert(
    std::is_same_v<std::decay_t<decltype(hpx::execution::par.on(
                       std::declval<shared_suspending_fork_join_executor>())(
                       hpx::execution::task))>::executor_type,
        hpx::execution::parallel_executor>);
static_assert(
    std::is_same_v<std::decay_t<decltype(hpx::execution::par_unseq.on(
                       std::declval<shared_suspending_fork_join_executor>())(
                       hpx::execution::task))>::executor_type,
        hpx::execution::parallel_executor>);

// the properties give a shared executor
static_assert(
    std::is_same_v<decltype(hpx::execution::experimental::with_priority(
                       global_exec, hpx::threads::thread_priority::default_)),
        shared_suspending_fork_join_executor>);
static_assert(
    std::is_same_v<decltype(hpx::execution::experimental::with_stacksize(
                       global_exec, hpx::threads::thread_stacksize::medium)),
        shared_suspending_fork_join_executor>);
static_assert(std::is_same_v<
    decltype(hpx::execution::experimental::with_processing_units_count(
        global_exec, 2)),
    shared_suspending_fork_join_executor>);
static_assert(
    std::is_same_v<decltype(hpx::execution::experimental::with_first_core(
                       global_exec, 1)),
        shared_suspending_fork_join_executor>);
static_assert(
    std::is_same_v<decltype(hpx::execution::experimental::with_annotation(
                       global_exec, "test")),
        shared_suspending_fork_join_executor>);
static_assert(
    std::is_same_v<decltype(hpx::execution::experimental::with_hint(
                       global_exec, hpx::threads::thread_schedule_hint{})),
        shared_suspending_fork_join_executor>);

// all elements are visited exactly once
void test_bulk(shared_suspending_fork_join_executor const& exec)
{
    for (std::size_t const size : {0, 1, 7, 1000, 100000})
    {
        std::vector<std::atomic<int>> visited(size);
        std::vector<std::size_t> indices(size);
        std::iota(indices.begin(), indices.end(), std::size_t(0));

        hpx::parallel::execution::bulk_sync_execute(
            exec, [&](std::size_t const i) { ++visited[i]; }, indices);

        for (auto const& v : visited)
        {
            HPX_TEST_EQ(v.load(), 1);
        }

        auto const results = hpx::parallel::execution::bulk_sync_execute(
            exec, [](std::size_t const i) { return 2 * i; }, indices);
        HPX_TEST_EQ(results.size(), size);
        for (std::size_t i = 0; i != results.size(); ++i)
        {
            HPX_TEST_EQ(results[i], 2 * i);
        }
    }
}

// a region started from inside a region falls back to the parallel_executor
void test_nested()
{
    shared_suspending_fork_join_executor const exec;

    constexpr std::size_t outer = 64;
    constexpr std::size_t inner = 1000;

    std::vector<std::size_t> outer_indices(outer);
    std::iota(outer_indices.begin(), outer_indices.end(), std::size_t(0));
    std::vector<std::size_t> inner_indices(inner);
    std::iota(inner_indices.begin(), inner_indices.end(), std::size_t(0));

    std::atomic<std::size_t> count{0};
    hpx::parallel::execution::bulk_sync_execute(
        exec,
        [&](std::size_t) {
            hpx::parallel::execution::bulk_sync_execute(
                exec, [&](std::size_t) { ++count; }, inner_indices);
        },
        outer_indices);

    HPX_TEST_EQ(count.load(), outer * inner);
}

// many threads use the executor at the same time
void test_concurrent()
{
    constexpr std::size_t num_tasks = 32;
    constexpr std::size_t num_regions = 100;
    constexpr std::size_t size = 1000;

    std::vector<std::size_t> indices(size);
    std::iota(indices.begin(), indices.end(), std::size_t(0));

    std::vector<hpx::future<std::size_t>> futures;
    futures.reserve(num_tasks);
    for (std::size_t t = 0; t != num_tasks; ++t)
    {
        futures.push_back(hpx::async([&]() {
            shared_suspending_fork_join_executor const exec;
            std::atomic<std::size_t> count{0};
            for (std::size_t r = 0; r != num_regions; ++r)
            {
                hpx::parallel::execution::bulk_sync_execute(
                    exec, [&](std::size_t) { ++count; }, indices);
            }
            return count.load();
        }));
    }

    for (auto& f : futures)
    {
        HPX_TEST_EQ(f.get(), num_regions * size);
    }
}

// exceptions reach the caller, from the worker threads and from the fallback
void test_exceptions()
{
    shared_suspending_fork_join_executor const exec;

    std::vector<std::size_t> indices(1000);
    std::iota(indices.begin(), indices.end(), std::size_t(0));

    auto const throwing = [](std::size_t const i) {
        if (i == 517)
        {
            throw std::runtime_error("test");
        }
    };

    bool caught = false;
    try
    {
        hpx::parallel::execution::bulk_sync_execute(exec, throwing, indices);
    }
    catch (hpx::exception_list const&)
    {
        caught = true;
    }
    catch (std::runtime_error const&)
    {
        caught = true;
    }
    HPX_TEST(caught);

    // nested: the inner region runs through the parallel_executor
    std::atomic<std::size_t> caught_inner{0};
    std::vector<std::size_t> outer(8);
    hpx::parallel::execution::bulk_sync_execute(
        exec,
        [&](std::size_t) {
            try
            {
                hpx::parallel::execution::bulk_sync_execute(
                    exec, throwing, indices);
            }
            catch (hpx::exception_list const&)
            {
                ++caught_inner;
            }
            catch (std::runtime_error const&)
            {
                ++caught_inner;
            }
        },
        outer);
    HPX_TEST_EQ(caught_inner.load(), outer.size());

    // the executor still works
    test_bulk(exec);
}

// the parallel algorithms work with the executor, also nested
void test_algorithms()
{
    shared_suspending_fork_join_executor const exec;
    auto const policy = hpx::execution::par.on(exec);

    std::vector<std::uint64_t> v(100000);
    std::iota(v.begin(), v.end(), std::uint64_t(0));
    std::uint64_t const expected = (v.size() - 1) * v.size() / 2;

    HPX_TEST_EQ(
        hpx::reduce(policy, v.begin(), v.end(), std::uint64_t(0)), expected);
    HPX_TEST_EQ(hpx::reduce(policy(hpx::execution::task), v.begin(), v.end(),
                    std::uint64_t(0))
                    .get(),
        expected);
    HPX_TEST_EQ(hpx::reduce(hpx::execution::par_unseq.on(exec), v.begin(),
                    v.end(), std::uint64_t(0)),
        expected);

    std::vector<std::uint64_t> sums(v.size());
    hpx::inclusive_scan(policy, v.begin(), v.end(), sums.begin());
    HPX_TEST_EQ(sums.back(), expected);

    std::vector<std::uint64_t> reversed(v.rbegin(), v.rend());
    hpx::sort(policy, reversed.begin(), reversed.end());
    HPX_TEST(reversed == v);

    std::atomic<std::uint64_t> nested_sum{0};
    hpx::for_each(policy, v.begin(), v.begin() + 100, [&](std::uint64_t) {
        nested_sum += hpx::reduce(policy, v.begin(), v.end(), std::uint64_t(0));
    });
    HPX_TEST_EQ(nested_sum.load(), 100 * expected);

    // the default parallel policies (a shared_suspending_fork_join_executor
    // with HPX_WITH_SUSPENDING_FORK_JOIN_EXECUTOR_AS_DEFAULT=ON)
    HPX_TEST_EQ(
        hpx::reduce(hpx::execution::par, v.begin(), v.end(), std::uint64_t(0)),
        expected);
    HPX_TEST_EQ(hpx::reduce(hpx::execution::par_unseq, v.begin(), v.end(),
                    std::uint64_t(0)),
        expected);
    HPX_TEST_EQ(hpx::reduce(hpx::execution::par(hpx::execution::task),
                    v.begin(), v.end(), std::uint64_t(0))
                    .get(),
        expected);
}

// A region runs on num_cores worker threads only, starting at first_core: the
// worker threads (other than the calling thread) that take part are in that
// window.
template <typename Executor>
void test_cores(Executor const& exec)
{
    std::size_t const num_threads = hpx::get_num_worker_threads();
    HPX_TEST_LTE(num_threads, std::size_t(64));

    std::vector<std::size_t> indices(1000);
    std::iota(indices.begin(), indices.end(), std::size_t(0));

    for (std::size_t num_cores = 1; num_cores <= num_threads; ++num_cores)
    {
        for (std::size_t first_core = 0; first_core != num_threads;
            ++first_core)
        {
            auto const e = hpx::execution::experimental::with_first_core(
                hpx::execution::experimental::with_processing_units_count(
                    exec, num_cores),
                first_core);
            static_assert(std::is_same_v<std::decay_t<decltype(e)>, Executor>);

            HPX_TEST_EQ(hpx::execution::experimental::processing_units_count(e),
                num_cores);

            std::vector<std::atomic<int>> visited(indices.size());
            std::atomic<std::uint64_t> workers{0};
            auto const caller = hpx::threads::get_self_id();
            hpx::parallel::execution::bulk_sync_execute(
                e,
                [&](std::size_t const i) {
                    ++visited[i];
                    if (hpx::threads::get_self_id() != caller)
                    {
                        workers |= std::uint64_t(1)
                            << hpx::get_worker_thread_num();
                    }
                },
                indices);

            for (auto const& v : visited)
            {
                HPX_TEST_EQ(v.load(), 1);
            }

            std::uint64_t const used = workers.load();
            HPX_TEST_LTE(
                static_cast<std::size_t>(std::popcount(used)), num_cores);
            for (std::size_t w = 0; w != num_threads; ++w)
            {
                if (used & (std::uint64_t(1) << w))
                {
                    HPX_TEST_LT((w + num_threads - first_core) % num_threads,
                        num_cores);
                }
            }
        }
    }
}

// The properties apply to the parallel_executor as well, which runs single
// tasks and the task policies.
void test_properties()
{
    namespace ex = hpx::execution::experimental;

    hpx::threads::thread_schedule_hint const hint(static_cast<std::int16_t>(1));
    auto const exec = ex::with_hint(
        ex::with_stacksize(
            ex::with_priority(
                ex::with_first_core(
                    ex::with_processing_units_count(global_exec, 2), 1),
                hpx::threads::thread_priority::high),
            hpx::threads::thread_stacksize::medium),
        hint);

    HPX_TEST_EQ(ex::get_priority(exec), hpx::threads::thread_priority::high);
    HPX_TEST_EQ(
        ex::get_stacksize(exec), hpx::threads::thread_stacksize::medium);
    HPX_TEST(ex::get_hint(exec) == hint);
    HPX_TEST_EQ(ex::processing_units_count(exec), std::size_t(2));

    auto const task_exec =
        hpx::execution::par.on(exec)(hpx::execution::task).executor();
    HPX_TEST_EQ(
        ex::get_priority(task_exec), hpx::threads::thread_priority::high);
    HPX_TEST_EQ(
        ex::get_stacksize(task_exec), hpx::threads::thread_stacksize::medium);
    HPX_TEST(ex::get_hint(task_exec) == hint);
    HPX_TEST_EQ(ex::processing_units_count(task_exec), std::size_t(2));
    HPX_TEST_EQ(ex::get_first_core(task_exec), std::size_t(1));

    // the worker threads have their own priority, unless it was set
    auto const task_exec_default =
        hpx::execution::par.on(global_exec)(hpx::execution::task).executor();
    HPX_TEST_EQ(ex::get_priority(task_exec_default),
        ex::get_priority(hpx::execution::parallel_executor()));

    // single tasks run with these properties
    HPX_TEST_EQ(
        hpx::parallel::execution::async_execute(exec,
            [] {
                return hpx::threads::get_self_id_data()->get_stack_size_enum();
            })
            .get(),
        hpx::threads::thread_stacksize::medium);

    // the regions too
    test_bulk(exec);
}

// Iterations that wait for each other: the hint says that tasks must not be
// combined, so each iteration gets a task of its own. There are more of them
// than worker threads, which would deadlock on the worker threads.
void test_interdependent()
{
    namespace ex = hpx::execution::experimental;

    constexpr std::size_t size = 64;
    hpx::threads::thread_schedule_hint hint = ex::get_hint(global_exec);
    hint.sharing_mode(hpx::threads::thread_sharing_hint::do_not_combine_tasks);
    auto const exec = ex::with_hint(global_exec, hint);

    hpx::latch all_arrived(static_cast<std::ptrdiff_t>(size));
    std::atomic<std::size_t> count{0};
    hpx::parallel::execution::bulk_sync_execute(
        exec,
        [&](std::size_t) {
            all_arrived.arrive_and_wait();
            ++count;
        },
        hpx::util::counting_shape(size));
    HPX_TEST_EQ(count.load(), size);

    // the algorithms that need a task for each part use the
    // parallel_executor
    static_assert(
        std::is_same_v<decltype(hpx::execution::to_hierarchical_spawning(
                           global_exec)),
            decltype(hpx::execution::to_hierarchical_spawning(
                hpx::execution::parallel_executor()))>);
}

// A function object with state that is not thread safe: each share of a
// region (and each task of the fallback) calls its own copy, as each task of
// a parallel_executor does.
struct stateful_function
{
    std::atomic<std::size_t>* calls;
    std::size_t in_call = 0;

    void operator()(std::size_t)
    {
        HPX_TEST_EQ(in_call++, std::size_t(0));
        for (int i = 0; i != 100; ++i)
        {
            HPX_SMT_PAUSE;
        }
        ++*calls;
        --in_call;
    }
};

template <typename Executor>
void test_stateful_function(Executor const& exec)
{
    std::atomic<std::size_t> calls{0};
    std::vector<std::size_t> indices(10000);
    hpx::parallel::execution::bulk_sync_execute(
        exec, stateful_function{&calls}, indices);
    HPX_TEST_EQ(calls.load(), indices.size());

    // the fallback, from inside a region
    std::atomic<std::size_t> nested_calls{0};
    hpx::parallel::execution::bulk_sync_execute(
        exec,
        [&](std::size_t) {
            hpx::parallel::execution::bulk_sync_execute(
                global_exec, stateful_function{&nested_calls}, indices);
        },
        std::vector<std::size_t>(4));
    HPX_TEST_EQ(nested_calls.load(), 4 * indices.size());
}

// the configuration of a shared executor can be sent elsewhere
void test_serialization()
{
    auto const exec = hpx::execution::experimental::with_first_core(
        hpx::execution::experimental::with_processing_units_count(
            shared_suspending_fork_join_executor(
                hpx::threads::thread_priority::default_,
                hpx::threads::thread_stacksize::medium,
                shared_suspending_fork_join_executor::loop_schedule::static_,
                std::chrono::microseconds(30)),
            2),
        1);

    std::vector<char> buffer;
    {
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << exec;
    }

    shared_suspending_fork_join_executor loaded;
    HPX_TEST(loaded != exec);
    {
        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> loaded;
    }
    HPX_TEST(loaded == exec);
    HPX_TEST_EQ(hpx::execution::experimental::get_priority(loaded),
        hpx::threads::thread_priority::default_);
    HPX_TEST_EQ(hpx::execution::experimental::get_stacksize(loaded),
        hpx::threads::thread_stacksize::medium);

    test_bulk(loaded);
}

// a thread that is not an HPX thread uses the parallel_executor
void test_external_thread()
{
    std::vector<std::size_t> indices(1000);
    std::atomic<std::size_t> count{0};

    std::thread t([&]() {
        hpx::parallel::execution::bulk_sync_execute(
            global_exec, [&](std::size_t) { ++count; }, indices);
    });
    t.join();

    HPX_TEST_EQ(count.load(), indices.size());
}

int hpx_main()
{
    test_bulk(global_exec);
    test_bulk(shared_suspending_fork_join_executor(
        hpx::threads::thread_priority::default_,
        hpx::threads::thread_stacksize::small_,
        shared_suspending_fork_join_executor::loop_schedule::static_));
    test_nested();
    test_concurrent();
    test_exceptions();
    test_algorithms();
    test_external_thread();
    test_cores(global_exec);
    test_properties();
    test_interdependent();
    test_stateful_function(global_exec);
    test_stateful_function(
        hpx::execution::experimental::suspending_fork_join_executor());
    test_cores(hpx::execution::experimental::suspending_fork_join_executor());
    test_serialization();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
