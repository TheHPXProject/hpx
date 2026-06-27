//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/datastructures/tuple.hpp>

#include <type_traits>

namespace hpx::traits {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
        struct is_tuple_like_impl : std::false_type
        {
        };

        template <typename T>
        struct is_tuple_like_impl<T,
            std::void_t<decltype(hpx::tuple_size<T>::value)>> : std::true_type
        {
        };
    }    // namespace detail

    /// Deduces to a true type if the given parameter T has a specific tuple
    /// like size.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_tuple_like : detail::is_tuple_like_impl<std::remove_cv_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;
}    // namespace hpx::traits
