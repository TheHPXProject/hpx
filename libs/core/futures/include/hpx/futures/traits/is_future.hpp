//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/future_fwd.hpp>
#include <hpx/modules/type_support.hpp>

#include <functional>
#include <type_traits>

namespace hpx::traits {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Future, typename Enable = void>
        struct is_unique_future : std::false_type
        {
        };

        template <typename R>
        struct is_unique_future<hpx::future<R>> : std::true_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename R>
        inline constexpr bool is_unique_future_v = is_unique_future<R>::value;

        HPX_CXX_CORE_EXPORT template <typename Future, typename Enable = void>
        struct is_future_customization_point : std::false_type
        {
        };

        template <typename R>
        struct is_future_customization_point<hpx::future<R>> : std::true_type
        {
        };

        template <typename R>
        struct is_future_customization_point<hpx::shared_future<R>>
          : std::true_type
        {
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename Future>
    struct is_future
      : detail::is_future_customization_point<std::decay_t<Future>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename R>
    inline constexpr bool is_future_v = is_future<R>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename... Ts>
    using is_future_any = hpx::util::any_of<is_future<Ts>...>;

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    inline constexpr bool is_future_any_v = is_future_any<Ts...>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Future>
    struct is_ref_wrapped_future : std::false_type
    {
    };

    template <typename Future>
    struct is_ref_wrapped_future<std::reference_wrapper<Future>>
      : is_future<Future>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename R>
    inline constexpr bool is_ref_wrapped_future_v =
        is_ref_wrapped_future<R>::value;
}    // namespace hpx::traits
