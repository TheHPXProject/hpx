//  Copyright (c) 2024-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file reference_wrapper.hpp
/// \page hpx::reference_wrapper, hpx::ref, hpx::cref
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <functional>
#include <type_traits>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct reference_wrapper : std::reference_wrapper<T>
    {
        reference_wrapper() = delete;

        using std::reference_wrapper<T>::reference_wrapper;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    reference_wrapper(T&) -> reference_wrapper<T>;

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr reference_wrapper<T> ref(T& val) noexcept
    {
        return reference_wrapper<T>(val);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    void ref(T const&&) = delete;

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr reference_wrapper<T> ref(
        reference_wrapper<T> val) noexcept
    {
        return val;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr reference_wrapper<T const> cref(
        T const& val) noexcept
    {
        return reference_wrapper<T const>(val);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    void cref(T const&&) = delete;

    HPX_CXX_CORE_EXPORT template <typename T>
    [[nodiscard]] constexpr reference_wrapper<T const> cref(
        reference_wrapper<T> val) noexcept
    {
        return val;
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace traits {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct needs_reference_semantics : std::false_type
        {
        };

        template <typename T>
        struct needs_reference_semantics<T const> : needs_reference_semantics<T>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool needs_reference_semantics_v =
            needs_reference_semantics<T>::value;
    }    // namespace traits

    template <typename T>
    struct reference_wrapper<T,
        std::enable_if_t<traits::needs_reference_semantics_v<T>>>
    {
        using type = T;

        // we define a default constructor to support serialization
        reference_wrapper() = default;

        // clang-format off
        template <typename U,
            typename Enable = std::enable_if_t<
                !std::is_same_v<std::decay_t<U>, reference_wrapper>>>
        // clang-format on
        reference_wrapper(U&& val)
          : ptr(val.ref())
        {
        }

        reference_wrapper(reference_wrapper const& rhs)
          : ptr(rhs.get())
        {
        }
        reference_wrapper(reference_wrapper&& rhs) = default;

        reference_wrapper& operator=(reference_wrapper const& rhs)
        {
            ptr = rhs.get();
            return *this;
        }
        reference_wrapper& operator=(reference_wrapper&& rhs) = default;

        operator type() const
        {
            return ptr.ref();
        }

        [[nodiscard]] type get() const
        {
            return ptr.ref();
        }

    private:
        T ptr{};
    };

    HPX_CXX_CORE_EXPORT template <typename T>
        requires(traits::needs_reference_semantics_v<T>)
    reference_wrapper<T> ref(T&& val) noexcept
    {
        return reference_wrapper<T>(HPX_FORWARD(T, val));
    }

    namespace util {

        template <typename T>
        struct unwrap_reference<::hpx::reference_wrapper<T>>
        {
            using type = T;
        };

        template <typename T>
        struct unwrap_reference<::hpx::reference_wrapper<T> const>
        {
            using type = T;
        };

        template <typename X>
        struct detail::decay_unwrap_impl<::hpx::reference_wrapper<X>,
            std::enable_if_t<traits::needs_reference_semantics_v<X>>>
        {
            using type = X;

            constexpr static decltype(auto) call(
                ::hpx::reference_wrapper<X> ref) noexcept
            {
                return ref.get();
            }
        };
    }    // namespace util
}    // namespace hpx
