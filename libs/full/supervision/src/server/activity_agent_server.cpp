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
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/supervision/server/activity_agent.hpp>
#include <hpx/supervision/supervision_api.hpp>

#include <cstddef>
#include <exception>
#include <mutex>

using activity_agent_component =
    hpx::supervision::server::activity_agent_component;
using activity_agent_component_type =
    hpx::components::component<activity_agent_component>;
HPX_REGISTER_COMPONENT(activity_agent_component_type, activity_agent_component);

using invoke_if_active_activity_action =
    activity_agent_component::invoke_if_active_action;
HPX_REGISTER_ACTION_ID(invoke_if_active_activity_action,
    invoke_if_active_activity_action,
    hpx::actions::supervision_activity_invoke_if_active_action_id);

using deactivate_and_wait_activity_action =
    activity_agent_component::deactivate_and_wait_action;
HPX_REGISTER_ACTION_ID(deactivate_and_wait_activity_action,
    deactivate_and_wait_activity_action,
    hpx::actions::supervision_activity_deactivate_and_wait_action_id);

namespace hpx::supervision::server {

    hpx::id_type create_activity_agent(activity_callback f)
    {
        return hpx::local_new<activity_agent_component>(
            hpx::launch::sync, HPX_MOVE(f));
    }

    bool activity_agent_component::invoke_if_active(
        activity_notification const& notify)
    {
        {
            std::lock_guard<hpx::spinlock> l(mtx_);
            if (!active_)
            {
                return false;
            }
            ++in_flight_;
        }

        bool result = true;
        try
        {
            if (f_)
            {
                result = f_(notify);
            }
        }
        catch (...)
        {
            finish_delivery();
            std::rethrow_exception(std::current_exception());
        }

        finish_delivery();

        return result;
    }

    void activity_agent_component::finish_delivery()
    {
        std::unique_lock<hpx::spinlock> l(mtx_);
        HPX_ASSERT(in_flight_ != 0);

        if (--in_flight_ == 0)
        {
            cv_.notify_all(HPX_MOVE(l));
        }
    }

    void activity_agent_component::deactivate_and_wait()
    {
        std::unique_lock<hpx::spinlock> l(mtx_);
        active_ = false;

        while (in_flight_ > 0)
        {
            cv_.wait(l);
        }
    }
}    // namespace hpx::supervision::server
