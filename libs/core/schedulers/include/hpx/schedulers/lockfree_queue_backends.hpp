////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Bryce Adelstein-Lelbach
//  Copyright (c) 2019-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concurrency.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace hpx::threads::policies {

    HPX_CXX_CORE_EXPORT struct lockfree_fifo;

    ///////////////////////////////////////////////////////////////////////////////
    // FIFO
    HPX_CXX_CORE_EXPORT template <typename T>
    struct lockfree_fifo_backend
    {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        using container_type = hpx::lockfree::deque<T,
            hpx::lockfree::caching_freelist_t, hpx::util::aligned_allocator<T>>;
#else
        using container_type =
            hpx::lockfree::queue<T, hpx::util::aligned_allocator<T>>;
#endif

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        static constexpr bool support_bulk_dequeue = false;

        explicit lockfree_fifo_backend(size_type const initial_size = 0,
            size_type /* num_thread */ = static_cast<size_type>(-1))
          : queue_(static_cast<std::size_t>(initial_size))
        {
        }

        bool push(const_reference val, bool /*other_end*/ = false)    //-V659
        {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
            return queue_.push_left(val);
#else
            return queue_.push(val);
#endif
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)    //-V659
        {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
            return queue_.push_left(HPX_MOVE(val));
#else
            return queue_.push(HPX_MOVE(val));
#endif
        }

        bool pop(reference val, bool /* steal */ = true) noexcept
        {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
            return queue_.pop_right(val);
#else
            return queue_.pop(val);
#endif
        }

        bool empty() noexcept
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    HPX_CXX_CORE_EXPORT struct lockfree_fifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_fifo_backend<T>;
        };
    };

    // Dual-queue push-to-passive semantics, conditional (128-bit atomic)
    // end-selection for pop, distinct steal=true vs. steal=false pop control
    // flow (bounded retries + parity toggle), and empty() requiring both
    // queues to be empty.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct lockfree_fifo_double_backend
    {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        using container_type = hpx::lockfree::deque<T,
            hpx::lockfree::caching_freelist_t, hpx::util::aligned_allocator<T>>;
#else
        using container_type =
            hpx::lockfree::queue<T, hpx::util::aligned_allocator<T>>;
#endif

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        static constexpr bool support_bulk_dequeue = false;

        explicit lockfree_fifo_double_backend(size_type const initial_size = 0,
            size_type /* num_thread */ = static_cast<size_type>(-1))
          : queues_(static_cast<std::size_t>(initial_size))
        {
        }

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        bool push(const_reference val, bool /*other_end*/ = false)    //-V659
        {
            return current_passive().push_left(val);
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)    //-V659
        {
            return current_passive().push_left(HPX_MOVE(val));
        }
#else
        bool push(const_reference val, bool /*other_end*/ = false)    //-V659
        {
            return current_passive().push(val);
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)    //-V659
        {
            return current_passive().push(HPX_MOVE(val));
        }
#endif

        bool pop(reference val, bool const steal = true) noexcept
        {
            auto [active, passive] = indices(std::memory_order_acquire);

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
            if (steal)
            {
                return queues_[passive].pop_right(val) ||
                    queues_[active].pop_left(val);
            }

            constexpr int max_retries = 4;
            for (int i = 0; i < max_retries; ++i)
            {
                if (queues_[active].pop_right(val))
                    return true;

                HPX_SMT_PAUSE;
            }

            // Toggle parity (single-threaded exchanger, so no guard needed)
            idx_.fetch_xor(1u, std::memory_order_acq_rel);

            // After toggling parity, the newly-selected "active" is the other
            // queue.
            return queues_[passive].pop_right(val);
#else
            if (steal)
            {
                return queues_[passive].pop(val) || queues_[active].pop(val);
            }

            constexpr int max_retries = 4;
            for (int i = 0; i < max_retries; ++i)
            {
                if (queues_[active].pop(val))
                    return true;

                HPX_SMT_PAUSE;
            }

            // Toggle parity (single-threaded exchanger, so no guard needed)
            idx_.fetch_xor(1u, std::memory_order_acq_rel);

            // After toggling parity, the newly-selected "active" is the other
            // queue (the previously passive one).
            return queues_[passive].pop(val);
#endif
        }

        bool empty() const noexcept
        {
            auto [active, passive] = indices();
            return queues_[active].empty() && queues_[passive].empty();
        }

    private:
        static constexpr unsigned int active_index(
            unsigned int const idx) noexcept
        {
            return (idx & 0x1) ? 0 : 1;
        }

        static constexpr unsigned int passive_index(
            unsigned int const idx) noexcept
        {
            return (idx & 0x1) ? 1 : 0;
        }

        container_type& current_active(
            std::memory_order const mo = std::memory_order_acquire) noexcept
        {
            return queues_[active_index(idx_.load(mo))];
        }
        container_type const& current_active(
            std::memory_order const mo =
                std::memory_order_acquire) const noexcept
        {
            return queues_[active_index(idx_.load(mo))];
        }

        container_type& current_passive(
            std::memory_order const mo = std::memory_order_acquire) noexcept
        {
            return queues_[passive_index(idx_.load(mo))];
        }
        container_type const& current_passive(
            std::memory_order const mo =
                std::memory_order_acquire) const noexcept
        {
            return queues_[passive_index(idx_.load(mo))];
        }

        std::pair<unsigned int, unsigned int> indices(
            std::memory_order const mo =
                std::memory_order_relaxed) const noexcept
        {
            unsigned int const idx = idx_.load(mo);
            return {active_index(idx), passive_index(idx)};
        }

        struct queues
        {
            explicit queues(std::size_t const initial_size)
              : data{container_type(initial_size), container_type(initial_size)}
            {
            }

            constexpr container_type& operator[](
                unsigned int const idx) noexcept
            {
                return data[idx];
            }
            constexpr container_type const& operator[](
                unsigned int const idx) const noexcept
            {
                return data[idx];
            }

            container_type data[2];
        } queues_;

        util::cache_aligned_data_derived<std::atomic<unsigned int>> idx_ = 0;
    };

    HPX_CXX_CORE_EXPORT struct lockfree_fifo_double
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_fifo_double_backend<T>;
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    // MoodyCamel FIFO
    HPX_CXX_CORE_EXPORT template <typename T>
    struct moodycamel_fifo_backend
    {
        using container_type = hpx::concurrency::ConcurrentQueue<T>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        static constexpr bool support_bulk_dequeue = true;

        explicit moodycamel_fifo_backend(size_type const initial_size = 0,
            size_type /* num_thread */ = static_cast<size_type>(-1))
          : queue_(static_cast<std::size_t>(initial_size))
        {
        }

        bool push(const_reference val, bool /*other_end*/ = false)    //-V659
        {
            return queue_.enqueue(val);
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)    //-V659
        {
            return queue_.enqueue(HPX_MOVE(val));
        }

        bool pop(reference val, bool /* steal */ = true) noexcept(
            noexcept(std::is_nothrow_copy_constructible_v<T>))
        {
            return queue_.try_dequeue(val);
        }

        template <typename Iterator>
        std::size_t pop_bulk(Iterator it, std::int64_t max_items,
            bool /* steal */ = true) noexcept(noexcept(std::
                is_nothrow_copy_constructible_v<T>))
        {
            return queue_.try_dequeue_bulk(it, max_items);
        }

        bool empty() noexcept
        {
            return (queue_.size_approx() == 0);
        }

    private:
        container_type queue_;
    };

    HPX_CXX_CORE_EXPORT struct concurrentqueue_fifo
    {
        template <typename T>
        struct apply
        {
            using type = moodycamel_fifo_backend<T>;
        };
    };

// LIFO
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT struct lockfree_lifo;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct lockfree_lifo_backend
    {
        using container_type = hpx::lockfree::deque<T,
            hpx::lockfree::caching_freelist_t, hpx::util::aligned_allocator<T>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        static constexpr bool support_bulk_dequeue = false;

        explicit lockfree_lifo_backend(size_type initial_size = 0,
            size_type /* num_thread */ = static_cast<size_type>(-1))
          : queue_(static_cast<std::size_t>(initial_size))
        {
        }

        bool push(const_reference val, bool other_end = false)    //-V659
        {
            if (other_end)
                return queue_.push_right(val);
            return queue_.push_left(val);
        }

        bool push(rvalue_reference val, bool other_end = false)    //-V659
        {
            if (other_end)
                return queue_.push_right(HPX_MOVE(val));
            return queue_.push_left(HPX_MOVE(val));
        }

        bool pop(reference val, bool /* steal */ = true) noexcept
        {
            return queue_.pop_left(val);
        }

        bool empty() noexcept
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    HPX_CXX_CORE_EXPORT struct lockfree_lifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_lifo_backend<T>;
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    // FIFO + stealing at opposite end.
    HPX_CXX_CORE_EXPORT struct lockfree_abp_fifo;
    HPX_CXX_CORE_EXPORT struct lockfree_abp_lifo;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct lockfree_abp_fifo_backend
    {
        using container_type = hpx::lockfree::deque<T,
            hpx::lockfree::caching_freelist_t, hpx::util::aligned_allocator<T>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        static constexpr bool support_bulk_dequeue = false;

        explicit lockfree_abp_fifo_backend(size_type initial_size = 0,
            size_type /* num_thread */ = static_cast<size_type>(-1))
          : queue_(static_cast<std::size_t>(initial_size))
        {
        }

        bool push(const_reference val, bool /*other_end*/ = false)    //-V659
        {
            return queue_.push_left(val);
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)    //-V659
        {
            return queue_.push_left(HPX_MOVE(val));
        }

        bool pop(reference val, bool steal = true) noexcept
        {
            if (steal)
                return queue_.pop_left(val);
            return queue_.pop_right(val);
        }

        bool empty() noexcept
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    HPX_CXX_CORE_EXPORT struct lockfree_abp_fifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_abp_fifo_backend<T>;
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    // LIFO + stealing at opposite end.
    // E.g. ABP (Arora, Blumofe and Plaxton) queuing
    // http://dl.acm.org/citation.cfm?id=277678
    HPX_CXX_CORE_EXPORT template <typename T>
    struct lockfree_abp_lifo_backend
    {
        using container_type = hpx::lockfree::deque<T,
            hpx::lockfree::caching_freelist_t, hpx::util::aligned_allocator<T>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        static constexpr bool support_bulk_dequeue = false;

        explicit lockfree_abp_lifo_backend(size_type initial_size = 0,
            size_type /* num_thread */ = static_cast<size_type>(-1))
          : queue_(static_cast<std::size_t>(initial_size))
        {
        }

        bool push(const_reference val, bool other_end = false)    //-V659
        {
            if (other_end)
                return queue_.push_right(val);
            return queue_.push_left(val);
        }

        bool push(rvalue_reference val, bool other_end = false)    //-V659
        {
            if (other_end)
                return queue_.push_right(HPX_MOVE(val));
            return queue_.push_left(HPX_MOVE(val));
        }

        bool pop(reference val, bool steal = true) noexcept
        {
            if (steal)
                return queue_.pop_right(val);
            return queue_.pop_left(val);
        }

        bool empty() noexcept
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    HPX_CXX_CORE_EXPORT struct lockfree_abp_lifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_abp_lifo_backend<T>;
        };
    };

#endif    // HPX_HAVE_CXX11_STD_ATOMIC_128BIT
}    // namespace hpx::threads::policies
