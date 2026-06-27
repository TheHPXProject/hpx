//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for action stack size
    HPX_CXX_EXPORT template <typename Action, typename Enable = void>
    struct action_priority
    {
        static constexpr threads::thread_priority value =
            threads::thread_priority::default_;
    };

    HPX_CXX_EXPORT template <typename Action>
    inline constexpr threads::thread_priority action_priority_v =
        action_priority<Action>::value;
}    // namespace hpx::traits
