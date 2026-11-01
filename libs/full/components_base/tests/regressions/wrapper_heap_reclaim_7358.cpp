//  Copyright (c) 2026 Fabian C.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for #7358.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <shared_mutex>

namespace {

    using heap_type = hpx::components::detail::wrapper_heap;

    struct test_heap_list : hpx::util::one_size_heap_list
    {
        using hpx::util::one_size_heap_list::one_size_heap_list;

        std::size_t heap_list_size() const
        {
            std::shared_lock<hpx::shared_mutex> sl(rwlock_);
            return heap_list_.size();
        }

        std::size_t front_heap_size() const
        {
            std::shared_lock<hpx::shared_mutex> sl(rwlock_);
            HPX_TEST(!heap_list_.empty());
            if (heap_list_.empty())
                return 0;
            return heap_list_.front()->size();
        }

        std::size_t front_heap_free_size() const
        {
            std::shared_lock<hpx::shared_mutex> sl(rwlock_);
            HPX_TEST(!heap_list_.empty());
            if (heap_list_.empty())
                return 0;
            return heap_list_.front()->free_size();
        }

#if defined(HPX_DEBUG)
        std::size_t front_heap_alloc_count() const
        {
            std::shared_lock<hpx::shared_mutex> sl(rwlock_);
            HPX_TEST(!heap_list_.empty());
            if (heap_list_.empty())
                return 0;
            auto const& heap =
                static_cast<heap_type const&>(*heap_list_.front());
            return heap.alloc_count_.load(std::memory_order_relaxed);
        }
#endif
    };

    void test_reclamation()
    {
        constexpr std::size_t capacity = 8;

        test_heap_list heaps("wrapper_heap_reclaim_7358",
            {capacity, alignof(std::max_align_t), sizeof(std::max_align_t)},
            static_cast<heap_type*>(nullptr));

        std::array<void*, capacity> pointers{};

        for (void*& p : pointers)
        {
            p = heaps.alloc();
            HPX_TEST(p != nullptr);
            HPX_TEST(heaps.did_alloc(p));
        }

        void* const first = pointers.front();

        for (void* p : pointers)
            heaps.free(p);

        HPX_TEST(!heaps.did_alloc(first));
        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));

        // Returning a single allocation must not release a heap whose
        // remaining tail is still usable.
        pointers[0] = heaps.alloc();
        HPX_TEST(pointers[0] != nullptr);
        HPX_TEST_EQ(heaps.front_heap_size(), std::size_t(1));
        HPX_TEST_EQ(heaps.front_heap_free_size(), capacity - std::size_t(1));

        void* const partial = pointers[0];
        heaps.free(partial);
        HPX_TEST_EQ(heaps.front_heap_size(), std::size_t(0));
        HPX_TEST_EQ(heaps.front_heap_free_size(), capacity);

        HPX_TEST(heaps.did_alloc(partial));
        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(1));

        for (std::size_t i = 1; i < capacity; ++i)
        {
            pointers[i] = heaps.alloc();
            HPX_TEST(pointers[i] != nullptr);
        }

        for (std::size_t i = 1; i < capacity; ++i)
            heaps.free(pointers[i]);

        HPX_TEST(!heaps.did_alloc(partial));
        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));
    }

    void test_oversized_allocation()
    {
        constexpr std::size_t capacity = 8;

        test_heap_list heaps("wrapper_heap_oversized_7358",
            {capacity, alignof(std::max_align_t), sizeof(std::max_align_t)},
            static_cast<heap_type*>(nullptr));

        bool threw = false;
        try
        {
            (void) heaps.alloc(capacity + 1);
        }
        catch (std::bad_alloc const&)
        {
            threw = true;
        }

        HPX_TEST(threw);
        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));
    }

    void test_unusable_tail_reclamation()
    {
        constexpr std::size_t capacity = 8;
        constexpr std::size_t count = 3;

        test_heap_list heaps("wrapper_heap_tail_reclaim_7358",
            {capacity, alignof(std::max_align_t), sizeof(std::max_align_t)},
            static_cast<heap_type*>(nullptr));

        // Two alloc(3) requests leave a two-slot tail. Repeated cycles must not
        // accumulate backing pools that have no live allocations.
        for (std::size_t i = 0; i != 12; ++i)
        {
            void* const p = heaps.alloc(count);
            HPX_TEST(p != nullptr);

            heaps.free(p, count);
            HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(1));
        }

        // The next alloc(5) cannot use the two-slot tail, so that empty pool
        // is reclaimed and replaced. The replacement still has three usable
        // slots after the five-slot allocation is returned.
        void* const p = heaps.alloc(5);
        HPX_TEST(p != nullptr);
        heaps.free(p, 5);

        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(1));

        // Consume the replacement heap's remaining tail and return everything.
        void* const tail = heaps.alloc(3);
        HPX_TEST(tail != nullptr);
        heaps.free(tail, 3);

        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));
    }

    void test_concurrent_reclamation()
    {
        // Capacity one makes every free a possible backing-pool release.
        constexpr std::size_t capacity = 1;
        constexpr std::size_t num_tasks = 3;
        constexpr std::size_t iterations = 1000;

        test_heap_list heaps("wrapper_heap_concurrent_reclaim_7358",
            {capacity, alignof(std::max_align_t), sizeof(std::max_align_t)},
            static_cast<heap_type*>(nullptr));

        hpx::latch ready(num_tasks);
        hpx::latch start(1);

        std::array<hpx::future<bool>, num_tasks> futures;

        for (auto& f : futures)
        {
            f = hpx::async(hpx::launch::async, [&heaps, &ready, &start]() {
                ready.count_down(1);
                start.wait();

                try
                {
                    for (std::size_t i = 0; i != iterations; ++i)
                    {
                        void* const p = heaps.alloc();
                        if (p == nullptr)
                            return false;

                        heaps.free(p);
                    }
                }
                catch (...)
                {
                    return false;
                }

                return true;
            });
        }

        ready.wait();
        start.count_down(1);

        for (auto& f : futures)
            HPX_TEST(f.get());

        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));
#if defined(HPX_DEBUG)
        HPX_TEST_EQ(heaps.alloc_count_.load(std::memory_order_relaxed),
            num_tasks * iterations);
        HPX_TEST_EQ(heaps.free_count_, num_tasks * iterations);
#endif
    }

#if defined(HPX_DEBUG)
    void test_concurrent_statistics()
    {
        constexpr std::size_t num_tasks = 3;
        constexpr std::size_t iterations = 1000;
        constexpr std::size_t capacity = num_tasks * iterations;

        test_heap_list heaps("wrapper_heap_concurrent_statistics_7358",
            {capacity, alignof(std::max_align_t), sizeof(std::max_align_t)},
            static_cast<heap_type*>(nullptr));

        hpx::latch ready(num_tasks);
        hpx::latch start(1);

        std::array<std::array<void*, iterations>, num_tasks> pointers{};
        std::array<hpx::future<bool>, num_tasks> futures{};

        for (std::size_t task = 0; task != num_tasks; ++task)
        {
            futures[task] = hpx::async(hpx::launch::async,
                [&heaps, &ready, &start, &pointers, task]() {
                    ready.count_down(1);
                    start.wait();

                    try
                    {
                        for (std::size_t i = 0; i != iterations; ++i)
                        {
                            void* const p = heaps.alloc();
                            if (p == nullptr)
                                return false;

                            pointers[task][i] = p;
                        }
                    }
                    catch (...)
                    {
                        return false;
                    }

                    return true;
                });
        }

        ready.wait();
        start.count_down(1);

        for (auto& f : futures)
            HPX_TEST(f.get());

        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(1));
        HPX_TEST_EQ(
            heaps.alloc_count_.load(std::memory_order_relaxed), capacity);
        HPX_TEST_EQ(heaps.free_count_, std::size_t(0));
        HPX_TEST_EQ(
            heaps.max_alloc_count_.load(std::memory_order_relaxed), capacity);
        HPX_TEST_EQ(heaps.front_heap_alloc_count(), capacity);
        HPX_TEST_EQ(heaps.front_heap_size(), capacity);
        HPX_TEST_EQ(heaps.front_heap_free_size(), std::size_t(0));

        for (auto const& task_pointers : pointers)
        {
            for (void* p : task_pointers)
                heaps.free(p);
        }

        HPX_TEST_EQ(heaps.free_count_, capacity);
        HPX_TEST_EQ(
            heaps.max_alloc_count_.load(std::memory_order_relaxed), capacity);
        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));
    }
#endif
    void test_concurrent_bulk_reservation()
    {
        constexpr std::size_t capacity = 8;
        constexpr std::size_t count = 3;
        constexpr std::size_t num_tasks = 3;

        test_heap_list heaps("wrapper_heap_concurrent_bulk_7358",
            {capacity, alignof(std::max_align_t), sizeof(std::max_align_t)},
            static_cast<heap_type*>(nullptr));

        hpx::latch ready(num_tasks);
        hpx::latch start(1);
        hpx::latch allocated(num_tasks);
        hpx::latch release(1);

        std::array<hpx::future<bool>, num_tasks> futures;

        for (auto& f : futures)
        {
            f = hpx::async(hpx::launch::async,
                [&heaps, &ready, &start, &allocated, &release]() {
                    ready.count_down(1);
                    start.wait();

                    void* p = nullptr;
                    try
                    {
                        p = heaps.alloc(count);
                    }
                    catch (...)
                    {
                        allocated.count_down(1);
                        return false;
                    }

                    allocated.count_down(1);
                    release.wait();

                    if (p == nullptr)
                        return false;

                    heaps.free(p, count);
                    return true;
                });
        }

        ready.wait();
        start.count_down(1);

        // No allocation may be freed until all three concurrent reservations
        // have completed.
        allocated.wait();
#if defined(HPX_DEBUG)
        HPX_TEST_EQ(heaps.alloc_count_.load(std::memory_order_relaxed),
            num_tasks * count);
        HPX_TEST_EQ(heaps.free_count_, std::size_t(0));
        HPX_TEST_EQ(heaps.max_alloc_count_.load(std::memory_order_relaxed),
            num_tasks * count);
#endif
        release.count_down(1);

        for (auto& f : futures)
            HPX_TEST(f.get());

        // The first capacity-eight heap is retired/reclaimed; the replacement
        // heap remains reusable.
        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(1));
        HPX_TEST_EQ(heaps.front_heap_size(), std::size_t(0));
        HPX_TEST_EQ(heaps.front_heap_free_size(), capacity);

        void* const p = heaps.alloc(5);
        HPX_TEST(p != nullptr);
        heaps.free(p, 5);

        HPX_TEST_EQ(heaps.heap_list_size(), std::size_t(0));
    }

}    // namespace

int hpx_main()
{
    test_reclamation();
    test_oversized_allocation();
    test_unusable_tail_reclamation();
    test_concurrent_reclamation();
#if defined(HPX_DEBUG)
    test_concurrent_statistics();
#endif
    test_concurrent_bulk_reservation();

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#endif
