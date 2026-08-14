//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/traits/action_priority.hpp>
#include <hpx/actions_base/traits/extract_action.hpp>
#include <hpx/modules/coroutines.hpp>

namespace hpx::actions {

    HPX_CXX_EXPORT template <typename Action>
    constexpr threads::thread_priority action_priority() noexcept
    {
        //  The mapping to 'normal' is now done at the last possible moment in
        //  the scheduler.
        using action_type = typename hpx::traits::extract_action<Action>::type;
        return hpx::traits::action_priority_v<action_type>;
    }
}    // namespace hpx::actions
