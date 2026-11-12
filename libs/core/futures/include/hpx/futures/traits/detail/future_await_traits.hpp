//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/futures/detail/future_data.hpp>
#include <hpx/futures/traits/future_access.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#if defined(__has_include)
#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    template <typename Promise = void>
    using coroutine_handle = hpx::coroutine_handle<Promise>;
    using suspend_never = hpx::suspend_never;

    ///////////////////////////////////////////////////////////////////////////
    // this was removed from the TS, so we define our own
    struct suspend_if
    {
        bool is_ready_;

        constexpr explicit suspend_if(bool cond) noexcept
          : is_ready_(!cond)
        {
        }

        [[nodiscard]] constexpr bool await_ready() const noexcept
        {
            return is_ready_;
        }

        static constexpr void await_suspend(coroutine_handle<>) noexcept {}
        static constexpr void await_resume() noexcept {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Allow using co_await with an expression which evaluates to
    // hpx::future<T>.
    template <typename T>
    HPX_FORCEINLINE bool await_ready(hpx::future<T> const& f) noexcept
    {
        return f.is_ready();
    }

    HPX_HAS_MEMBER_XXX_TRAIT_DEF(set_exception);

    template <typename T, typename Promise>
    HPX_FORCEINLINE void await_suspend(
        hpx::future<T>& f, coroutine_handle<Promise> rh)
        requires(has_set_exception_v<Promise>)
    {
        // f.then([=](future<T> result) {});
        auto st = traits::detail::get_shared_state(f);
        st->set_on_completed([st, rh]() mutable {
            if (st->has_exception())
            {
                rh.promise().set_exception(st->get_exception_ptr());
            }
            rh();
        });
    }

    template <typename T>
    HPX_FORCEINLINE T await_resume(hpx::future<T>& f)
    {
        return f.get();
    }

    // Allow wrapped futures to be unwrapped, if possible.
    template <typename T>
    HPX_FORCEINLINE T await_resume(hpx::future<hpx::future<T>>& f)
    {
        return f.get().get();
    }

    template <typename T>
    HPX_FORCEINLINE T await_resume(hpx::future<hpx::shared_future<T>>& f)
    {
        return f.get().get();
    }

    // Allow using co_await with an expression which evaluates to
    // hpx::shared_future<T>.
    template <typename T>
    HPX_FORCEINLINE bool await_ready(hpx::shared_future<T> const& f) noexcept
    {
        return f.is_ready();
    }

    template <typename T, typename Promise>
    HPX_FORCEINLINE void await_suspend(
        hpx::shared_future<T>& f, coroutine_handle<Promise> rh)
        requires(has_set_exception_v<Promise>)
    {
        // f.then([=](shared_future<T> result) {})
        auto st = traits::detail::get_shared_state(f);
        st->set_on_completed([st, rh]() mutable {
            if (st->has_exception())
            {
                rh.promise().set_exception(st->get_exception_ptr());
            }
            rh();
        });
    }

    template <typename T>
    HPX_FORCEINLINE T await_resume(hpx::shared_future<T>& f)
    {
        return f.get();
    }

    ///////////////////////////////////////////////////////////////////////////
    // derive from future shared state as this will be combined with the
    // necessary stack frame for the resumable function
    template <typename T, typename Derived>
    struct coroutine_promise_base : hpx::lcos::detail::future_data<T>
    {
        using base_type = hpx::lcos::detail::future_data<T>;
        using init_no_addref = typename base_type::init_no_addref;

        using allocator_type = hpx::util::internal_allocator<char>;

        // the shared state is held alive by the coroutine

        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        coroutine_promise_base() noexcept
          : base_type(init_no_addref{})
        {
        }

        hpx::future<T> get_return_object()
        {
            hpx::intrusive_ptr<Derived> shared_state(
                static_cast<Derived*>(this));
            return hpx::traits::future_access<hpx::future<T>>::create(
                HPX_MOVE(shared_state));
        }

        static constexpr suspend_never initial_suspend() noexcept
        {
            return suspend_never{};
        }

        suspend_if final_suspend() noexcept
        {
            // This gives up the coroutine's reference count on the shared
            // state. If this was the last reference count, the coroutine
            // should not suspend before exiting.
            return suspend_if{!this->base_type::requires_delete()};
        }

        void destroy() noexcept override
        {
            coroutine_handle<Derived>::from_promise(
                *static_cast<Derived*>(this))
                .destroy();
        }

        // allocator support for shared coroutine state
        [[nodiscard]] static void* allocate(std::size_t size)
        {
            using char_allocator = typename std::allocator_traits<
                allocator_type>::template rebind_alloc<char>;
            using traits = std::allocator_traits<char_allocator>;
            using unique_ptr = std::unique_ptr<char,
                hpx::util::allocator_deleter<char_allocator>>;

            char_allocator alloc{};
            unique_ptr p(traits::allocate(alloc, size),
                hpx::util::allocator_deleter<char_allocator>{alloc});

            return p.release();
        }

        static void deallocate(void* p, std::size_t size) noexcept
        {
            using char_allocator = typename std::allocator_traits<
                allocator_type>::template rebind_alloc<char>;
            using traits = std::allocator_traits<char_allocator>;

            char_allocator alloc{};
            traits::deallocate(alloc, static_cast<char*>(p), size);
        }
    };
}    // namespace hpx::lcos::detail

///////////////////////////////////////////////////////////////////////////////
namespace HPX_COROUTINE_NAMESPACE_STD {

    // Allow for functions that use co_await to return a hpx::future<T>
    template <typename T, typename... Ts>
    struct coroutine_traits<hpx::future<T>, Ts...>
    {
        using allocator_type = hpx::util::internal_allocator<coroutine_traits>;

        struct promise_type
          : hpx::lcos::detail::coroutine_promise_base<T, promise_type>
        {
            using base_type =
                hpx::lcos::detail::coroutine_promise_base<T, promise_type>;

            promise_type() = default;

            template <typename U>
            void return_value(U&& value)
            {
                this->base_type::set_value(HPX_FORWARD(U, value));
            }

            void unhandled_exception() noexcept
            {
                this->base_type::set_exception(std::current_exception());
            }

            [[nodiscard]] HPX_FORCEINLINE static void* operator new(
                std::size_t size)
            {
                return base_type::allocate(size);
            }

            HPX_FORCEINLINE static void operator delete(
                void* p, std::size_t size) noexcept
            {
                base_type::deallocate(p, size);
            }
        };
    };

    template <typename... Ts>
    struct coroutine_traits<hpx::future<void>, Ts...>
    {
        using allocator_type = hpx::util::internal_allocator<coroutine_traits>;

        struct promise_type
          : hpx::lcos::detail::coroutine_promise_base<void, promise_type>
        {
            using base_type =
                hpx::lcos::detail::coroutine_promise_base<void, promise_type>;

            promise_type() = default;

            void return_void()
            {
                this->base_type::set_value();
            }

            void unhandled_exception() noexcept
            {
                this->base_type::set_exception(std::current_exception());
            }

            [[nodiscard]] HPX_FORCEINLINE static void* operator new(
                std::size_t size)
            {
                return base_type::allocate(size);
            }

            HPX_FORCEINLINE static void operator delete(
                void* p, std::size_t size) noexcept
            {
                base_type::deallocate(p, size);
            }
        };
    };

    // Allow for functions that use co_await to return an hpx::shared_future<T>
    template <typename T, typename... Ts>
    struct coroutine_traits<hpx::shared_future<T>, Ts...>
    {
        using allocator_type = hpx::util::internal_allocator<coroutine_traits>;

        struct promise_type
          : hpx::lcos::detail::coroutine_promise_base<T, promise_type>
        {
            using base_type =
                hpx::lcos::detail::coroutine_promise_base<T, promise_type>;

            promise_type() = default;

            // Return the coroutine's return type itself: if the type
            // differed, the compiler would have to convert the returned
            // object, which some compilers get wrong for a coroutine frame
            // outliving the call
            hpx::shared_future<T> get_return_object()
            {
                return hpx::shared_future<T>(
                    this->base_type::get_return_object());
            }

            template <typename U>
            void return_value(U&& value)
            {
                this->base_type::set_value(HPX_FORWARD(U, value));
            }

            void unhandled_exception() noexcept
            {
                this->base_type::set_exception(std::current_exception());
            }

            [[nodiscard]] HPX_FORCEINLINE static void* operator new(
                std::size_t size)
            {
                return base_type::allocate(size);
            }

            HPX_FORCEINLINE static void operator delete(
                void* p, std::size_t size) noexcept
            {
                base_type::deallocate(p, size);
            }
        };
    };

    template <typename... Ts>
    struct coroutine_traits<hpx::shared_future<void>, Ts...>
    {
        using allocator_type = hpx::util::internal_allocator<coroutine_traits>;

        struct promise_type
          : hpx::lcos::detail::coroutine_promise_base<void, promise_type>
        {
            using base_type =
                hpx::lcos::detail::coroutine_promise_base<void, promise_type>;

            promise_type() = default;

            // see coroutine_traits<hpx::shared_future<T>, Ts...>; the template
            // parameter defers the use of shared_future<void>, which is not
            // complete yet here
            template <typename SharedFuture = hpx::shared_future<void>>
            SharedFuture get_return_object()
            {
                return SharedFuture(this->base_type::get_return_object());
            }

            void return_void()
            {
                this->base_type::set_value();
            }

            void unhandled_exception() noexcept
            {
                this->base_type::set_exception(std::current_exception());
            }

            [[nodiscard]] HPX_FORCEINLINE static void* operator new(
                std::size_t size)
            {
                return base_type::allocate(size);
            }

            HPX_FORCEINLINE static void operator delete(
                void* p, std::size_t size) noexcept
            {
                base_type::deallocate(p, size);
            }
        };
    };
}    // namespace HPX_COROUTINE_NAMESPACE_STD

#endif    // HPX_HAVE_CXX20_COROUTINES
