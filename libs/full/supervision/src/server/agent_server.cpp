//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/supervision/server/agent.hpp>
#include <hpx/supervision/supervision_api.hpp>

using agent_component = hpx::supervision::server::agent_component;
using agent_component_type = hpx::components::component<agent_component>;
HPX_REGISTER_COMPONENT(agent_component_type, agent_component);

using invoke_if_active_action = agent_component::invoke_if_active_action;
HPX_REGISTER_ACTION_ID(invoke_if_active_action, invoke_if_active_action,
    hpx::actions::supervision_invoke_if_active_action_id);

using deactivate_and_wait_action = agent_component::deactivate_and_wait_action;
HPX_REGISTER_ACTION_ID(deactivate_and_wait_action, deactivate_and_wait_action,
    hpx::actions::supervision_deactivate_and_wait_action_id);

namespace hpx::supervision::server {

    hpx::id_type create_agent(lifecycle_callback f)
    {
        return hpx::local_new<agent_component>(hpx::launch::sync, HPX_MOVE(f));
    }

    void agent_component::invoke_if_active(
        lifecycle_event_notification const& notify)
    {
        {
            std::lock_guard<hpx::spinlock> l(mtx_);
            if (!active_)
            {
                return;
            }
            ++in_flight_;
        }

        try
        {
            if (f_)
            {
                f_(notify, hpx::throws);
            }
        }
        catch (...)
        {
            finish_delivery();
            throw;
        }

        finish_delivery();
    }

    void agent_component::finish_delivery()
    {
        std::unique_lock<hpx::spinlock> l(mtx_);
        HPX_ASSERT(in_flight_ != 0);

        if (--in_flight_ == 0)
        {
            cv_.notify_all(HPX_MOVE(l));
        }
    }

    void agent_component::deactivate_and_wait()
    {
        std::unique_lock<hpx::spinlock> l(mtx_);
        active_ = false;
        while (in_flight_ != 0)
        {
            cv_.wait(l);
        }
    }
}    // namespace hpx::supervision::server
