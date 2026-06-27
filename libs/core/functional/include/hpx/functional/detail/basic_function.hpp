//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2013-2025 Hartmut Kaiser
//  Copyright (c) 2014-2019 Agustin Berge
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/functional/detail/empty_function.hpp>
#include <hpx/functional/detail/vtable/function_vtable.hpp>
#include <hpx/functional/detail/vtable/vtable.hpp>
#include <hpx/functional/traits/get_function_address.hpp>
#include <hpx/functional/traits/get_function_annotation.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::util::detail {

    inline constexpr std::size_t function_storage_size = 3 * sizeof(void*);

    ///////////////////////////////////////////////////////////////////////////
    class HPX_CORE_EXPORT function_base
    {
        using vtable = function_base_vtable;

    public:
        explicit constexpr function_base(
            function_base_vtable const* empty_vptr) noexcept
          : vptr(empty_vptr)
          , object(nullptr)
          , storage_init()
        {
        }

        function_base(function_base const& other, vtable const* empty_vtable);
        function_base(function_base&& other, vtable const* empty_vptr) noexcept;

        ~function_base();

        void op_assign(function_base const& other, vtable const* empty_vtable);
        void op_assign(
            function_base&& other, vtable const* empty_vtable) noexcept;

        void destroy() const noexcept;
        void reset(vtable const* empty_vptr) noexcept;
        void swap(function_base& f) noexcept;

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return object == nullptr;
        }

        explicit constexpr operator bool() const noexcept
        {
            return !empty();
        }

        [[nodiscard]] std::size_t get_function_address() const;
        [[nodiscard]] char const* get_function_annotation() const;
        [[nodiscard]] hpx::tracing::annotation_handle
        get_function_annotation_tracing() const;

    protected:
        vtable const* vptr;
        void* object;
        union
        {
            char storage_init;
            mutable unsigned char storage[function_storage_size];
        };
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename F>
    [[nodiscard]] constexpr bool is_empty_function(F* fp) noexcept
    {
        return fp == nullptr;
    }

    template <typename T, typename C>
    [[nodiscard]] constexpr bool is_empty_function(T C::* mp) noexcept
    {
        return mp == nullptr;
    }

    [[nodiscard]] constexpr bool is_empty_function_impl(
        function_base const* f) noexcept
    {
        return f->empty();
    }

    [[nodiscard]] constexpr bool is_empty_function_impl(...) noexcept
    {
        return false;
    }

    template <typename F>
    [[nodiscard]] constexpr bool is_empty_function(F const& f) noexcept
    {
        return detail::is_empty_function_impl(&f);
    }

    template <typename Sig>
    [[nodiscard]] constexpr bool is_empty_function(
        std::function<Sig> const& f) noexcept
    {
        return !f;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Sig, bool Copyable, bool Serializable>
    class basic_function;

    template <bool Copyable, typename R, typename... Ts>
    class basic_function<R(Ts...), Copyable, /*Serializable*/ false>
      : public function_base
    {
        using base_type = function_base;
        using vtable = function_vtable<R(Ts...), Copyable>;

    public:
        constexpr basic_function() noexcept
          : base_type(get_empty_vtable())
        {
        }

        basic_function(basic_function const& other)
          : base_type(other, get_empty_vtable())
        {
        }

        basic_function(basic_function&& other) noexcept
          : base_type(HPX_MOVE(other), get_empty_vtable())
        {
        }

        basic_function& operator=(basic_function const& other)
        {
            base_type::op_assign(other, get_empty_vtable());
            return *this;
        }

        basic_function& operator=(basic_function&& other) noexcept
        {
            base_type::op_assign(HPX_MOVE(other), get_empty_vtable());
            return *this;
        }

        ~basic_function() = default;

        constexpr void assign(std::nullptr_t) noexcept
        {
            base_type::reset(get_empty_vtable());
        }

        template <typename F>
        void assign(F&& f)
        {
            using T = std::decay_t<F>;
            static_assert(!Copyable || std::is_constructible_v<T, T const&>,
                "F shall be CopyConstructible");

            if (!detail::is_empty_function(f))
            {
                constexpr vtable const* f_vptr = get_vtable<T>();
                void* buffer;
                if (vptr == f_vptr)
                {
                    HPX_ASSERT(object != nullptr);

                    // reuse object storage
                    buffer = object;
                    std::destroy_at(
                        std::addressof(vtable::template get<T>(object)));
                }
                else
                {
                    destroy();
                    vptr = f_vptr;
                    buffer = vtable::template allocate<T>(
                        storage, function_storage_size);
                }
                // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                object = ::new (buffer) T(HPX_FORWARD(F, f));
            }
            else
            {
                base_type::reset(get_empty_vtable());
            }
        }

        constexpr void reset() noexcept
        {
            base_type::reset(get_empty_vtable());
        }

        using base_type::empty;
        using base_type::swap;
        using base_type::operator bool;

        template <typename T>
        [[nodiscard]] T* target() noexcept
        {
            using TD = std::remove_cv_t<T>;
            static_assert(hpx::is_invocable_r_v<R, TD&, Ts...>,
                "T shall be Callable with the function signature");

            constexpr vtable const* f_vptr = get_vtable<TD>();
            if (vptr != f_vptr || empty())
                return nullptr;

            return &vtable::template get<TD>(object);
        }

        template <typename T>
        [[nodiscard]] T const* target() const noexcept
        {
            using TD = std::remove_cv_t<T>;
            static_assert(hpx::is_invocable_r_v<R, TD&, Ts...>,
                "T shall be Callable with the function signature");

            constexpr vtable const* f_vptr = get_vtable<TD>();
            if (vptr != f_vptr || empty())
                return nullptr;

            return &vtable::template get<TD>(object);
        }

        HPX_FORCEINLINE R operator()(Ts... vs) const
        {
            vtable const* f_vptr = static_cast<vtable const*>(base_type::vptr);
            return f_vptr->invoke(object, HPX_FORWARD(Ts, vs)...);
        }

        using base_type::get_function_address;
        using base_type::get_function_annotation;
        using base_type::get_function_annotation_tracing;

    private:
        [[nodiscard]] static constexpr vtable const* get_empty_vtable() noexcept
        {
            return detail::get_empty_function_vtable<R(Ts...)>();
        }

        template <typename T>
        [[nodiscard]] static constexpr vtable const* get_vtable() noexcept
        {
            return detail::get_vtable<vtable, T>();
        }

    protected:
        using base_type::object;
        using base_type::storage;
        using base_type::vptr;
    };
}    // namespace hpx::util::detail
