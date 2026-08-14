//  Copyright (c) 2015 Thomas Heller
//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::traits {

    namespace detail {

        template <typename Continuation, typename Enable = void>
        struct is_continuation_impl : std::false_type
        {
        };

        template <typename Continuation>
        struct is_continuation_impl<Continuation,
            std::void_t<typename Continuation::continuation_tag>>
          : std::true_type
        {
        };
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Continuation, typename Enable = void>
    struct is_continuation
      : detail::is_continuation_impl<std::decay_t<Continuation>>
    {
    };

    HPX_CXX_EXPORT template <typename Continuation>
    inline constexpr bool is_continuation_v =
        is_continuation<Continuation>::value;
}    // namespace hpx::traits
