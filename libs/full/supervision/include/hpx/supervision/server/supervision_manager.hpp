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
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/supervision/supervision_api.hpp>

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace hpx::supervision::server {

    namespace detail {

        // Testing infrastructure support
        HPX_CXX_EXPORT HPX_EXPORT void set_register_observer_snapshot_hook(
            std::function<void()> hook);
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    // Base name used to register the component
    HPX_CXX_EXPORT inline constexpr char const* const supervision_manager_name =
        "supervision_manager/";

    // An observer registered for a target, optionally scoped to a single
    // epoch: if epoch_filter is engaged, only notifications whose epoch
    // matches are delivered to agent, notifications for any other epoch are
    // skipped for this observer.
    struct observer_entry
    {
        hpx::id_type agent;
        std::optional<std::uint64_t> epoch_filter;
    };

    struct supervision_manager
      : hpx::components::fixed_component_base<supervision_manager>
    {
        using base_type = components::fixed_component_base<supervision_manager>;

        supervision_manager()
          : base_type(supervision::detail::supervision_manager_msb,
                supervision::detail::supervision_manager_lsb)
        {
        }

        void finalize() const;

        void register_server_instance(char const* service_name,
            std::uint32_t locality_id, error_code& ec = throws);
        void unregister_server_instance(error_code& ec = throws) const;

        // Supervision API implementation
        publish_result publish_event(
            hpx::id_type const& target, event ev, std::uint64_t epoch);

        struct publish_event_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::publish_event),
                &supervision_manager::publish_event, publish_event_action>
        {
        };

        lifecycle_state query_state(hpx::id_type const& target);

        struct query_state_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::query_state),
                &supervision_manager::query_state, query_state_action>
        {
        };

        hpx::id_type register_observer(hpx::id_type const& target,
            hpx::id_type const& agent,
            std::optional<std::uint64_t> epoch_filter = std::nullopt);

        struct register_observer_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::register_observer),
                &supervision_manager::register_observer,
                register_observer_action>
        {
        };

        void unregister_observer(hpx::id_type const& observer_handle);

        struct unregister_observer_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::unregister_observer),
                &supervision_manager::unregister_observer,
                unregister_observer_action>
        {
        };

    protected:
        hpx::future<void> fire_events(hpx::id_type const& target,
            lifecycle_event_notification const& notification);
        hpx::future<void> fire_event(hpx::id_type const& target,
            hpx::id_type const& agent,
            lifecycle_event_notification notification);

        void record_error(hpx::id_type const& target,
            std::uint64_t expected_sequence_number, hpx::error_code const& ec);

        void unregister_observer_target(
            hpx::id_type const& target, hpx::id_type const& observer_handle);

    private:
        mutable hpx::spinlock mtx_;
        std::string instance_name_;

        // registered events: targets -> states
        std::map<hpx::id_type, lifecycle_state> states_;
        // current epoch per target: targets -> epoch
        std::map<hpx::id_type, std::uint64_t> current_epoch_;
        // registered observers: targets -> observer entries (agent +
        // optional epoch filter)
        std::map<hpx::id_type, std::vector<observer_entry>> observers_;
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
