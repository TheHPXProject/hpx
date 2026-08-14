//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/futures/traits/is_future.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <functional>
#include <type_traits>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename R, typename Enable = void>
    struct is_future_range : std::false_type
    {
    };

    template <typename R>
    struct is_future_range<R, std::enable_if_t<std::ranges::range<R>>>
      : is_future<typename range_traits<R>::value_type>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename R>
    using is_future_range_t = typename is_future_range<R>::type;

    HPX_CXX_CORE_EXPORT template <typename R>
    inline constexpr bool is_future_range_v = is_future_range<R>::value;

    HPX_CXX_CORE_EXPORT template <typename R, typename Enable = void>
    struct is_ref_wrapped_future_range : std::false_type
    {
    };

    template <typename R>
    struct is_ref_wrapped_future_range<::std::reference_wrapper<R>>
      : is_future_range<R>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename R>
    using is_ref_wrapped_future_range_t =
        typename is_ref_wrapped_future_range<R>::type;

    HPX_CXX_CORE_EXPORT template <typename R>
    inline constexpr bool is_ref_wrapped_future_range_v =
        is_ref_wrapped_future_range<R>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename R,
        bool IsFutureRange = is_future_range<R>::value>
    struct future_range_traits
    {
    };

    template <typename R>
    struct future_range_traits<R, true>
    {
        using future_type = typename range_traits<R>::value_type;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename T>
        struct is_future_or_future_range
          : std::integral_constant<bool, is_future_v<T> || is_future_range_v<T>>
        {
        };

        template <typename R>
        inline constexpr bool is_future_or_future_range_v =
            is_future_or_future_range<R>::value;
    }    // namespace detail
}    // namespace hpx::traits
