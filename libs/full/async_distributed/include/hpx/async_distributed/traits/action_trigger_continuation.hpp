//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/async_distributed/continuation_fwd.hpp>
#include <hpx/async_distributed/trigger.hpp>
#include <hpx/modules/actions_base.hpp>

#include <utility>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Trait to determine the continuation type for an action
    template <typename Result, typename RemoteResult>
    struct action_trigger_continuation<
        actions::typed_continuation<Result, RemoteResult>>
    {
        template <typename F, typename... Ts>
        HPX_FORCEINLINE static void call(
            actions::typed_continuation<Result, RemoteResult>&& cont, F&& f,
            Ts&&... ts) noexcept
        {
            actions::trigger(
                HPX_MOVE(cont), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::traits
