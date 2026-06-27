//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::traits {

    // verify that given Action is actually supported by the given Component
    HPX_CXX_EXPORT template <typename Action, typename Component,
        typename Enable = void>
    struct is_valid_action
      : std::is_same<std::decay_t<typename Action::component_type>,
            std::decay_t<Component>>
    {
    };

    HPX_CXX_EXPORT template <typename Action, typename Component>
    inline constexpr bool is_valid_action_v =
        is_valid_action<Action, Component>::value;
}    // namespace hpx::traits
