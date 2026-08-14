//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2019 Austin McCartney
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>
#include <utility>

namespace hpx::traits {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T, typename U,
            typename Enable = void>
        struct equality_result
        {
        };

        // different versions of clang-format disagree
        // clang-format off
        template <typename T, typename U>
        struct equality_result<T, U,
            std::void_t<decltype(
                std::declval<T const&>() == std::declval<U const&>())>>
        {
            using type =
                decltype(std::declval<T const&>() == std::declval<U const&>());
        };
        // clang-format on

        HPX_CXX_CORE_EXPORT template <typename T, typename U>
        using equality_result_t = typename equality_result<T, U>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T, typename U,
            typename Enable = void>
        struct inequality_result
        {
        };

        // different versions of clang-format disagree
        // clang-format off
        template <typename T, typename U>
        struct inequality_result<T, U,
            std::void_t<decltype(
                std::declval<T const&>() != std::declval<U const&>())>>
        {
            using type =
                decltype(std::declval<T const&>() != std::declval<U const&>());
        };
        // clang-format on

        HPX_CXX_CORE_EXPORT template <typename T, typename U>
        using inequality_result_t = typename inequality_result<T, U>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T, typename U,
            typename Enable = void>
        struct is_weakly_equality_comparable_with : std::false_type
        {
        };

        template <typename T, typename U>
        struct is_weakly_equality_comparable_with<T, U,
            std::void_t<detail::equality_result_t<T, U>,
                detail::equality_result_t<U, T>,
                detail::inequality_result_t<T, U>,
                detail::inequality_result_t<U, T>>> : std::true_type
        {
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    struct is_weakly_equality_comparable_with
      : detail::is_weakly_equality_comparable_with<std::decay_t<T>,
            std::decay_t<U>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    inline constexpr bool is_weakly_equality_comparable_with_v =
        is_weakly_equality_comparable_with<T, U>::value;

    // for now is_equality_comparable is equivalent to its weak version
    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    struct is_equality_comparable_with
      : detail::is_weakly_equality_comparable_with<std::decay_t<T>,
            std::decay_t<U>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename U>
    inline constexpr bool is_equality_comparable_with_v =
        is_equality_comparable_with<T, U>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_equality_comparable
      : detail::is_weakly_equality_comparable_with<std::decay_t<T>,
            std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_equality_comparable_v =
        is_equality_comparable<T>::value;
}    // namespace hpx::traits
