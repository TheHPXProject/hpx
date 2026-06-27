//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::actions {

    HPX_CXX_EXPORT template <typename Result, typename RemoteResult = Result>
    struct typed_continuation;
}    // namespace hpx::actions

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Trait to determine the continuation type for an action
    HPX_CXX_EXPORT template <typename Action, typename Enable = void>
    struct action_continuation
    {
        using type =
            hpx::actions::typed_continuation<typename Action::local_result_type,
                typename Action::remote_result_type>;
    };

    HPX_CXX_EXPORT template <typename Action>
    using action_continuation_t = typename action_continuation<Action>::type;
}    // namespace hpx::traits
