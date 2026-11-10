//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/execution_base/stdexec_forward.hpp>
#include <hpx/type_support/coroutines_support.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <variant>

namespace hpx::execution::experimental {

    template <typename T = void, typename TaskEnv = empty_env>
    class [[nodiscard]] task;

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Memory block for P3552 coroutine frame allocator alignment
        struct alignas(alignof(std::max_align_t)) memblock
        {
            std::byte storage[alignof(std::max_align_t)];
        };

        constexpr std::size_t divmod(
            std::size_t total_size, std::size_t chunk_size) noexcept
        {
            return (total_size / chunk_size) +
                (total_size % chunk_size != 0 ? 1 : 0);
        }

        ///////////////////////////////////////////////////////////////////////
        // Type-erased allocator deallocation interface
        struct any_alloc_base
        {
            virtual ~any_alloc_base() = default;
            virtual void deallocate(void* ptr, std::size_t bytes) noexcept = 0;
        };

        template <typename Alloc>
        struct any_alloc final : any_alloc_base
        {
            using value_type =
                typename std::allocator_traits<Alloc>::value_type;
            static_assert(std::is_same_v<value_type, memblock>);

            explicit any_alloc(Alloc alloc)
              : alloc_(HPX_MOVE(alloc))
            {
            }

            void deallocate(void* ptr, std::size_t bytes) noexcept override
            {
                std::size_t const promise_blocks =
                    divmod(bytes, sizeof(memblock));
                static constexpr std::size_t alloc_blocks =
                    divmod(sizeof(any_alloc<Alloc>), sizeof(memblock));

                auto alloc = HPX_MOVE(alloc_);
                std::destroy_at(this);
                std::allocator_traits<Alloc>::deallocate(alloc,
                    static_cast<memblock*>(ptr),
                    promise_blocks + alloc_blocks);
            }

            Alloc alloc_;
        };

        ///////////////////////////////////////////////////////////////////////
        // promise_base: return value storage specialization
        template <typename T>
        struct task_promise_base
        {
            void return_value(T val)
            {
                data_.template emplace<1>(HPX_MOVE(val));
            }

            std::variant<std::monostate, T, std::exception_ptr> data_{};
        };

        template <>
        struct task_promise_base<void>
        {
            struct void_type
            {
            };

            void return_void() noexcept
            {
                data_.template emplace<1>(void_type{});
            }

            std::variant<std::monostate, void_type, std::exception_ptr> data_{};
        };

        ///////////////////////////////////////////////////////////////////////
        // task_promise: C++26 P3552 coroutine promise type
        template <typename T, typename TaskEnv>
        struct task_promise
          : task_promise_base<T>
          , hpx::execution::experimental::with_awaitable_senders<
                task_promise<T, TaskEnv>>
        {
            task<T, TaskEnv> get_return_object() noexcept;

            hpx::suspend_always initial_suspend() noexcept
            {
                return {};
            }

            hpx::suspend_always final_suspend() noexcept
            {
                return {};
            }

            void unhandled_exception() noexcept
            {
                this->data_.template emplace<2>(std::current_exception());
            }

            // P3552 Allocator Support
            static void* operator new(std::size_t bytes)
            {
                return operator new(
                    bytes, std::allocator_arg, std::allocator<std::byte>{});
            }

            static void operator delete(void* ptr, std::size_t bytes) noexcept
            {
                std::size_t const promise_blocks =
                    divmod(bytes, sizeof(memblock));
                void* const alloc_loc =
                    static_cast<memblock*>(ptr) + promise_blocks;
                auto* alloc = static_cast<any_alloc_base*>(alloc_loc);
                alloc->deallocate(ptr, bytes);
            }

            template <typename Alloc, typename... Args>
            static void* operator new(std::size_t bytes, std::allocator_arg_t,
                Alloc const& alloc, Args const&...)
            {
                using palloc_t = typename std::allocator_traits<
                    Alloc>::template rebind_alloc<memblock>;
                using pointer_t =
                    typename std::allocator_traits<palloc_t>::pointer;
                static_assert(std::is_pointer_v<pointer_t>,
                    "Allocator pointer type must be a raw pointer");

                static constexpr std::size_t alloc_blocks =
                    divmod(sizeof(any_alloc<palloc_t>), sizeof(memblock));
                std::size_t const promise_blocks =
                    divmod(bytes, sizeof(memblock));

                palloc_t palloc(alloc);
                auto* const ptr = std::allocator_traits<palloc_t>::allocate(
                    palloc, promise_blocks + alloc_blocks);

                void* const alloc_loc = ptr + promise_blocks;
                std::construct_at(
                    static_cast<any_alloc<palloc_t>*>(alloc_loc),
                    HPX_MOVE(palloc));
                return ptr;
            }

            template <typename Alloc, typename... Args>
            static void operator delete(void* ptr, std::size_t bytes,
                std::allocator_arg_t, Alloc const&, Args const&...) noexcept
            {
                operator delete(ptr, bytes);
            }
        };

    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Production C++26 task<T> conforming to P3552
    template <typename T, typename TaskEnv>
    class [[nodiscard]] task
    {
    public:
        using promise_type = detail::task_promise<T, TaskEnv>;

        task() noexcept = default;

        explicit task(hpx::coroutine_handle<promise_type> coro) noexcept
          : coro_(coro)
        {
        }

        task(task&& other) noexcept
          : coro_(std::exchange(other.coro_, {}))
        {
        }

        task& operator=(task&&) = delete;
        task(task const&) = delete;
        task& operator=(task const&) = delete;

        ~task()
        {
            if (coro_)
            {
                coro_.destroy();
            }
        }

    private:
        hpx::coroutine_handle<promise_type> coro_{};
    };

    namespace detail {
        template <typename T, typename TaskEnv>
        task<T, TaskEnv> task_promise<T, TaskEnv>::get_return_object() noexcept
        {
            return task<T, TaskEnv>(
                hpx::coroutine_handle<task_promise>::from_promise(*this));
        }
    }    // namespace detail

}    // namespace hpx::execution::experimental

namespace hpx::experimental {
    using hpx::execution::experimental::task;
}

#endif    // HPX_HAVE_CXX20_COROUTINES

