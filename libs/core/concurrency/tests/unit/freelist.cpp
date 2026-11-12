//  Copyright (C) 2011 Tim Blechmann
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <set>

#include "test_helpers.hpp"

std::atomic<bool> test_running(false);

struct dummy
{
    dummy()
      : allocated(0)
    {
        padding[0] = padding[1] = 0;
        if (test_running.load(std::memory_order_relaxed))
            HPX_TEST(allocated == 0);
        allocated = 1;
    }

    ~dummy()
    {
        if (test_running.load(std::memory_order_relaxed))
            HPX_TEST(allocated == 1);
        allocated = 0;
    }

    std::size_t padding[2];    // used for the freelist node
    int allocated;
};

template <typename Freelist, bool ThreadSafe, bool Bounded>
void run_test()
{
    Freelist fl(std::allocator<int>(), 8);

    std::set<dummy*> nodes;

    dummy d;
    if constexpr (Bounded)
        test_running.store(true);

    for (int i = 0; i != 4; ++i)
    {
        dummy* allocated = fl.template construct<ThreadSafe, Bounded>();
        HPX_TEST(nodes.find(allocated) == nodes.end());
        nodes.insert(allocated);
    }

    for (dummy* d : nodes)
        fl.template destruct<ThreadSafe>(d);

    nodes.clear();
    for (int i = 0; i != 4; ++i)
        nodes.insert(fl.template construct<ThreadSafe, Bounded>());

    for (dummy* d : nodes)
        fl.template destruct<ThreadSafe>(d);

    for (int i = 0; i != 4; ++i)
        nodes.insert(fl.template construct<ThreadSafe, Bounded>());

    if constexpr (Bounded)
        test_running.store(false);
}

template <bool Bounded>
void run_tests()
{
    run_test<hpx::lockfree::detail::freelist_stack<dummy>, true, Bounded>();
    run_test<hpx::lockfree::detail::freelist_stack<dummy>, false, Bounded>();
    run_test<hpx::lockfree::detail::fixed_size_freelist<dummy>, true,
        Bounded>();
}

void freelist_tests()
{
    run_tests<false>();
    run_tests<true>();
}

template <typename Freelist, bool ThreadSafe>
void oom_test(void)
{
    constexpr bool bounded = true;
    Freelist fl(std::allocator<int>(), 8);

    for (int i = 0; i != 8; ++i)
        fl.template construct<ThreadSafe, bounded>();

    dummy* allocated = fl.template construct<ThreadSafe, bounded>();
    HPX_TEST(allocated == nullptr);
}

void oom_tests()
{
    oom_test<hpx::lockfree::detail::freelist_stack<dummy>, true>();
    oom_test<hpx::lockfree::detail::freelist_stack<dummy>, false>();
    oom_test<hpx::lockfree::detail::fixed_size_freelist<dummy>, true>();
    oom_test<hpx::lockfree::detail::fixed_size_freelist<dummy>, false>();
}

template <typename Freelist, bool Bounded>
struct freelist_tester
{
    static constexpr int size = 128;
    static constexpr int thread_count = 2;
    static constexpr int operations_per_thread = 10000;

    Freelist fl;
    hpx::lockfree::queue<dummy*> allocated_nodes;

    std::atomic<bool> running;
    static_hashed_set<dummy*, 1 << 16> working_set;

    freelist_tester()
      : fl(std::allocator<int>(), size)
      , allocated_nodes(256)
      , running(false)
    {
    }

    void run()
    {
        running = true;

        if constexpr (Bounded)
            test_running.store(true);

        hpx::experimental::task_group alloc_threads;
        hpx::experimental::task_group dealloc_threads;

        for (int i = 0; i != thread_count; ++i)
            dealloc_threads.run(&freelist_tester::deallocate, this);

        for (int i = 0; i != thread_count; ++i)
            alloc_threads.run(&freelist_tester::allocate, this);

        alloc_threads.wait();

        test_running.store(false);
        running = false;

        dealloc_threads.wait();
    }

    void allocate(void)
    {
        for (long i = 0; i != operations_per_thread; ++i)
        {
            for (;;)
            {
                dummy* node = fl.template construct<true, Bounded>();
                if (node)
                {
                    bool success = working_set.insert(node);
                    HPX_TEST(success);
                    allocated_nodes.push(node);
                    break;
                }

                // The bounded freelist is empty until a deallocating task
                // returns a node. HPX threads are not preempted, so let the
                // deallocating tasks run, otherwise the allocating tasks may
                // occupy all worker threads and spin forever.
                hpx::this_thread::yield();
            }

            hpx::this_thread::yield();
        }
    }

    void deallocate(void)
    {
        for (;;)
        {
            dummy* node;
            if (allocated_nodes.pop(node))
            {
                bool success = working_set.erase(node);
                HPX_TEST(success);
                fl.template destruct<true>(node);
            }

            if (!running.load())
                break;

            hpx::this_thread::yield();
        }

        dummy* node;
        while (allocated_nodes.pop(node))
        {
            bool success = working_set.erase(node);
            HPX_TEST(success);
            fl.template destruct<true>(node);
        }
    }
};

template <typename Tester>
void run_tester()
{
    std::unique_ptr<Tester> tester(new Tester);
    tester->run();
}

void unbounded_freelist_test()
{
    using test_type =
        freelist_tester<hpx::lockfree::detail::freelist_stack<dummy>, false>;
    run_tester<test_type>();
}

void bounded_freelist_test()
{
    using test_type =
        freelist_tester<hpx::lockfree::detail::freelist_stack<dummy>, true>;
    run_tester<test_type>();
}

void fixed_size_freelist_test()
{
    using test_type =
        freelist_tester<hpx::lockfree::detail::fixed_size_freelist<dummy>,
            true>;
    run_tester<test_type>();
}

int hpx_main(hpx::program_options::variables_map&)
{
    freelist_tests();
    oom_tests();
    unbounded_freelist_test();
    bounded_freelist_test();
    fixed_size_freelist_test();

    return hpx::local::finalize();
}

int main(int argc, char** argv)
{
    hpx::local::init(hpx_main, argc, argv);
    return hpx::util::report_errors();
}
