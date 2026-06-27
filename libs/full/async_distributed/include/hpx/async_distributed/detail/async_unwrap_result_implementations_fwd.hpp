//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/naming_base.hpp>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Launch, typename... Ts>
    hpx::traits::extract_action_t<Action>::local_result_type
    async_unwrap_result_impl(
        Launch&& policy, hpx::id_type const& id, Ts&&... vs);
}    // namespace hpx::detail
