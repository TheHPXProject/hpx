//  Copyright (c) 2014 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/futures/traits/is_future.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>

namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename Tuple, typename Enable = void>
    struct is_future_tuple : std::false_type
    {
    };

    template <typename... Ts>
    struct is_future_tuple<hpx::tuple<Ts...>> : util::all_of<is_future<Ts>...>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    inline constexpr bool is_future_tuple_v = is_future_tuple<Ts...>::value;
}    // namespace hpx::traits
