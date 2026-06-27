//
//  intrusive_ptr.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

// make inspect happy: hpxinspect:noinclude:hpx::intrusive_ptr

#pragma once

#include <hpx/config.hpp>
#include <hpx/memory/config/defines.hpp>
#include <hpx/assert.hpp>
#include <hpx/memory/detail/sp_convertible.hpp>

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <type_traits>
#include <utility>

namespace hpx {

    //  intrusive_ptr
    //
    //  A smart pointer that uses intrusive reference counting.
    //
    //  Relies on unqualified calls to
    //
    //      void intrusive_ptr_add_ref(T * p);
    //      void intrusive_ptr_release(T * p);
    //
    //          (p != nullptr)
    //
    //  The object is responsible for destroying itself.
    //
    HPX_CXX_CORE_EXPORT template <typename T>
    class intrusive_ptr
    {
        using this_type = intrusive_ptr;

    public:
        using element_type = T;

        constexpr intrusive_ptr() noexcept = default;

        constexpr intrusive_ptr(std::nullptr_t) noexcept
          : intrusive_ptr()
        {
        }

        /* implicit */ intrusive_ptr(T* p, bool add_ref = true) noexcept(
            noexcept(intrusive_ptr_add_ref(std::declval<T*>())))
          : px(p)
        {
            if (px != nullptr && add_ref)
                intrusive_ptr_add_ref(px);
        }

        template <typename U>
            requires(memory::detail::sp_convertible_v<U, T>)
        intrusive_ptr(intrusive_ptr<U> const& rhs) noexcept(
            noexcept(intrusive_ptr_add_ref(std::declval<U*>())))
          : px(rhs.get())
        {
            if (px != nullptr)
                intrusive_ptr_add_ref(px);
        }

        intrusive_ptr(intrusive_ptr const& rhs) noexcept(
            noexcept(intrusive_ptr_add_ref(std::declval<T*>())))
          : px(rhs.px)
        {
            if (px != nullptr)
                intrusive_ptr_add_ref(px);
        }

        HPX_FORCEINLINE ~intrusive_ptr() noexcept
        {
            if (px != nullptr)
                intrusive_ptr_release(px);
        }

        template <typename U>
        intrusive_ptr& operator=(intrusive_ptr<U> const& rhs) noexcept(
            noexcept(intrusive_ptr_add_ref(std::declval<U*>())))
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        // Move support
        constexpr intrusive_ptr(intrusive_ptr&& rhs) noexcept
          : px(rhs.px)
        {
            rhs.px = nullptr;
        }

        intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept
        {
            this_type(static_cast<intrusive_ptr&&>(rhs)).swap(*this);
            return *this;
        }

        template <typename U>
        friend class intrusive_ptr;

        template <typename U,
            typename Enable =
                std::enable_if_t<memory::detail::sp_convertible_v<U, T>>>
        explicit constexpr intrusive_ptr(intrusive_ptr<U>&& rhs) noexcept
          : px(rhs.px)
        {
            rhs.px = nullptr;
        }

        template <typename U>
        intrusive_ptr& operator=(intrusive_ptr<U>&& rhs) noexcept
        {
            this_type(static_cast<intrusive_ptr<U>&&>(rhs)).swap(*this);
            return *this;
        }

        // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
        intrusive_ptr& operator=(intrusive_ptr const& rhs) noexcept
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr& operator=(T* rhs) noexcept
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        void reset() noexcept
        {
            this_type().swap(*this);
        }

        void reset(T* rhs) noexcept
        {
            this_type(rhs).swap(*this);
        }

        void reset(T* rhs, bool add_ref) noexcept
        {
            this_type(rhs, add_ref).swap(*this);
        }

        [[nodiscard]] constexpr T* get() const noexcept
        {
            return px;
        }

        constexpr T* detach() noexcept
        {
            T* ret = px;
            px = nullptr;
            return ret;
        }

        T& operator*() const noexcept
        {
            HPX_ASSERT(px != nullptr);
            return *px;
        }

        T* operator->() const noexcept
        {
            HPX_ASSERT(px != nullptr);
            return px;
        }

        explicit constexpr operator bool() const noexcept
        {
            return px != nullptr;
        }

        constexpr void swap(intrusive_ptr& rhs) noexcept
        {
            T* tmp = px;
            px = rhs.px;
            rhs.px = tmp;
        }

    private:
        T* px = nullptr;
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr bool operator==(
        hpx::intrusive_ptr<T> const& a, hpx::intrusive_ptr<U> const& b) noexcept
    {
        return a.get() == b.get();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr bool operator!=(
        hpx::intrusive_ptr<T> const& a, hpx::intrusive_ptr<U> const& b) noexcept
    {
        return a.get() != b.get();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr bool operator==(
        hpx::intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() == b;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr bool operator!=(
        hpx::intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() != b;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr bool operator==(
        T* a, hpx::intrusive_ptr<U> const& b) noexcept
    {
        return a == b.get();
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr bool operator!=(
        T* a, hpx::intrusive_ptr<U> const& b) noexcept
    {
        return a != b.get();
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr bool operator==(
        hpx::intrusive_ptr<T> const& p, std::nullptr_t) noexcept
    {
        return p.get() == nullptr;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr bool operator==(
        std::nullptr_t, hpx::intrusive_ptr<T> const& p) noexcept
    {
        return p.get() == nullptr;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr bool operator!=(
        hpx::intrusive_ptr<T> const& p, std::nullptr_t) noexcept
    {
        return p.get() != nullptr;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr bool operator!=(
        std::nullptr_t, hpx::intrusive_ptr<T> const& p) noexcept
    {
        return p.get() != nullptr;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr bool operator<(
        hpx::intrusive_ptr<T> const& a, hpx::intrusive_ptr<T> const& b) noexcept
    {
        return std::less<T*>{}(a.get(), b.get());
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    void swap(hpx::intrusive_ptr<T>& lhs, hpx::intrusive_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    // mem_fn support
    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr T* get_pointer(
        hpx::intrusive_ptr<T> const& p) noexcept
    {
        return p.get();
    }

    // pointer casts
    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr hpx::intrusive_ptr<T> static_pointer_cast(
        hpx::intrusive_ptr<U> const& p) noexcept
    {
        return hpx::intrusive_ptr<T>(static_cast<T*>(p.get()));
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr hpx::intrusive_ptr<T> const_pointer_cast(
        hpx::intrusive_ptr<U> const& p) noexcept
    {
        return hpx::intrusive_ptr<T>(const_cast<T*>(p.get()));
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] hpx::intrusive_ptr<T> dynamic_pointer_cast(
        hpx::intrusive_ptr<U> const& p) noexcept
    {
        return hpx::intrusive_ptr<T>(dynamic_cast<T*>(p.get()));
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr hpx::intrusive_ptr<T> static_pointer_cast(
        hpx::intrusive_ptr<U>&& p) noexcept
    {
        return hpx::intrusive_ptr<T>(static_cast<T*>(p.detach()), false);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] constexpr hpx::intrusive_ptr<T> const_pointer_cast(
        hpx::intrusive_ptr<U>&& p) noexcept
    {
        return hpx::intrusive_ptr<T>(const_cast<T*>(p.detach()), false);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    [[nodiscard]] hpx::intrusive_ptr<T> dynamic_pointer_cast(
        hpx::intrusive_ptr<U>&& p) noexcept
    {
        T* p2 = dynamic_cast<T*>(p.get());

        hpx::intrusive_ptr<T> r(p2, false);

        if (p2)
            p.detach();

        return r;
    }

    // operator<<
    HPX_CXX_CORE_EXPORT template <typename Y>
    std::ostream& operator<<(std::ostream& os, hpx::intrusive_ptr<Y> const& p)
    {
        os << p.get();
        return os;
    }
}    // namespace hpx

// support hashing
template <typename T>
struct std::hash<::hpx::intrusive_ptr<T>>
{
    using result_type = std::size_t;

    constexpr result_type operator()(
        ::hpx::intrusive_ptr<T> const& p) const noexcept
    {
        return hash<T*>{}(p.get());
    }
};
