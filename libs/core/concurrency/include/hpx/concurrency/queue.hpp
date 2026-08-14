//  Copyright (C) 2008-2013 Tim Blechmann
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  lock-free queue from
//  Michael, M. M. and Scott, M. L.,
//  "simple, fast and practical non-blocking and blocking concurrent queue algorithms"

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/concurrency/cache_line_data.hpp>
#include <hpx/concurrency/detail/copy_payload.hpp>
#include <hpx/concurrency/detail/freelist_stack.hpp>
#include <hpx/concurrency/detail/tagged_ptr.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace hpx::lockfree {
    /**
     * The queue class provides a multi-writer/multi-reader queue, pushing and
     * popping is lock-free,
     *  construction/destruction has to be synchronized. It uses a freelist for
     *  memory management, freed nodes are pushed to the freelist and not
     *  returned to the OS before the queue is destroyed.
     *
     *  \b Policies:
     *  - \ref hpx::lockfree::fixed_sized, defaults to \c
     *    hpx::lockfree::fixed_sized<false> \n Can be used to completely
     *    disable dynamic memory allocations during push in order to ensure
     *    lockfree behavior. \n If the data structure is configured as
     *    fixed-sized, the internal nodes are stored inside an array, and they
     *    are addressed by array indexing. This limits the possible size of the
     *    queue to the number of elements that can be addressed by the index
     *    type (usually 2**16-2), but on platforms that lack double-width
     *    compare-and-exchange instructions, this is the best way to achieve
     *    lock-freedom.
     *
     *  - \ref hpx::lockfree::capacity, optional \n If this template argument
     *    is passed to the options, the size of the queue is set at
     *    compile-time.\n This option implies \c fixed_sized<true>
     *
     *  - \ref hpx::lockfree::allocator, defaults to \c
     *    hpx::lockfree::allocator<std::allocator<void>> \n Specifies the
     *    allocator that is used for the internal freelist
     *
     *  \b Requirements:
     *   - T must have a copy constructor
     *   - T must have a trivial assignment operator
     *   - T must have a trivial destructor
     */
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Allocator = std::allocator<T>, std::size_t Capacity = 0,
        bool IsFixedSize = false>
    class queue
    {
    private:
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);

        static constexpr bool has_capacity = Capacity != 0;
        static constexpr bool fixed_sized = IsFixedSize;
        static constexpr bool node_based = !(has_capacity || fixed_sized);
        static constexpr bool compile_time_sized = has_capacity;

        // the queue uses one dummy node
        static constexpr std::size_t capacity = Capacity + 1;

        struct node;
        struct node_data
        {
            using tagged_node_handle =
                typename detail::select_tagged_handle<node,
                    node_based>::tagged_handle_type;
            using handle_type = typename detail::select_tagged_handle<node,
                node_based>::handle_type;

            template <typename T_>
            node_data(T_&& t, handle_type null_handle) noexcept(
                noexcept(std::is_nothrow_constructible_v<T, T_&&>))
              : data(HPX_FORWARD(T_, t))
            {
                /* increment tag to avoid ABA problem */
                tagged_node_handle old_next =
                    next.load(std::memory_order_relaxed);
                tagged_node_handle new_next(
                    null_handle, old_next.get_next_tag());
                next.store(new_next, std::memory_order_release);
            }

            explicit node_data(handle_type null_handle) noexcept
              : next(tagged_node_handle(null_handle, 0))
              , data()
            {
            }

            node_data() noexcept = default;

            std::atomic<tagged_node_handle> next;
            T data;
        };

        struct node : hpx::util::cache_aligned_data_derived<node_data>
        {
            using hpx::util::cache_aligned_data_derived<
                node_data>::cache_aligned_data_derived;
        };

        using node_allocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<node>;

        using pool_t = typename detail::select_freelist<node, node_allocator,
            compile_time_sized, fixed_sized, capacity>::type;

        using tagged_node_handle = typename pool_t::tagged_node_handle;
        using handle_type = typename detail::select_tagged_handle<node,
            node_based>::handle_type;

        using index_t = typename tagged_node_handle::index_t;

        static constexpr auto null_index_t() noexcept
        {
            if constexpr (std::is_pointer_v<index_t>)
            {
                return nullptr;
            }
            else
            {
                return static_cast<index_t>(0);
            }
        }

        void initialize()
        {
            node* n = pool.template construct<true, false>(pool.null_handle());
            tagged_node_handle dummy_node(pool.get_handle(n), 0);
            head_.store(dummy_node, std::memory_order_relaxed);
            tail_.store(dummy_node, std::memory_order_release);
        }

        struct implementation_defined
        {
            using allocator = node_allocator;
            using size_type = std::size_t;
        };

    public:
        queue(queue const&) = delete;
        queue(queue&&) = delete;
        queue& operator=(queue const&) = delete;
        queue& operator=(queue&&) = delete;

        using value_type = T;
        using allocator = typename implementation_defined::allocator;
        using size_type = typename implementation_defined::size_type;

        /**
         * \return true, if implementation is lock-free.
         *
         * \warning It only checks, if the queue head and tail nodes and the
         *          freelist can be modified in a lock-free manner.
         *       On most platforms, the whole implementation is lock-free, if this
         *       is true. Using c++0x-style atomics, there is no possibility to
         *       provide a completely accurate implementation, because one would
         *       need to test every internal node, which is impossible if further
         *       nodes will be allocated from the operating system.
         */
        [[nodiscard]] constexpr bool is_lock_free() const noexcept
        {
            return head_.is_lock_free() && tail_.is_lock_free() &&
                pool.is_lock_free();
        }

        /** Construct a fixed-sized queue
         *
         *  \pre Must specify a capacity<> argument
         */
        queue()
          : head_(tagged_node_handle(null_index_t(), 0))
          , tail_(tagged_node_handle(null_index_t(), 0))
          , pool(node_allocator(), capacity)
        {
            // Don't use static_assert() here since it will be evaluated when
            // compiling this function and this function may be compiled even
            // when it isn't being used.
            HPX_ASSERT(has_capacity);
            initialize();
        }

        /** Construct a fixed-sized queue with a custom allocator
         *
         *  \pre Must specify a capacity<> argument
         */
        template <typename U>
        explicit queue(typename std::allocator_traits<
            Allocator>::template rebind_alloc<U> const& alloc)
          : head_(tagged_node_handle(null_index_t(), 0))
          , tail_(tagged_node_handle(null_index_t(), 0))
          , pool(alloc, capacity)
        {
            static_assert(has_capacity);
            initialize();
        }

        /**
         * Construct a fixed-sized queue with a custom allocator
         *
         *  \pre Must specify a capacity<> argument
         */
        explicit queue(allocator const& alloc)
          : head_(tagged_node_handle(null_index_t(), 0))
          , tail_(tagged_node_handle(null_index_t(), 0))
          , pool(alloc, capacity)
        {
            // Don't use static_assert() here since it will be evaluated when
            // compiling this function and this function may be compiled even
            // when it isn't being used.
            HPX_ASSERT(has_capacity);
            initialize();
        }

        /**
         * Construct a variable-sized queue
         *
         *  Allocate n nodes initially for the freelist
         *
         *  \pre Must \b not specify a capacity<> argument
         */
        explicit queue(size_type n)
          : head_(tagged_node_handle(null_index_t(), 0))
          , tail_(tagged_node_handle(null_index_t(), 0))
          , pool(node_allocator(), n + 1)
        {
            // Don't use static_assert() here since it will be evaluated when
            // compiling this function and this function may be compiled even
            // when it isn't being used.
            HPX_ASSERT(!has_capacity);
            initialize();
        }

        /**
         * Construct a variable-sized queue with a custom allocator
         *
         *  Allocate n nodes initially for the freelist
         *
         *  \pre Must \b not specify a capacity<> argument
         */
        template <typename U>
        queue(size_type n,
            typename std::allocator_traits<Allocator>::template rebind_alloc<
                U> const& alloc)
          : head_(tagged_node_handle(null_index_t(), 0))
          , tail_(tagged_node_handle(null_index_t(), 0))
          , pool(alloc, n + 1)
        {
            static_assert(!has_capacity);
            initialize();
        }

        /** \copydoc hpx::lockfree::stack::reserve
         */
        void reserve(size_type n)
        {
            pool.template reserve<true>(n);
        }

        /** \copydoc hpx::lockfree::stack::reserve_unsafe
         */
        void reserve_unsafe(size_type n)
        {
            pool.template reserve<false>(n);
        }

        /** Destroys queue, free all nodes from freelist.
         */
        ~queue()
        {
            T dummy;
            while (unsynchronized_pop(dummy))
            {
            }

            pool.template destruct<false>(
                head_.load(std::memory_order_relaxed));
        }

        /**
         * Check if the queue is empty
         *
         * \return true, if the queue is empty, false otherwise
         * \note The result is only accurate, if no other thread modifies the
         *       queue. Therefore, it is rarely practical to use this value in
         *       program logic.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return pool.get_handle(head_.load(std::memory_order_acquire)) ==
                pool.get_handle(tail_.load(std::memory_order_acquire));
        }

        /**
         * Pushes object t to the queue.
         *
         * \post object will be pushed to the queue, if internal node can be
         *       allocated
         * \returns true, if the push operation is successful.
         *
         * \note Thread-safe. If internal memory pool is exhausted and the
         *       memory pool is not fixed-sized, a new node will be allocated
         *                    from the OS. This may not be lock-free.
         */
        template <typename T_>
        bool push(T_&& t)
        {
            return do_push<false>(HPX_FORWARD(T_, t));
        }

        /**
         * Pushes object t to the queue.
         *
         * \post object will be pushed to the queue, if internal node can be
         *       allocated
         * \returns true, if the push operation is successful.
         *
         * \note Thread-safe and non-blocking. If internal memory pool is
         *       exhausted, operation will fail
         * \throws if memory allocator throws
         */
        template <typename T_>
        bool bounded_push(T_&& t)
        {
            return do_push<true>(HPX_FORWARD(T_, t));
        }

    private:
        template <bool Bounded, typename T_>
        bool do_push(T_&& t)
        {
            node* n = pool.template construct<true, Bounded>(
                HPX_FORWARD(T_, t), pool.null_handle());
            handle_type node_handle = pool.get_handle(n);

            if (n == nullptr)
                return false;

            for (;;)
            {
                tagged_node_handle tail = tail_.load(std::memory_order_acquire);
                node* tail_node = pool.get_pointer(tail);
                tagged_node_handle next =
                    tail_node->next.load(std::memory_order_acquire);
                node* next_ptr = pool.get_pointer(next);

                tagged_node_handle tail2 =
                    tail_.load(std::memory_order_acquire);
                if (HPX_LIKELY(tail == tail2))
                {
                    if (next_ptr == nullptr)
                    {
                        tagged_node_handle new_tail_next(
                            node_handle, next.get_next_tag());
                        if (tail_node->next.compare_exchange_weak(
                                next, new_tail_next))
                        {
                            tagged_node_handle new_tail(
                                node_handle, tail.get_next_tag());
                            tail_.compare_exchange_strong(tail, new_tail);
                            return true;
                        }
                    }
                    else
                    {
                        tagged_node_handle new_tail(
                            pool.get_handle(next_ptr), tail.get_next_tag());
                        tail_.compare_exchange_strong(tail, new_tail);
                    }
                }
            }
        }

    public:
        /**
         * Pushes object t to the queue.
         *
         * \post object will be pushed to the queue, if internal node can be
         *       allocated
         * \returns true, if the push operation is successful.
         *
         * \note Not Thread-safe. If internal memory pool is exhausted and the
         *       memory pool is not fixed-sized, a new node will be allocated
         *       from the OS. This may not be lock-free.
         * \throws if memory allocator throws
         */
        template <typename T_>
        bool unsynchronized_push(T_&& t)
        {
            node* n = pool.template construct<false, false>(
                HPX_FORWARD(T_, t), pool.null_handle());

            if (n == nullptr)
                return false;

            for (;;)
            {
                tagged_node_handle tail = tail_.load(std::memory_order_relaxed);
                tagged_node_handle next =
                    tail->next.load(std::memory_order_relaxed);
                node* next_ptr = next.get_ptr();

                if (next_ptr == nullptr)
                {
                    tail->next.store(tagged_node_handle(n, next.get_next_tag()),
                        std::memory_order_relaxed);
                    tail_.store(tagged_node_handle(n, tail.get_next_tag()),
                        std::memory_order_relaxed);
                    return true;
                }
                else
                {
                    tail_.store(
                        tagged_node_handle(next_ptr, tail.get_next_tag()),
                        std::memory_order_relaxed);
                }
            }
        }

        /**
         * Pops object from queue.
         *
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if queue was
         *          empty.
         *
         * \note Thread-safe and non-blocking
         */
        bool pop(T& ret) noexcept(
            noexcept(std::is_nothrow_copy_constructible_v<T>))
        {
            return pop<T>(ret);
        }

        /**
         * Pops object from queue.
         *
         * \pre type U must be constructible by T and copyable, or T must be
         *      convertible to U
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if queue was
         *          empty.
         *
         * \note Thread-safe and non-blocking
         */
        template <typename U>
        bool pop(U& ret) noexcept(
            noexcept(std::is_nothrow_constructible_v<U, T>))
        {
            for (;;)
            {
                tagged_node_handle head = head_.load(std::memory_order_acquire);
                node* head_ptr = pool.get_pointer(head);

                tagged_node_handle tail = tail_.load(std::memory_order_acquire);
                tagged_node_handle next =
                    head_ptr->next.load(std::memory_order_acquire);
                node* next_ptr = pool.get_pointer(next);

                tagged_node_handle head2 =
                    head_.load(std::memory_order_acquire);
                if (HPX_LIKELY(head == head2))
                {
                    if (pool.get_handle(head) == pool.get_handle(tail))
                    {
                        if (next_ptr == nullptr)
                            return false;

                        tagged_node_handle new_tail(
                            pool.get_handle(next), tail.get_next_tag());
                        tail_.compare_exchange_strong(tail, new_tail);
                    }
                    else
                    {
                        if (next_ptr == nullptr)
                        {
                            // this check is not part of the original algorithm
                            // as published by michael and scott
                            //
                            // however we reuse the tagged_ptr part for the
                            // freelist and clear the next part during node
                            // allocation. we can observe a null-pointer here.
                            continue;
                        }

                        detail::copy_payload(next_ptr->data, ret);

                        tagged_node_handle new_head(
                            pool.get_handle(next), head.get_next_tag());
                        if (head_.compare_exchange_weak(head, new_head))
                        {
                            pool.template destruct<true>(head);
                            return true;
                        }
                    }
                }
            }
        }

        /**
         * Pops object from queue.
         *
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if queue was
         *          empty.
         *
         * \note Not thread-safe, but non-blocking
         *
         */
        bool unsynchronized_pop(T& ret) noexcept(
            noexcept(std::is_nothrow_copy_constructible_v<T>))
        {
            return unsynchronized_pop<T>(ret);
        }

        /**
         * Pops object from queue.
         *
         * \pre type U must be constructible by T and copyable, or T must be
         *      convertible to U
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if queue was
         *          empty.
         *
         * \note Not thread-safe, but non-blocking
         *
         */
        template <typename U>
        bool unsynchronized_pop(U& ret) noexcept(
            noexcept(std::is_nothrow_constructible_v<U, T>))
        {
            for (;;)
            {
                tagged_node_handle head = head_.load(std::memory_order_relaxed);
                node* head_ptr = pool.get_pointer(head);
                tagged_node_handle tail = tail_.load(std::memory_order_relaxed);
                tagged_node_handle next =
                    head_ptr->next.load(std::memory_order_relaxed);
                node* next_ptr = pool.get_pointer(next);

                if (pool.get_handle(head) == pool.get_handle(tail))
                {
                    if (next_ptr == nullptr)
                        return false;

                    tagged_node_handle new_tail(
                        pool.get_handle(next), tail.get_next_tag());
                    tail_.store(new_tail);
                }
                else
                {
                    if (next_ptr == nullptr)
                    {
                        // this check is not part of the original algorithm as
                        // published by michael and scott
                        //
                        // however we reuse the tagged_ptr part for the freelist
                        // and clear the next part during node allocation. we
                        // can observe a null-pointer here.
                        continue;
                    }

                    detail::copy_payload(next_ptr->data, ret);
                    tagged_node_handle new_head(
                        pool.get_handle(next), head.get_next_tag());
                    head_.store(new_head);
                    pool.template destruct<false>(head);
                    return true;
                }
            }
        }

        /**
         * consumes one element via a functor
         *
         *  pops one element from the queue and applies the functor on this
         *  object
         *
         * \returns true, if one element was consumed
         *
         * \note Thread-safe and non-blocking, if functor is thread-safe and
         *       non-blocking
         */
        template <typename F>
        bool consume_one(F&& f)
        {
            T element;
            bool const success = pop(element);
            if (success)
                f(element);

            return success;
        }

        /**
         * consumes all elements via a functor
         *
         * sequentially pops all elements from the queue and applies the functor
         * on each object
         *
         * \returns number of elements that are consumed
         *
         * \note Thread-safe and non-blocking, if functor is thread-safe and
         *       non-blocking
         */
        template <typename F>
        std::size_t consume_all(F&& f)
        {
            std::size_t element_count = 0;
            while (consume_one(f))
                ++element_count;

            return element_count;
        }

    private:
        util::cache_aligned_data_derived<std::atomic<tagged_node_handle>> head_;
        util::cache_aligned_data_derived<std::atomic<tagged_node_handle>> tail_;

        pool_t pool;
    };
}    // namespace hpx::lockfree
