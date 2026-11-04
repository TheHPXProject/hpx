//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for action task size. An action that binds a
    // function given a size with hpx::threads::with_task_size has that size,
    // other actions have none unless they are given one with
    // HPX_ACTION_HAS_TASK_SIZE.
    HPX_CXX_EXPORT template <typename Action, typename Enable = void>
    struct action_task_size
    {
        static constexpr threads::task_size value = threads::task_size::unknown;
    };

    template <typename Action>
        requires requires { Action::function_task_size(); }
    struct action_task_size<Action>
    {
        static constexpr threads::task_size value =
            Action::function_task_size();
    };

    HPX_CXX_EXPORT template <typename Action>
    inline constexpr threads::task_size action_task_size_v =
        action_task_size<Action>::value;
}    // namespace hpx::traits
