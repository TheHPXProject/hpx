//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/traits/action_continuation.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for action capabilities
    HPX_CXX_EXPORT template <typename Action, typename Enable = void>
    struct action_decorate_continuation
    {
        using continuation_type =
            typename hpx::traits::action_continuation<Action>::type;

        static constexpr bool call(continuation_type&) noexcept
        {
            // by default, we do nothing
            return false;    // continuation has not been modified
        }
    };
}    // namespace hpx::traits
