//  Copyright (C) 2008-2016 Tim Blechmann
//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  lock-free freelist

#pragma once

#include <cstring>
#include <limits>
#include <memory>

#include <hpx/config.hpp>
#include <hpx/concurrency/detail/tagged_ptr.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace hpx::lockfree::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Alloc = std::allocator<T>>
    class freelist_stack : Alloc
    {
        struct freelist_node
        {
            tagged_ptr<freelist_node> next;
        };

        using tagged_node_ptr = tagged_ptr<freelist_node>;

    public:
        using index_t = T*;
        using tagged_node_handle = tagged_ptr<T>;

        template <typename Allocator>
        explicit freelist_stack(Allocator const& alloc, std::size_t n = 0)
          : Alloc(alloc)
          , pool_(tagged_node_ptr(nullptr))
        {
            reserve<false>(n);
        }

        template <bool ThreadSafe>
        void reserve(std::size_t count)
        {
            for (std::size_t i = 0; i != count; ++i)
            {
                T* node = Alloc::allocate(1);
                std::memset(static_cast<void*>(node), 0, sizeof(T));
                deallocate<ThreadSafe>(node);
            }
        }

        template <bool ThreadSafe, bool Bounded, typename... Ts>
        T* construct(Ts&&... ts)
        {
            T* node = allocate<ThreadSafe, Bounded>();
            if (node)
                new (node) T(HPX_FORWARD(Ts, ts)...);
            return node;
        }

        template <bool ThreadSafe>
        void destruct(tagged_node_handle const& tagged_ptr) noexcept
        {
            T* n = tagged_ptr.get_ptr();
            std::destroy_at(n);
            deallocate<ThreadSafe>(n);
        }

        template <bool ThreadSafe>
        void destruct(T* n) noexcept
        {
            std::destroy_at(n);
            deallocate<ThreadSafe>(n);
        }

        ~freelist_stack()
        {
            tagged_node_ptr current = pool_.load();

            while (current)
            {
                freelist_node* current_ptr = current.get_ptr();
                if (current_ptr)
                    current = current_ptr->next;

                // NOLINTNEXTLINE(bugprone-bitwise-pointer-cast)
                Alloc::deallocate(hpx::bit_cast<T*>(current_ptr), 1);
            }
        }

        constexpr bool is_lock_free() const noexcept
        {
            return pool_.is_lock_free();
        }

        constexpr T* get_handle(T* pointer) const noexcept
        {
            return pointer;
        }

        constexpr T* get_handle(tagged_node_handle const& handle) const noexcept
        {
            return get_pointer(handle);
        }

        T* get_pointer(tagged_node_handle const& tptr) const noexcept
        {
            return tptr.get_ptr();
        }

        constexpr T* get_pointer(T* pointer) const noexcept
        {
            return pointer;
        }

        constexpr T* null_handle() const noexcept
        {
            return nullptr;
        }

    protected:    // allow use from subclasses
        template <bool ThreadSafe, bool Bounded>
        T* allocate()
        {
            if constexpr (ThreadSafe)
            {
                return allocate_impl<Bounded>();
            }
            else
            {
                return allocate_impl_unsafe<Bounded>();
            }
        }

    private:
        template <bool Bounded>
        T* allocate_impl()
        {
            tagged_node_ptr old_pool = pool_.load(std::memory_order_acquire);

            for (;;)
            {
                if (!old_pool.get_ptr())
                {
                    if constexpr (!Bounded)
                    {
                        T* ptr = Alloc::allocate(1);
                        std::memset(static_cast<void*>(ptr), 0, sizeof(T));
                        return ptr;
                    }
                    else
                    {
                        return nullptr;
                    }
                }

                freelist_node* new_pool_ptr = old_pool->next.get_ptr();
                tagged_node_ptr new_pool(new_pool_ptr, old_pool.get_next_tag());

                if (pool_.compare_exchange_weak(old_pool, new_pool))
                {
                    void* ptr = old_pool.get_ptr();
                    return static_cast<T*>(ptr);
                }
            }
        }

        template <bool Bounded>
        T* allocate_impl_unsafe()
        {
            tagged_node_ptr old_pool = pool_.load(std::memory_order_relaxed);

            if (!old_pool.get_ptr())
            {
                if constexpr (!Bounded)
                {
                    T* ptr = Alloc::allocate(1);
                    std::memset(static_cast<void*>(ptr), 0, sizeof(T));
                    return ptr;
                }
                else
                {
                    return nullptr;
                }
            }

            freelist_node* new_pool_ptr = old_pool->next.get_ptr();
            tagged_node_ptr new_pool(new_pool_ptr, old_pool.get_next_tag());

            pool_.store(new_pool, std::memory_order_relaxed);
            void* ptr = old_pool.get_ptr();
            return static_cast<T*>(ptr);
        }

    protected:
        template <bool ThreadSafe>
        void deallocate(T* n) noexcept
        {
            if constexpr (ThreadSafe)
            {
                deallocate_impl(n);
            }
            else
            {
                deallocate_impl_unsafe(n);
            }
        }

    private:
        void deallocate_impl(T* n) noexcept
        {
            void* node = n;
            tagged_node_ptr old_pool = pool_.load(std::memory_order_acquire);
            auto* new_pool_ptr = static_cast<freelist_node*>(node);

            for (;;)
            {
                tagged_node_ptr new_pool(new_pool_ptr, old_pool.get_tag());
                new_pool->next.set_ptr(old_pool.get_ptr());

                if (pool_.compare_exchange_weak(old_pool, new_pool))
                    return;
            }
        }

        void deallocate_impl_unsafe(T* n) noexcept
        {
            void* node = n;
            tagged_node_ptr old_pool = pool_.load(std::memory_order_relaxed);
            auto* new_pool_ptr = static_cast<freelist_node*>(node);

            tagged_node_ptr new_pool(new_pool_ptr, old_pool.get_tag());
            new_pool->next.set_ptr(old_pool.get_ptr());

            pool_.store(new_pool, std::memory_order_relaxed);
        }

        std::atomic<tagged_node_ptr> pool_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class tagged_index_data
    {
    public:
        using tag_t = std::uint16_t;
        using index_t = std::uint16_t;

        // Variable 'hpx::lockfree::detail::tagged_index_data::tag' is uninitialized
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

        /** uninitialized constructor */
        tagged_index_data() noexcept    //-V832 //-V730
        {
        }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

        /** copy constructor */
        tagged_index_data(tagged_index_data const& rhs) = default;

        explicit constexpr tagged_index_data(index_t i, tag_t t = 0) noexcept
          : index(i)
          , tag(t)
        {
        }

        /** index access */
        /* @{ */
        constexpr index_t get_index() const noexcept
        {
            return index;
        }

        void set_index(index_t i) noexcept
        {
            index = i;
        }
        /* @} */

        /** tag access */
        /* @{ */
        constexpr tag_t get_tag() const noexcept
        {
            return tag;
        }

        constexpr tag_t get_next_tag() const noexcept
        {
            tag_t const next =
                (get_tag() + 1u) & (std::numeric_limits<tag_t>::max)();
            return next;
        }

        void set_tag(tag_t t) noexcept
        {
            tag = t;
        }
        /* @} */

        friend constexpr bool operator==(
            tagged_index_data const& lhs, tagged_index_data const& rhs) noexcept
        {
            return (lhs.index == rhs.index) && (lhs.tag == rhs.tag);
        }

        friend constexpr bool operator!=(
            tagged_index_data const& lhs, tagged_index_data const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

    protected:
        index_t index;
        tag_t tag;
    };

    HPX_CXX_CORE_EXPORT using tagged_index = tagged_index_data;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, std::size_t Size>
    struct compiletime_sized_freelist_storage_data
    {
        // array-based freelists only support a 16bit address space.
        static_assert(Size < 65536);

        static constexpr std::size_t array_size =
            Size * sizeof(T) + hpx::threads::get_cache_line_size();
        std::array<char, array_size> data;

        // unused ... only for API purposes
        template <typename Allocator>
        compiletime_sized_freelist_storage_data(
            Allocator const& /* alloc */, std::size_t /* count */)
        {
            data.fill(0);
        }

        T* nodes() const noexcept
        {
            char* data_pointer = const_cast<char*>(data.data());
            return reinterpret_cast<T*>(util::align_up(
                data_pointer, hpx::threads::get_cache_line_size()));
        }

        static constexpr std::size_t node_count() noexcept
        {
            return Size;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t Size>
    using compiletime_sized_freelist_storage = util::cache_aligned_data_derived<
        compiletime_sized_freelist_storage_data<T, Size>>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Alloc = std::allocator<T>>
    struct runtime_sized_freelist_storage
      : hpx::util::aligned_allocator<T, Alloc>
    {
        using allocator_type = hpx::util::aligned_allocator<T, Alloc>;

        T* nodes_;
        std::size_t node_count_;

        template <typename Allocator>
        runtime_sized_freelist_storage(
            Allocator const& alloc, std::size_t count)
          : allocator_type(alloc)
          , node_count_(count)
        {
            if (count > 65535)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "runtime_sized_freelist_storage::runtime_sized_freelist_"
                    "storage",
                    "hpx::concurrency: freelist size is limited to a maximum "
                    "of 65535 objects");
            }
            nodes_ = allocator_type::allocate(count);
            std::memset(static_cast<void*>(nodes_), 0, sizeof(T) * count);
        }

        ~runtime_sized_freelist_storage()
        {
            allocator_type::deallocate(nodes_, node_count_);
        }

        constexpr T* nodes() const noexcept
        {
            return nodes_;
        }

        constexpr std::size_t node_count() const noexcept
        {
            return node_count_;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T,
        typename NodeStorage = runtime_sized_freelist_storage<T>>
    class fixed_size_freelist : NodeStorage
    {
        struct freelist_node
        {
            tagged_index next;
        };

        void initialize()
        {
            T* nodes = NodeStorage::nodes();
            for (std::size_t i = 0; i != NodeStorage::node_count(); ++i)
            {
                tagged_index* next_index =
                    reinterpret_cast<tagged_index*>(nodes + i);
                next_index->set_index(null_handle());
                deallocate<false>(static_cast<index_t>(i));
            }
        }

    public:
        using tagged_node_handle = tagged_index;
        using index_t = tagged_index::index_t;

        template <typename Allocator>
        fixed_size_freelist(Allocator const& alloc, std::size_t count)
          : NodeStorage(alloc, count)
          , pool_(tagged_index(static_cast<index_t>(count), 0))
        {
            initialize();
        }

        fixed_size_freelist()
          : pool_(tagged_index(NodeStorage::node_count(), 0))
        {
            initialize();
        }

        template <bool ThreadSafe, bool Bounded, typename... Ts>
        T* construct(Ts&&... ts)
        {
            index_t node_index = allocate<ThreadSafe>();
            if (node_index == null_handle())
                return nullptr;

            T* node = NodeStorage::nodes() + node_index;
            new (node) T(HPX_FORWARD(Ts, ts)...);
            return node;
        }

        template <bool ThreadSafe>
        void destruct(tagged_node_handle tagged_index)
        {
            index_t index = tagged_index.get_index();
            T* n = NodeStorage::nodes() + index;
            std::destroy_at(n);
            deallocate<ThreadSafe>(index);
        }

        template <bool ThreadSafe>
        void destruct(T* n)
        {
            std::destroy_at(n);
            deallocate<ThreadSafe>(
                static_cast<index_t>(n - NodeStorage::nodes()));
        }

        constexpr bool is_lock_free() const noexcept
        {
            return pool_.is_lock_free();
        }

        constexpr index_t null_handle() const noexcept
        {
            return static_cast<index_t>(NodeStorage::node_count());
        }

        constexpr index_t get_handle(T* pointer) const noexcept
        {
            if (pointer == nullptr)
                return null_handle();

            return static_cast<index_t>(pointer - NodeStorage::nodes());
        }

        constexpr index_t get_handle(
            tagged_node_handle const& handle) const noexcept
        {
            return handle.get_index();
        }

        constexpr T* get_pointer(tagged_node_handle const& tptr) const noexcept
        {
            return get_pointer(tptr.get_index());
        }

        constexpr T* get_pointer(index_t index) const noexcept
        {
            if (index == null_handle())
                return nullptr;

            return NodeStorage::nodes() + index;
        }

        constexpr T* get_pointer(T* ptr) const noexcept
        {
            return ptr;
        }

    protected:    // allow use from subclasses
        template <bool ThreadSafe>
        index_t allocate()
        {
            if constexpr (ThreadSafe)
            {
                return allocate_impl();
            }
            else
            {
                return allocate_impl_unsafe();
            }
        }

    private:
        index_t allocate_impl()
        {
            tagged_index old_pool = pool_.load(std::memory_order_acquire);

            for (;;)
            {
                index_t index = old_pool.get_index();
                if (index == null_handle())
                    return index;

                T* old_node = NodeStorage::nodes() + index;
                tagged_index const* next_index =
                    reinterpret_cast<tagged_index*>(old_node);

                tagged_index const new_pool(
                    next_index->get_index(), old_pool.get_next_tag());

                if (pool_.compare_exchange_weak(old_pool, new_pool))
                    return old_pool.get_index();
            }
        }

        index_t allocate_impl_unsafe()
        {
            tagged_index const old_pool = pool_.load(std::memory_order_acquire);

            index_t index = old_pool.get_index();
            if (index == null_handle())
                return index;

            T* old_node = NodeStorage::nodes() + index;
            tagged_index const* next_index =
                reinterpret_cast<tagged_index*>(old_node);

            tagged_index const new_pool(
                next_index->get_index(), old_pool.get_next_tag());

            pool_.store(new_pool, std::memory_order_relaxed);
            return old_pool.get_index();
        }

        template <bool ThreadSafe>
        void deallocate(index_t index) noexcept
        {
            if constexpr (ThreadSafe)
            {
                deallocate_impl(index);
            }
            else
            {
                deallocate_impl_unsafe(index);
            }
        }

        void deallocate_impl(index_t index) noexcept
        {
            freelist_node* new_pool_node =
                reinterpret_cast<freelist_node*>(NodeStorage::nodes() + index);
            tagged_index old_pool = pool_.load(std::memory_order_acquire);

            for (;;)
            {
                tagged_index const new_pool(index, old_pool.get_tag());
                new_pool_node->next.set_index(old_pool.get_index());

                if (pool_.compare_exchange_weak(old_pool, new_pool))
                    return;
            }
        }

        void deallocate_impl_unsafe(index_t index) noexcept
        {
            freelist_node* new_pool_node =
                reinterpret_cast<freelist_node*>(NodeStorage::nodes() + index);
            tagged_index const old_pool = pool_.load(std::memory_order_acquire);

            tagged_index const new_pool(index, old_pool.get_tag());
            new_pool_node->next.set_index(old_pool.get_index());

            pool_.store(new_pool);
        }

        util::cache_aligned_data_derived<std::atomic<tagged_index>> pool_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc,
        bool IsCompileTimeSized, bool IsFixedSize, std::size_t Capacity>
    struct select_freelist
    {
        using fixed_sized_storage_type = std::conditional_t<IsCompileTimeSized,
            compiletime_sized_freelist_storage<T, Capacity>,
            runtime_sized_freelist_storage<T, Alloc>>;

        using type = std::conditional_t<IsCompileTimeSized || IsFixedSize,
            fixed_size_freelist<T, fixed_sized_storage_type>,
            freelist_stack<T, Alloc>>;
    };

    HPX_CXX_CORE_EXPORT template <typename T, bool IsNodeBased>
    struct select_tagged_handle
    {
        using tagged_handle_type =
            std::conditional_t<IsNodeBased, tagged_ptr<T>, tagged_index>;

        using handle_type =
            std::conditional_t<IsNodeBased, T*, tagged_index::index_t>;
    };
}    // namespace hpx::lockfree::detail
