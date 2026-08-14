//  Copyright (C) 2008-2013 Tim Blechmann
//  Copyright (c) 2022-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/concurrency/cache_line_data.hpp>
#include <hpx/concurrency/detail/copy_payload.hpp>
#include <hpx/concurrency/detail/freelist_stack.hpp>
#include <hpx/concurrency/detail/tagged_ptr.hpp>
#include <hpx/modules/datastructures.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>

namespace hpx::lockfree {

    /**
     * The stack class provides a multi-writer/multi-reader stack, pushing and
     * popping is lock-free,
     *  construction/destruction has to be synchronized. It uses a freelist for
     *  memory management, freed nodes are pushed to the freelist and not
     *  returned to the OS before the stack is destroyed.
     *
     *  \b Policies:
     *
     *  - \c hpx::lockfree::fixed_sized<>, defaults to \c
     *    hpx::lockfree::fixed_sized<false> <br> Can be used to completely
     *    disable dynamic memory allocations during push in order to ensure
     *    lockfree behavior.<br> If the data structure is configured as
     *    fixed-sized, the internal nodes are stored inside an array and they
     *    are addressed by array indexing. This limits the possible size of the
     *    stack to the number of elements that can be addressed by the index
     *    type (usually 2**16-2), but on platforms that lack double-width
     *    compare-and-exchange instructions, this is the best way to achieve
     *    lock-freedom.
     *
     *  - \c hpx::lockfree::capacity<>, optional <br> If this template
     *    argument is passed to the options, the size of the stack is set at
     *    compile-time. <br> It this option implies \c fixed_sized<true>
     *
     *  - \c hpx::lockfree::allocator<>, defaults to \c
     *    hpx::lockfree::allocator<std::allocator<void>> <br> Specifies the
     *    allocator that is used for the internal freelist
     *
     *  \b Requirements:
     *  - T must have a copy constructor
     *
     */
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Allocator = std::allocator<T>, std::size_t Capacity = 0,
        bool IsFixedSize = false>
    class stack
    {
    private:
        static_assert(std::is_copy_constructible_v<T>);

        static constexpr bool has_capacity = Capacity != 0;
        static constexpr bool fixed_sized = IsFixedSize;
        static constexpr bool node_based = !(has_capacity || fixed_sized);
        static constexpr bool compile_time_sized = has_capacity;
        static constexpr std::size_t capacity = Capacity;

        struct node
        {
            template <typename T_,
                typename =
                    std::enable_if_t<!std::is_same_v<std::decay_t<T_>, T>>>
            explicit node(T_&& t) noexcept(
                noexcept(std::is_nothrow_constructible_v<T, T_&&>))
              : v(HPX_FORWARD(T_, t))
            {
            }

            explicit node(T const& val)
              : v(val)
            {
            }
            explicit node(T&& val) noexcept
              : v(HPX_MOVE(val))
            {
            }

            using handle_t = typename detail::select_tagged_handle<node,
                node_based>::handle_type;

            handle_t next;
            T const v;
        };

        using node_allocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<node>;

        using pool_t = typename detail::select_freelist<node, node_allocator,
            compile_time_sized, fixed_sized, capacity>::type;
        using tagged_node_handle = typename pool_t::tagged_node_handle;

        // check compile-time capacity
        static_assert(!has_capacity ||
            capacity - 1 < (std::numeric_limits<std::uint16_t>::max)());

        struct implementation_defined
        {
            using allocator = node_allocator;
            using size_type = std::size_t;
        };

    public:
        stack(stack const&) = delete;
        stack(stack&&) = delete;
        stack& operator=(stack const&) = delete;
        stack& operator=(stack&&) = delete;

        using value_type = T;
        using allocator = typename implementation_defined::allocator;
        using size_type = typename implementation_defined::size_type;

        /**
         * \return true, if implementation is lock-free.
         *
         * \warning It only checks, if the top stack node and the freelist can
         *          be modified in a lock-free manner. On most platforms, the
         *          whole implementation is lock-free, if this is true. Using
         *          c++0x-style atomics, there is no possibility to provide a
         *          completely accurate implementation, because one would need
         *          to test every internal node, which is impossible if further
         *          nodes will be allocated from the operating system.
         */
        [[nodiscard]] constexpr bool is_lock_free() const noexcept
        {
            return tos.is_lock_free() && pool.is_lock_free();
        }

        /**
         * Construct a fixed-sized stack
         *
         *  \pre Must specify a capacity<> argument
         */
        stack()
          : pool(node_allocator(), capacity)
        {
            // Don't use static_assert() here since it will be evaluated when
            // compiling this function and this function may be compiled even
            // when it isn't being used.
            HPX_ASSERT(has_capacity);
            initialize();
        }

        /**
         * Construct a fixed-sized stack with a custom allocator
         *
         *  \pre Must specify a capacity<> argument
         */
        template <typename U>
        explicit stack(typename std::allocator_traits<
            node_allocator>::template rebind_alloc<U> const& alloc)
          : pool(alloc, capacity)
        {
            static_assert(has_capacity);
            initialize();
        }

        /**
         * Construct a fixed-sized stack with a custom allocator
         *
         *  \pre Must specify a capacity<> argument
         */
        explicit stack(allocator const& alloc)
          : pool(alloc, capacity)
        {
            // Don't use static_assert() here since it will be evaluated when
            // compiling this function and this function may be compiled even
            // when it isn't being used.
            HPX_ASSERT(has_capacity);
            initialize();
        }

        /**
         * Construct a variable-sized stack
         *
         *  Allocate n nodes initially for the freelist
         *
         *  \pre Must \b not specify a capacity<> argument
         */
        explicit stack(size_type n)
          : pool(node_allocator(), n)
        {
            // Don't use static_assert() here since it will be evaluated when
            // compiling this function and this function may be compiled even
            // when it isn't being used.
            HPX_ASSERT(!has_capacity);
            initialize();
        }

        /**
         * Construct a variable-sized stack with a custom allocator
         *
         *  Allocate n nodes initially for the freelist
         *
         *  \pre Must \b not specify a capacity<> argument
         */
        template <typename U>
        stack(size_type n,
            typename std::allocator_traits<
                node_allocator>::template rebind_alloc<U> const& alloc)
          : pool(alloc, n)
        {
            static_assert(!has_capacity);
            initialize();
        }

        /**
         * Allocate n nodes for freelist
         *
         *  \pre  only valid if no capacity<> argument given
         *  \note thread-safe, may block if memory allocator blocks
         */
        void reserve(size_type n)
        {
            // Don't use static_assert() here since it will be evaluated
            // when compiling this function and this function may be compiled
            // even when it isn't being used.
            HPX_ASSERT(!has_capacity);
            pool.template reserve<true>(n);
        }

        /**
        * Allocate n nodes for freelist
        *
        *  \pre  only valid if no capacity<> argument given
        *  \note not thread-safe, may block if memory allocator blocks
        */
        void reserve_unsafe(size_type n)
        {
            // Don't use static_assert() here since it will be evaluated
            // when compiling this function and this function may be compiled
            // even when it isn't being used.
            HPX_ASSERT(!has_capacity);
            pool.template reserve<false>(n);
        }

        /**
         * Destroys stack, free all nodes from freelist.
         *
         *  \note not thread-safe
         */
        ~stack()
        {
            detail::consume_noop consume;
            consume_all(consume);
        }

    private:
        void initialize() noexcept
        {
            tos.store(tagged_node_handle(pool.null_handle(), 0));
        }

        void link_nodes_atomic(node* new_top_node, node* end_node) noexcept
        {
            tagged_node_handle old_tos = tos.load(std::memory_order_relaxed);
            for (;;)
            {
                tagged_node_handle new_tos(
                    pool.get_handle(new_top_node), old_tos.get_tag());
                end_node->next = pool.get_handle(old_tos);

                if (tos.compare_exchange_weak(old_tos, new_tos))
                    break;
            }
        }

        void link_nodes_unsafe(node* new_top_node, node* end_node) noexcept
        {
            tagged_node_handle old_tos = tos.load(std::memory_order_relaxed);

            tagged_node_handle new_tos(
                pool.get_handle(new_top_node), old_tos.get_tag());
            end_node->next = pool.get_handle(old_tos);

            tos.store(new_tos, std::memory_order_relaxed);
        }

        template <bool Threadsafe, bool Bounded, typename ConstIterator>
        hpx::tuple<node*, node*> prepare_node_list(
            ConstIterator begin, ConstIterator end, ConstIterator& ret)
        {
            ConstIterator it = begin;
            node* end_node =
                pool.template construct<Threadsafe, Bounded>(*it++);
            if (end_node == nullptr)
            {
                ret = begin;
                return hpx::make_tuple<node*, node*>(nullptr, nullptr);
            }

            node* new_top_node = end_node;
            end_node->next = nullptr;

            try
            {
                // link nodes
                for (; it != end; ++it)
                {
                    node* newnode =
                        pool.template construct<Threadsafe, Bounded>(*it);
                    if (newnode == nullptr)
                        break;
                    newnode->next = new_top_node;
                    new_top_node = newnode;
                }
            }
            catch (...)
            {
                for (node* current_node = new_top_node;
                    current_node != nullptr;)
                {
                    node* next = current_node->next;
                    pool.template destruct<Threadsafe>(current_node);
                    current_node = next;
                }
                throw;
            }

            ret = it;
            return hpx::make_tuple(new_top_node, end_node);
        }

    public:
        /**
         * Pushes object t to the stack.
         *
         * \post object will be pushed to the stack, if internal node can be
         *       allocated
         * \returns true, if the push operation is successful.
         *
         * \note Thread-safe. If internal memory pool is exhausted and the
         *       memory pool is not fixed-sized, a new node will be allocated
         *                    from the OS. This may not be lock-free.
         * \throws if memory allocator throws
         */
        template <typename T_>
        bool push(T_&& t)
        {
            return do_push<false>(HPX_FORWARD(T_, t));
        }

        /**
         * Pushes object t to the stack.
         *
         * \post object will be pushed to the stack, if internal node can be
         *       allocated
         * \returns true, if the push operation is successful.
         *
         * \note Thread-safe and non-blocking. If internal memory pool is
         *       exhausted, the push operation will fail
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
            node* newnode =
                pool.template construct<true, Bounded>(HPX_FORWARD(T_, t));
            if (newnode == nullptr)
                return false;

            link_nodes_atomic(newnode, newnode);
            return true;
        }

        template <bool Bounded, typename ConstIterator>
        ConstIterator do_push(ConstIterator begin, ConstIterator end)
        {
            ConstIterator ret;

            auto top_end_pair =
                prepare_node_list<true, Bounded>(begin, end, ret);
            if (hpx::get<0>(top_end_pair))
            {
                link_nodes_atomic(
                    hpx::get<0>(top_end_pair), hpx::get<1>(top_end_pair));
            }
            return ret;
        }

    public:
        /**
         * Pushes as many objects from the range [begin, end) as freelist node
         * can be allocated.
         *
         * \return iterator to the first element, which has not been pushed
         *
         * \note Operation is applied atomically
         * \note Thread-safe. If internal memory pool is exhausted and the
         *       memory pool is not fixed-sized, a new node will be allocated
         *                    from the OS. This may not be lock-free.
         * \throws if memory allocator throws
         */
        template <typename ConstIterator>
        ConstIterator push(ConstIterator begin, ConstIterator end)
        {
            return do_push<false>(begin, end);
        }

        /**
         * Pushes as many objects from the range [begin, end) as freelist node
         * can be allocated.
         *
         * \return iterator to the first element, which has not been pushed
         *
         * \note Operation is applied atomically
         * \note Thread-safe and non-blocking. If internal memory pool is
         *       exhausted, the push operation will fail
         * \throws if memory allocator throws
         */
        template <typename ConstIterator>
        ConstIterator bounded_push(ConstIterator begin, ConstIterator end)
        {
            return do_push<true>(begin, end);
        }

        /**
         * Pushes object t to the stack.
         *
         * \post object will be pushed to the stack, if internal node can be
         *       allocated
         * \returns true, if the push operation is successful.
         *
         * \note Not thread-safe. If internal memory pool is exhausted and the
         *       memory pool is not fixed-sized, a new node will be allocated
         *       from the OS. This may not be lock-free.
         * \throws if memory allocator throws
         */
        template <typename T_>
        bool unsynchronized_push(T_&& t)
        {
            node* newnode =
                pool.template construct<false, false>(HPX_FORWARD(T_, t));
            if (newnode == nullptr)
                return false;

            link_nodes_unsafe(newnode, newnode);
            return true;
        }

        /**
         * Pushes as many objects from the range [begin, end) as freelist node
         * can be allocated.
         *
         * \return iterator to the first element, which has not been pushed
         *
         * \note Not thread-safe. If internal memory pool is exhausted and the
         *       memory pool is not fixed-sized, a new node will be allocated
         *       from the OS. This may not be lock-free.
         * \throws if memory allocator throws
         */
        template <typename ConstIterator>
        ConstIterator unsynchronized_push(
            ConstIterator begin, ConstIterator end)
        {
            ConstIterator ret;

            auto top_end_pair =
                prepare_node_list<false, false>(begin, end, ret);
            if (hpx::get<0>(top_end_pair))
            {
                link_nodes_unsafe(
                    hpx::get<0>(top_end_pair), hpx::get<1>(top_end_pair));
            }
            return ret;
        }

        /**
         * Pops object from stack.
         *
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if stack was
         *          empty.
         *
         * \note Thread-safe and non-blocking
         *
         */
        bool pop(T& ret) noexcept(
            noexcept(std::is_nothrow_copy_constructible_v<T>))
        {
            return pop<T>(ret);
        }

        /**
         * Pops object from stack.
         *
         * \pre type T must be convertible to U
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if stack was
         *          empty.
         *
         * \note Thread-safe and non-blocking
         */
        template <typename U>
        bool pop(U& ret) noexcept(
            noexcept(std::is_nothrow_constructible_v<U, T>))
        {
            static_assert(std::is_convertible_v<T, U>);
            detail::consume_via_copy<U> consumer(ret);

            return consume_one(consumer);
        }

        /**
         * Pops object from stack.
         *
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if stack was
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
         * Pops object from stack.
         *
         * \pre type T must be convertible to U
         * \post if pop operation is successful, object will be copied to ret.
         * \returns true, if the pop operation is successful, false if stack was
         *          empty.
         *
         * \note Not thread-safe, but non-blocking
         */
        template <typename U>
        bool unsynchronized_pop(U& ret) noexcept(
            noexcept(std::is_nothrow_constructible_v<U, T>))
        {
            static_assert(std::is_convertible_v<T, U>);

            tagged_node_handle old_tos = tos.load(std::memory_order_relaxed);
            node* old_tos_pointer = pool.get_pointer(old_tos);

            if (!pool.get_pointer(old_tos))
                return false;

            node* new_tos_ptr = pool.get_pointer(old_tos_pointer->next);
            tagged_node_handle new_tos(
                pool.get_handle(new_tos_ptr), old_tos.get_next_tag());

            tos.store(new_tos, std::memory_order_relaxed);
            detail::copy_payload(old_tos_pointer->v, ret);
            pool.template destruct<false>(old_tos);
            return true;
        }

        /**
         * consumes one element via a functor
         *
         *  pops one element from the stack and applies the functor on this
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
            tagged_node_handle old_tos = tos.load(std::memory_order_acquire);

            for (;;)
            {
                node* old_tos_pointer = pool.get_pointer(old_tos);
                if (!old_tos_pointer)
                    return false;

                tagged_node_handle new_tos(
                    old_tos_pointer->next, old_tos.get_next_tag());

                if (tos.compare_exchange_weak(old_tos, new_tos))
                {
                    f(old_tos_pointer->v);
                    pool.template destruct<true>(old_tos);
                    return true;
                }
            }
        }

        /**
         * consumes all elements via a functor
         *
         * sequentially pops all elements from the stack and applies the functor
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

        /**
         * consumes all elements via a functor
         *
         * atomically pops all elements from the stack and applies the functor
         * on each object
         *
         * \returns number of elements that are consumed
         *
         * \note Thread-safe and non-blocking, if functor is thread-safe and
         *       non-blocking
         */
        template <typename F>
        std::size_t consume_all_atomic(F&& f)
        {
            std::size_t element_count = 0;
            tagged_node_handle old_tos = tos.load(std::memory_order_acquire);

            for (;;)
            {
                node* old_tos_pointer = pool.get_pointer(old_tos);
                if (!old_tos_pointer)
                    return 0;

                tagged_node_handle new_tos(
                    pool.null_handle(), old_tos.get_next_tag());

                if (tos.compare_exchange_weak(old_tos, new_tos))
                    break;
            }

            tagged_node_handle nodes_to_consume = old_tos;

            for (;;)
            {
                node* node_pointer = pool.get_pointer(nodes_to_consume);
                f(node_pointer->v);
                ++element_count;

                node* next_node = pool.get_pointer(node_pointer->next);
                if (!next_node)
                {
                    pool.template destruct<true>(nodes_to_consume);
                    break;
                }

                tagged_node_handle next(pool.get_handle(next_node),
                    nodes_to_consume.get_next_tag());
                pool.template destruct<true>(nodes_to_consume);
                nodes_to_consume = next;
            }

            return element_count;
        }

        /**
         * consumes all elements via a functor
         *
         * atomically pops all elements from the stack and applies the functor
         * on each object in reversed order
         *
         * \returns number of elements that are consumed
         *
         * \note Thread-safe and non-blocking, if functor is thread-safe and
         *       non-blocking
         */
        template <typename F>
        std::size_t consume_all_atomic_reversed(F&& f)
        {
            std::size_t element_count = 0;
            tagged_node_handle old_tos = tos.load(std::memory_order_acquire);

            for (;;)
            {
                node* old_tos_pointer = pool.get_pointer(old_tos);
                if (!old_tos_pointer)
                    return 0;

                tagged_node_handle new_tos(
                    pool.null_handle(), old_tos.get_next_tag());

                if (tos.compare_exchange_weak(old_tos, new_tos))
                    break;
            }

            tagged_node_handle nodes_to_consume = old_tos;

            node* last_node_pointer = nullptr;
            tagged_node_handle nodes_in_reversed_order;
            for (;;)
            {
                node* node_pointer = pool.get_pointer(nodes_to_consume);
                node* next_node = pool.get_pointer(node_pointer->next);

                node_pointer->next = pool.get_handle(last_node_pointer);
                last_node_pointer = node_pointer;

                if (!next_node)
                {
                    nodes_in_reversed_order = nodes_to_consume;
                    break;
                }

                tagged_node_handle next(pool.get_handle(next_node),
                    nodes_to_consume.get_next_tag());
                nodes_to_consume = next;
            }

            for (;;)
            {
                node* node_pointer = pool.get_pointer(nodes_in_reversed_order);
                f(node_pointer->v);
                ++element_count;

                node* next_node = pool.get_pointer(node_pointer->next);
                if (!next_node)
                {
                    pool.template destruct<true>(nodes_in_reversed_order);
                    break;
                }

                tagged_node_handle next(pool.get_handle(next_node),
                    nodes_in_reversed_order.get_next_tag());
                pool.template destruct<true>(nodes_in_reversed_order);
                nodes_in_reversed_order = next;
            }

            return element_count;
        }

        /**
         * \return true, if stack is empty.
         *
         * \note It only guarantees that at some point during the execution of
         *       the function the stack has been empty. It is rarely practical
         *       to use this value in program logic, because the stack can be
         *       modified by other threads.
         */
        constexpr bool empty() const noexcept
        {
            return pool.get_pointer(tos.load()) == nullptr;
        }

    private:
        util::cache_aligned_data_derived<std::atomic<tagged_node_handle>> tos;

        pool_t pool;
    };

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Allocator = std::allocator<T>>
    class variable_size_stack : public stack<T, Allocator>
    {
        using stack<T, Allocator>::stack;
    };
}    // namespace hpx::lockfree
