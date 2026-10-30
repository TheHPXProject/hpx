//  Copyright (c) 1998-2024 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components_base/server/one_size_heap_list.hpp>
#include <hpx/components_base/server/wrapper_heap_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#if defined(HPX_DEBUG)
#include <hpx/modules/logging.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace hpx::util {

#if defined(HPX_DEBUG)
    namespace {
        // The caller holds rwlock_ shared or exclusively, keeping free_count
        // stable while concurrent allocations update the atomic counters.
        void update_alloc_statistics(std::atomic<std::size_t>& alloc_count,
            std::size_t free_count, std::atomic<std::size_t>& max_alloc_count,
            std::size_t count) noexcept
        {
            std::size_t const total_alloc_count =
                alloc_count.fetch_add(count, std::memory_order_relaxed) + count;

            HPX_ASSERT(total_alloc_count >= free_count);
            std::size_t const current_alloc_count =
                total_alloc_count - free_count;

            std::size_t max_count =
                max_alloc_count.load(std::memory_order_relaxed);

            while (max_count < current_alloc_count &&
                !max_alloc_count.compare_exchange_weak(max_count,
                    current_alloc_count, std::memory_order_relaxed,
                    std::memory_order_relaxed))
            {
            }
        }
    }    // namespace
#endif
    one_size_heap_list::one_size_heap_list()
    {
        HPX_ASSERT(false);    // shouldn't ever be called
    }

    one_size_heap_list::~one_size_heap_list() noexcept
    {
#if defined(HPX_DEBUG)
        std::size_t const alloc_count =
            alloc_count_.load(std::memory_order_relaxed);
        std::size_t const max_alloc_count =
            max_alloc_count_.load(std::memory_order_relaxed);

        LOSH_(info).format(
            "{1}::~{1}: size({2}), max_count({3}), alloc_count({4}), "
            "free_count({5})",
            name(), heap_count_, max_alloc_count, alloc_count, free_count_);

        if (alloc_count > free_count_)
        {
            LOSH_(warning).format(
                "{1}::~{1}: releasing with {2} allocated objects", name(),
                alloc_count - free_count_);
        }
#endif
    }

    void* one_size_heap_list::alloc(std::size_t count)
    {
        if (HPX_UNLIKELY(0 == count))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, name() + "::alloc",
                "cannot allocate 0 objects");
        }

        if (HPX_UNLIKELY(count > parameters_.capacity))
        {
            HPX_THROW_BAD_ALLOC("one_size_heap_list::alloc");
        }

        void* p = nullptr;

        // Fast path: allocations from existing heaps may proceed concurrently.
        {
            std::shared_lock<hpx::shared_mutex> sl(rwlock_);

            for (auto& heap : heap_list_)
            {
                if (heap->alloc(&p, count))
                {
#if defined(HPX_DEBUG)
                    update_alloc_statistics(
                        alloc_count_, free_count_, max_alloc_count_, count);
#endif
                    return p;
                }

#if defined(HPX_DEBUG)
                LOSH_(info).format(
                    "{1}::alloc: failed to allocate from heap[{2}] "
                    "(heap[{2}] has allocated {3} objects and has "
                    "space for {4} more objects)",
                    name(), heap->heap_count(), heap->size(),
                    heap->free_size());
#endif
            }
        }

        // Slow path: retry under exclusive ownership. This prevents redundant
        // replacement heaps and allows unusable trailing storage to be retired
        // safely without racing alloc/free/did_alloc/get_gid.
        std::unique_lock<hpx::shared_mutex> ul(rwlock_);

        for (auto it = heap_list_.begin(); it != heap_list_.end();)
        {
            auto const& heap = *it;

            if (heap->alloc(&p, count))
            {
#if defined(HPX_DEBUG)
                update_alloc_statistics(
                    alloc_count_, free_count_, max_alloc_count_, count);
#endif
                return p;
            }

            if (heap->reclaim_if_unusable(count))
                it = heap_list_.erase(it);
            else
                ++it;
        }

        // No existing heap can satisfy this request. Create one replacement
        // while still owning the exclusive list lock.
        std::shared_ptr<util::wrapper_heap_base> heap;
#if defined(HPX_DEBUG)
        heap = create_heap_(class_name_.c_str(), heap_count_ + 1, parameters_);
#else
        heap = create_heap_(class_name_.c_str(), 0, parameters_);
#endif

        if (HPX_UNLIKELY(!heap->alloc(&p, count) || nullptr == p))
        {
            // wrapper_heap can cleanly release an unusable fresh pool. Other
            // implementations retain the historical ownership behavior.
            if (!heap->reclaim_if_unusable(count))
                heap_list_.push_front(heap);

            ul.unlock();
            HPX_THROW_BAD_ALLOC("one_size_heap_list::alloc");
        }

        heap_list_.push_front(heap);

#if defined(HPX_DEBUG)
        update_alloc_statistics(
            alloc_count_, free_count_, max_alloc_count_, count);
        ++heap_count_;

        LOSH_(info).format(
            "{1}::alloc: creating new heap[{2}], size is now {3}", name(),
            heap_count_, heap_list_.size());
#endif

        return p;
    }

    bool one_size_heap_list::reschedule(void* p, std::size_t count)
    {
        if (nullptr == threads::get_self_ptr())
        {
            hpx::threads::thread_init_data data(
                hpx::threads::make_thread_function_nullary(
                    hpx::bind_front(&one_size_heap_list::free, this, p, count)),
                "one_size_heap_list::free");
            hpx::threads::register_work(data);
            return true;
        }
        return false;
    }

    void one_size_heap_list::free(void* p, std::size_t count)
    {
        if (nullptr == p || !threads::threadmanager_is(hpx::state::running))
        {
            return;
        }

        // if this is called from outside a HPX thread we need to
        // re-schedule the request
        if (reschedule(p, count))
            return;

        {
            // free() may release the backing pool, so exclude concurrent heap
            // users while freeing an element.
            std::unique_lock<hpx::shared_mutex> ul(rwlock_);

            // Find the heap which allocated this pointer.
            for (auto it = heap_list_.begin(); it != heap_list_.end(); ++it)
            {
                auto const& heap = *it;
                if (heap->did_alloc(p))
                {
                    heap->free(p, count);
#if defined(HPX_DEBUG)
                    free_count_ += count;
#endif

                    // did_alloc() tests ownership of the backing pool. If it
                    // becomes false, free() released the pool and this entry
                    // is no longer useful.
                    if (!heap->did_alloc(p))
                        heap_list_.erase(it);

                    return;
                }
            }
        }

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter, name() + "::free",
            "pointer {1} was not allocated by this {2}", p, name());
    }

    bool one_size_heap_list::did_alloc(void* p) const
    {
        std::shared_lock<hpx::shared_mutex> sl(rwlock_);
        for (auto const& heap : heap_list_)
        {
            if (heap->did_alloc(p))
            {
                return true;
            }
        }
        return false;
    }

    std::string one_size_heap_list::name() const
    {
        if (class_name_.empty())
        {
            return {"one_size_heap_list(unknown)"};
        }
        return std::string("one_size_heap_list(") + class_name_ + ")";
    }
}    // namespace hpx::util
