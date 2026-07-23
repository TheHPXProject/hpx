//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/supervision/supervision_api.hpp>

#include <cstddef>
#include <functional>

namespace hpx::supervision::server {

    class HPX_EXPORT agent_component
      : public hpx::components::component_base<agent_component>
    {
    public:
        agent_component() = default;

        explicit agent_component(lifecycle_callback f) noexcept
          : f_(HPX_MOVE(f))
        {
        }

        void invoke_if_active(lifecycle_event_notification const& notify);
        HPX_DEFINE_COMPONENT_ACTION(
            agent_component, invoke_if_active, invoke_if_active_action)

        void deactivate_and_wait();
        HPX_DEFINE_COMPONENT_ACTION(
            agent_component, deactivate_and_wait, deactivate_and_wait_action)

    private:
        void finish_delivery();

        lifecycle_callback f_;

        hpx::spinlock mtx_;
        hpx::lcos::local::detail::condition_variable cv_;
        bool active_ = true;
        std::size_t in_flight_ = 0;
    };

    // Create a local agent wrapping the given function
    HPX_EXPORT hpx::id_type create_agent(lifecycle_callback f);

}    // namespace hpx::supervision::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::agent_component::invoke_if_active_action,
    agent_component_invoke_if_active_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::agent_component::deactivate_and_wait_action,
    agent_component_deactivate_and_wait_action)
