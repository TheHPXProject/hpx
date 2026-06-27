//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/runtime_distributed/get_locality_name.hpp>

#include <string>

HPX_PLAIN_ACTION_ID(hpx::detail::get_locality_name,
    hpx_get_locality_name_action, hpx::actions::hpx_get_locality_name_action_id)

namespace hpx {

    future<std::string> get_locality_name(hpx::id_type const& id)
    {
        return async<hpx_get_locality_name_action>(id);
    }
}    // namespace hpx
