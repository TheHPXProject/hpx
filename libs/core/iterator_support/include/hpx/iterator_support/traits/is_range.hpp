//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/iterator_support/range.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // return whether a given type is a range generator (i.e. exposes supports
    // an iterate function that returns a range)
    HPX_CXX_CORE_EXPORT template <typename T>
    using is_range_generator = util::detail::is_range_generator<T>;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_range_generator_v = is_range_generator<T>::value;

    // return the iterator category encapsulated by the range
    HPX_CXX_CORE_EXPORT template <typename T>
    using range_category_t = iter_category_t<std::ranges::iterator_t<T>>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename R,
        bool IsRange = std::ranges::range<R>>
    struct range_traits
    {
    };

    template <typename R>
    struct range_traits<R, true>
      : std::iterator_traits<typename util::detail::iterator<R>::type>
    {
        using iterator_type = typename util::detail::iterator<R>::type;
        using sentinel_type = typename util::detail::sentinel<R>::type;
    };
}    // namespace hpx::traits
