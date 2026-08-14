//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>

namespace hpx::actions {

    HPX_CXX_EXPORT class HPX_EXPORT continuation;

    HPX_CXX_EXPORT template <typename Result, typename RemoteResult, typename F,
        typename... Ts>
    void trigger(
        typed_continuation<Result, RemoteResult>&&, F&&, Ts&&...) noexcept;
}    // namespace hpx::actions
