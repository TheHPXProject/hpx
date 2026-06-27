//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Trait to determine the continuation type for an action
    HPX_CXX_EXPORT template <typename Continuation, typename Enable = void>
    struct action_trigger_continuation
    {
        template <typename F, typename... Ts>
        static constexpr void call(Continuation&&, F&&, Ts&&...) noexcept
        {
        }
    };
}    // namespace hpx::traits
