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
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/supervision/supervision_fwd.hpp>

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace hpx::supervision::server {

    ////////////////////////////////////////////////////////////////////////////
    // Base name used to register the component
    HPX_CXX_EXPORT inline constexpr char const* const supervision_manager_name =
        "supervision_manager/";

    struct supervision_manager
      : hpx::components::fixed_component_base<supervision_manager>
    {
        using base_type = components::fixed_component_base<supervision_manager>;

        supervision_manager()
          : base_type(supervision::supervision_manager_msb,
                supervision::supervision_manager_lsb)
        {
        }

        void finalize() const;

        void register_server_instance(char const* service_name,
            std::uint32_t locality_id, error_code& ec = throws);
        void unregister_server_instance(error_code& ec = throws) const;

        // Supervision API implementation
        void publish_event(hpx::id_type const& target, event ev);

        lifecycle_state query_state(hpx::id_type const& target);

        hpx::id_type register_observer(
            hpx::id_type const& target, hpx::id_type const& agent);

        void unregister_observer(hpx::id_type const& observer_handle);

        HPX_DEFINE_COMPONENT_ACTION(supervision_manager, publish_event)
        HPX_DEFINE_COMPONENT_ACTION(supervision_manager, register_observer)
        HPX_DEFINE_COMPONENT_ACTION(supervision_manager, unregister_observer)
        HPX_DEFINE_COMPONENT_ACTION(supervision_manager, query_state)

    protected:
        hpx::future<void> fire_event(
            hpx::id_type const& target, hpx::id_type const& agent) const;
        hpx::future<void> fire_events(hpx::id_type const& target);

        void record_error(
            hpx::id_type const& target, hpx::error_code const& ec);

    private:
        mutable hpx::spinlock mtx_;
        std::string instance_name_;

        // registered events: targets -> states
        std::map<hpx::id_type, lifecycle_state> states_;
        // registered observers: targets -> agents
        std::map<hpx::id_type, std::vector<hpx::id_type>> observers_;
        // inverse lookup for agents: agents -> targets
        std::map<hpx::id_type, std::vector<hpx::id_type>> agents_;
    };
}    // namespace hpx::supervision::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::publish_event_action,
    supervision_publish_event_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::register_observer_action,
    supervision_register_observer_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::unregister_observer_action,
    supervision_unregister_observer_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::query_state_action,
    supervision_query_state_action)
