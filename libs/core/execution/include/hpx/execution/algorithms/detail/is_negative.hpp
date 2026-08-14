//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::parallel::detail {

    // main template represents non-integral types (raises error)
    HPX_CXX_CORE_EXPORT template <typename Size, typename Enable = void>
    struct is_negative_helper;

    // signed integral values may be negative
    template <typename T>
    struct is_negative_helper<T, std::enable_if_t<std::is_signed_v<T>>>
    {
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr bool call(
            T const& size) noexcept
        {
            return size < 0;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr T abs(
            T const& val) noexcept
        {
            return val < 0 ? -val : val;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE static T negate(T const& val)
        {
            return -val;
        }
    };

    // unsigned integral values are never negative
    template <typename T>
    struct is_negative_helper<T, std::enable_if_t<std::is_unsigned_v<T>>>
    {
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr bool call(
            T const&) noexcept
        {
            return false;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr T abs(
            T const& val) noexcept
        {
            return val;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr T negate(
            T const& val) noexcept
        {
            return val;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr bool is_negative(
        T const& val) noexcept
    {
        return is_negative_helper<T>::call(val);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T abs(T const& val) noexcept
    {
        return is_negative_helper<T>::abs(val);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T negate(T const& val) noexcept
    {
        return is_negative_helper<T>::negate(val);
    }
}    // namespace hpx::parallel::detail
