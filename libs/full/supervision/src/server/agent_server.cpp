//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/supervision/server/agent.hpp>
#include <hpx/supervision/supervision_fwd.hpp>

using agent_component = hpx::supervision::server::agent_component;
using agent_component_type = hpx::components::component<agent_component>;
HPX_REGISTER_COMPONENT(agent_component_type, agent_component);

using invoke_action = agent_component::invoke_action;
HPX_REGISTER_ACTION(invoke_action);

namespace hpx::supervision::server {

    hpx::id_type create_agent(lifecycle_callback f)
    {
        return hpx::local_new<agent_component>(hpx::launch::sync, HPX_MOVE(f));
    }

    void agent_component::invoke(
        lifecycle_event_notification const& notify) const
    {
        if (f_)
        {
            f_(notify, hpx::throws);
        }
    }
}    // namespace hpx::supervision::server
