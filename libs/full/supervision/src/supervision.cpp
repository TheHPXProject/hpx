//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/supervision/server/agent.hpp>
#include <hpx/supervision/server/supervision_manager.hpp>
#include <hpx/supervision/supervision_api.hpp>
#include <hpx/supervision/supervision_manager.hpp>

#include <hpx/config/warnings_prefix.hpp>

using hpx::supervision::server::supervision_manager;

#include <exception>

HPX_DEFINE_COMPONENT_NAME(supervision_manager, hpx_supervision_manager)
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(supervision_manager,
    to_int(components::component_enum_type::supervision_manager))

HPX_REGISTER_ACTION_ID(
    hpx::supervision::server::supervision_manager::publish_event_action,
    supervision_manager_publish_event_action,
    hpx::actions::supervision_manager_publish_event_action_id)
HPX_REGISTER_ACTION_ID(
    hpx::supervision::server::supervision_manager::register_observer_action,
    supervision_manager_register_observer_action,
    hpx::actions::supervision_manager_register_observer_action_id)
HPX_REGISTER_ACTION_ID(
    hpx::supervision::server::supervision_manager::unregister_observer_action,
    supervision_manager_unregister_observer_action,
    hpx::actions::supervision_manager_unregister_observer_action_id)
HPX_REGISTER_ACTION_ID(
    hpx::supervision::server::supervision_manager::query_state_action,
    supervision_manager_query_state_action,
    hpx::actions::supervision_manager_query_state_action_id)

namespace hpx::supervision {

    ///////////////////////////////////////////////////////////////////////////
    bool is_valid_transition(event const from, event const to) noexcept
    {
        switch (from)
        {
        case event::unknown:
            // the first event recorded for a target must be `started`
            return to == event::started;

        case event::started:
            return to == event::started || to == event::running ||
                to == event::failed || to == event::losing_locality;

        case event::running:
            return to == event::running || to == event::suspending ||
                to == event::completed || to == event::failed ||
                to == event::losing_locality;

        case event::suspending:
            return to == event::suspending || to == event::running ||
                to == event::completed || to == event::failed ||
                to == event::losing_locality;

        case event::losing_locality:
            // a locality that is going away can only end in failure
            return to == event::failed;

        case event::completed:
        case event::failed:
            // terminal states, no outgoing transitions
            return false;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Publish a lifecycle event from within an actor or action
    hpx::future<publish_result> publish_event(hpx::id_type const& locality,
        hpx::id_type const& target, hpx::supervision::event const ev)
    {
        if (!hpx::naming::is_locality(locality))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::publish_event",
                "The id passed as the first argument is not representing "
                "a locality");
        }
        if (!target)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::publish_event",
                "The id passed as the second argument is not representing "
                "a valid target");
        }

        // handle local requests locally
        if (locality.get_gid() == agas::get_locality())
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    auto result =
                        get_supervision_manager().publish_event(target, ev);
                    return hpx::make_ready_future(result);
                },
                [&](std::exception_ptr const& ep) {
                    return hpx::make_exceptional_future<publish_result>(ep);
                });
        }

        auto dest =
            hpx::id_type(supervision_manager::get_service_instance(locality),
                hpx::id_type::management_type::unmanaged);
        using action_type = server::supervision_manager::publish_event_action;
        return hpx::async(action_type(), dest, target, ev);
    }

    publish_result publish_event(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::id_type const& target,
        hpx::supervision::event const ev, hpx::error_code& ec)
    {
        return publish_event(locality, target, ev).get(ec);
    }

    // purely local request
    publish_result publish_event(hpx::id_type const& target,
        hpx::supervision::event const ev, hpx::error_code& ec)
    {
        if (!target)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "hpx::supervision::publish_event",
                "The id passed as the first argument is not representing "
                "a valid target");
            return publish_result::already_terminal;
        }
        return get_supervision_manager().publish_event(target, ev, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Completion Query
    void serialize(hpx::serialization::output_archive& ar,
        lifecycle_state const& state, unsigned int)
    {
        ar << state.actor << state.last_event << state.timestamp
           << state.event_sequence_number << state.ec;
    }

    void serialize(hpx::serialization::input_archive& ar,
        lifecycle_state& state, unsigned int)
    {
        ar >> state.actor >> state.last_event >> state.timestamp >>
            state.event_sequence_number >> state.ec;
    }

    hpx::future<lifecycle_state> query_state(
        hpx::id_type const& locality, hpx::id_type const& target)
    {
        if (!hpx::naming::is_locality(locality))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::query_state",
                "The id passed as the first argument is not representing "
                "a locality");
        }
        if (!target)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::query_state",
                "The id passed as the second argument is not representing "
                "a valid target");
        }

        // handle local requests locally
        if (locality.get_gid() == agas::get_locality())
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    auto result = get_supervision_manager().query_state(target);
                    return hpx::make_ready_future(HPX_MOVE(result));
                },
                [&](std::exception_ptr const& ep) {
                    return hpx::make_exceptional_future<lifecycle_state>(ep);
                });
        }

        auto dest =
            hpx::id_type(supervision_manager::get_service_instance(locality),
                hpx::id_type::management_type::unmanaged);
        using action_type = server::supervision_manager::query_state_action;
        return hpx::async(action_type(), dest, target);
    }

    lifecycle_state query_state(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::id_type const& target,
        hpx::error_code& ec)
    {
        return query_state(locality, target).get(ec);
    }

    lifecycle_state query_state(hpx::id_type const& target, hpx::error_code& ec)
    {
        if (!target)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "hpx::supervision::query_state",
                "The id passed as the first argument is not representing "
                "a valid target");
            return {};
        }
        return get_supervision_manager().query_state(target, ec);
    }

    /////////////////////////////////////////////////////////////////////////////
    void serialize(hpx::serialization::output_archive& ar,
        lifecycle_event_notification const& notification, unsigned int)
    {
        ar << notification.actor << notification.event
           << notification.event_time << notification.event_sequence_number
           << notification.ec;
    }

    void serialize(hpx::serialization::input_archive& ar,
        lifecycle_event_notification& notification, unsigned int)
    {
        ar >> notification.actor >> notification.event >>
            notification.event_time >> notification.event_sequence_number >>
            notification.ec;
    }

    // Register a callback to observe lifecycle events from a target actor
    hpx::future<hpx::id_type> register_observer(hpx::id_type const& locality,
        hpx::id_type const& target, lifecycle_callback const& callback)
    {
        if (!hpx::naming::is_locality(locality))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::register_observer",
                "The id passed as the first argument is not representing "
                "a locality");
        }
        if (!target)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::register_observer",
                "The id passed as the second argument is not representing "
                "a valid target");
        }

        auto agent = server::create_agent(callback);

        // handle local requests locally
        if (locality.get_gid() == agas::get_locality())
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    auto result = get_supervision_manager().register_observer(
                        target, agent);
                    return hpx::make_ready_future(result);
                },
                [&](std::exception_ptr const& ep) {
                    return hpx::make_exceptional_future<hpx::id_type>(ep);
                });
        }

        auto dest =
            hpx::id_type(supervision_manager::get_service_instance(locality),
                hpx::id_type::management_type::unmanaged);
        using action_type =
            server::supervision_manager::register_observer_action;
        return hpx::async(action_type(), dest, target, agent);
    }

    hpx::id_type register_observer(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::id_type const& target,
        lifecycle_callback const& callback, hpx::error_code& ec)
    {
        return register_observer(locality, target, callback).get(ec);
    }

    hpx::id_type register_observer(hpx::id_type const& target,
        lifecycle_callback const& callback, hpx::error_code& ec)
    {
        if (!target)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "hpx::supervision::register_observer",
                "The id passed as the first argument is not representing "
                "a valid target");
            return hpx::invalid_id;
        }
        auto agent = server::create_agent(callback);
        return get_supervision_manager().register_observer(
            target, HPX_MOVE(agent), ec);
    }

    // Register a callback
    hpx::future<void> unregister_observer(
        hpx::id_type const& locality, hpx::id_type const& observer_handle)
    {
        if (!hpx::naming::is_locality(locality))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::unregister_observer",
                "The id passed as the first argument is not representing "
                "a locality");
        }
        if (!observer_handle)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::supervision::unregister_observer",
                "The id passed as the second argument is not representing "
                "a valid observer handle");
        }

        // handle local requests locally
        if (locality.get_gid() == agas::get_locality())
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    get_supervision_manager().unregister_observer(
                        observer_handle);
                    return hpx::make_ready_future();
                },
                [&](std::exception_ptr const& ep) {
                    return hpx::make_exceptional_future<void>(ep);
                });
        }

        auto dest =
            hpx::id_type(supervision_manager::get_service_instance(locality),
                hpx::id_type::management_type::unmanaged);
        using action_type =
            server::supervision_manager::unregister_observer_action;
        return hpx::async(action_type(), dest, observer_handle);
    }

    void unregister_observer(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::id_type const& observer_handle,
        hpx::error_code& ec)
    {
        unregister_observer(locality, observer_handle).get(ec);
    }

    // Local callback unregistration
    void unregister_observer(
        hpx::id_type const& observer_handle, hpx::error_code& ec)
    {
        if (!observer_handle)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "hpx::supervision::unregister_observer",
                "The id passed as the first argument is not representing "
                "a valid observer handle");
            return;
        }
        get_supervision_manager().unregister_observer(observer_handle, ec);
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
