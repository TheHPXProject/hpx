//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/supervision/server/agent.hpp>
#include <hpx/supervision/server/supervision_manager.hpp>
#include <hpx/supervision/supervision_api.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace hpx::supervision::server {

    namespace {

        // Testing infrastructure support
        hpx::spinlock register_observer_hook_mtx;
        std::function<void()> register_observer_snapshot_hook;

        std::function<void()> get_register_observer_snapshot_hook()
        {
            std::lock_guard<hpx::spinlock> l(register_observer_hook_mtx);
            return register_observer_snapshot_hook;
        }
    }    // namespace

    namespace detail {

        void set_register_observer_snapshot_hook(std::function<void()> hook)
        {
            std::lock_guard<hpx::spinlock> l(register_observer_hook_mtx);
            register_observer_snapshot_hook = HPX_MOVE(hook);
        }
    }    // namespace detail

    void supervision_manager::finalize() const
    {
        if (!instance_name_.empty())
        {
            error_code ec(throwmode::lightweight);
            agas::unregister_name(launch::sync, instance_name_, ec);
        }
    }

    // event::completed and event::failed are terminal: once recorded,
    // a target's state is latched and further publications for that
    // target are no-ops (see supervision_manager::publish_event).
    constexpr bool is_terminal(event const ev) noexcept
    {
        return ev == event::completed || ev == event::failed;
    }

    void supervision_manager::record_error(
        hpx::id_type const& target, hpx::error_code const& ec)
    {
        std::unique_lock<hpx::spinlock> l(mtx_);

        if (auto const it = states_.find(target); it != states_.end())
        {
            it->second.last_event = supervision::event::failed;
            it->second.timestamp = std::chrono::steady_clock::now();
            it->second.event_sequence_number =
                it->second.event_sequence_number + 1;
            it->second.ec = ec;
        }
    }

    publish_result supervision_manager::publish_event(
        hpx::id_type const& target, event const ev)
    {
        lifecycle_event_notification notification;
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            auto const it = states_.find(target);
            event const prev_event =
                it != states_.end() ? it->second.last_event : event::unknown;

            // A terminal event (completed/failed) was reached, absorb any
            // further event without mutating state or notifying observers again.
            if (is_terminal(prev_event) && is_terminal(ev))
            {
                // exactly-once completion: the target already reached
                // a terminal event, ignore this (duplicate) publication
                return publish_result::already_terminal;
            }

            if (!is_valid_transition(prev_event, ev))
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "supervision_manager::publish_event",
                    "invalid lifecycle event transition");
            }

            lifecycle_state state = {.actor = target,
                .last_event = ev,
                .timestamp = std::chrono::steady_clock::now(),
                .event_sequence_number = 1};

            if (it != states_.end())
            {
                state.event_sequence_number =
                    it->second.event_sequence_number + 1;
                it->second = state;
            }
            else
            {
                auto const [_, inserted] =
                    states_.insert(std::make_pair(target, state));
                if (!inserted)
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::no_success,
                        "supervision_manager::publish_event",
                        "failed to insert event");
                }
            }

            notification = {.actor = target,
                .event = state.last_event,
                .event_time = state.timestamp,
                .event_sequence_number = state.event_sequence_number,
                .ec = state.ec};
        }

        // now fire event for all observers of this target
        auto f = fire_events(target, notification);
        try
        {
            f.get();
        }
        catch (...)
        {
            record_error(
                target, hpx::make_error_code(std::current_exception()));
        }

        return publish_result::applied;
    }

    hpx::future<void> supervision_manager::fire_events(
        hpx::id_type const& target,
        lifecycle_event_notification const& notification)
    {
        std::vector<hpx::id_type> observers;
        {
            std::unique_lock<hpx::spinlock> l(mtx_);
            if (auto const it = observers_.find(target); it != observers_.end())
            {
                observers = it->second;
            }
        }

        hpx::future<void> f;
        for (hpx::id_type const& agent : observers)
        {
            if (f.valid())
            {
                f = f.then([&, this, target, agent, notification](
                               hpx::future<void>&& prev_f) {
                    try
                    {
                        prev_f.get();
                    }
                    catch (...)
                    {
                        record_error(target,
                            hpx::make_error_code(std::current_exception()));
                    }
                    return fire_event(target, agent, notification);
                });
            }
            else
            {
                f = fire_event(target, agent, notification);
            }
        }

        if (!f.valid())
        {
            f = hpx::make_ready_future();
        }
        return f;
    }

    hpx::future<void> supervision_manager::fire_event(
        hpx::id_type const& target, hpx::id_type const& agent,
        lifecycle_event_notification notification) const
    {
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // check again if the agent is still registered as an observer for
            // the given target
            auto const it2 = observers_.find(target);
            if (it2 == observers_.end())
            {
                return hpx::make_ready_future();
            }

            if (std::ranges::find(it2->second, agent) == it2->second.end())
            {
                return hpx::make_ready_future();
            }
        }

        using action_type = agent_component::invoke_if_active_action;
        return hpx::async(action_type(), agent, HPX_MOVE(notification));
    }

    hpx::id_type supervision_manager::register_observer(
        hpx::id_type const& target, hpx::id_type const& agent)
    {
        std::optional<lifecycle_event_notification> initial_notification;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // insert observer into table of registered observers
            auto it = observers_.find(target);
            if (it == observers_.end())
            {
                auto [it2, inserted] = observers_.insert(
                    std::make_pair(target, std::vector<hpx::id_type>()));
                if (!inserted)
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::no_success,
                        "supervision_manager::register_observer",
                        "failed to register observer");
                }
                it = it2;
            }
            else if (std::ranges::find(it->second, agent) != it->second.end())
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::no_success,
                    "supervision_manager::register_observer",
                    "observer already registered for target");
            }

            it->second.push_back(agent);

            // insert into inverse lookup table as well
            auto it2 = agents_.find(agent);
            if (it2 == agents_.end())
            {
                auto const [it3, inserted] = agents_.insert(
                    std::make_pair(agent, std::vector<hpx::id_type>()));
                if (!inserted)
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::no_success,
                        "supervision_manager::register_observer",
                        "failed to register observer");
                }
                it2 = it3;
            }

            // Sanity check: the observers_/agents_ invariant should already
            // guarantee this can't happen given the check above; catches future
            // code paths that mutate one map without the other.
            HPX_ASSERT(
                std::ranges::find(it2->second, target) == it2->second.end());

            it2->second.push_back(target);

            // An existing target gets an initial notification. Keep a value
            // snapshot: do not let a subsequent publish replace its contents.
            if (auto const it3 = states_.find(target); it3 != states_.end())
            {
                lifecycle_state const& state = it3->second;
                initial_notification =
                    lifecycle_event_notification{.actor = target,
                        .event = state.last_event,
                        .event_time = state.timestamp,
                        .event_sequence_number = state.event_sequence_number,
                        .ec = state.ec};
            }
        }

        // Testing infrastructure support
        if (auto const hook = get_register_observer_snapshot_hook())
        {
            hook();
        }

        // now fire event for new observer; dispatched asynchronously, see
        // fire_event
        if (initial_notification)
        {
            auto f = fire_event(target, agent, HPX_MOVE(*initial_notification));
            try
            {
                f.get();
            }
            catch (...)
            {
                record_error(
                    target, hpx::make_error_code(std::current_exception()));
            }
        }
        return agent;
    }

    void supervision_manager::unregister_observer(
        hpx::id_type const& observer_handle)
    {
        // remove observer from all targets
        std::unique_lock<hpx::spinlock> l(mtx_);

        // locate targets the given observer was registered to
        if (auto const it = agents_.find(observer_handle); it != agents_.end())
        {
            // remove observer from all targets
            for (hpx::id_type const& target : it->second)
            {
                if (auto it2 = observers_.find(target); it2 != observers_.end())
                {
                    // it3 refers to observer in list

                    // delete observer from list
                    if (auto const it3 =
                            std::ranges::find(it2->second, observer_handle);
                        it3 != it2->second.end())
                    {
                        it2->second.erase(it3);
                    }

                    // delete list of observers from given target
                    if (it2->second.empty())
                    {
                        observers_.erase(it2);
                    }
                }
            }
            agents_.erase(it);
        }

        l.unlock();

        // A delivery action that was already queued may still reach the agent.
        // deactivate_and_wait fences such actions and drains any callback that
        // had already begun before this call returns.
        using action_type = agent_component::deactivate_and_wait_action;
        hpx::async(action_type(), observer_handle).get();
    }

    lifecycle_state supervision_manager::query_state(hpx::id_type const& target)
    {
        std::scoped_lock<hpx::spinlock> l(mtx_);
        if (auto const it = states_.find(target); it != states_.end())
        {
            return it->second;
        }

        // No event has ever been recorded for this target: the returned
        // (default) state may be stale, e.g. because the corresponding
        // publication has not yet been observed on this locality.
        return {.actor = target,
            .ec = hpx::error_code(hpx::error::stale_state,
                "no locally recorded lifecycle state is available for the "
                "requested target, the returned state may be stale")};
    }

    void supervision_manager::register_server_instance(
        char const* service_name, std::uint32_t locality_id, error_code& ec)
    {
        // set locality_id for this component
        if (locality_id == naming::invalid_locality_id)
            locality_id = 0;    // if not given, we're on the root

        this->base_type::set_locality_id(locality_id);

        // now register this supervision instance with AGAS
        instance_name_ = supervision::service_name;
        instance_name_ += service_name;
        instance_name_ += supervision::server::supervision_manager_name;

        auto const gid = get_unmanaged_id().get_gid();
        //naming::gid_type const manager_gid(
        //    gid.get_msb(), reinterpret_cast<std::uint64_t>(this));
        naming::address const manager_address(agas::get_locality(),
            components::get_component_type<
                supervision::server::supervision_manager>(),
            this);
        agas::bind_gid_local(gid, manager_address, ec);
        if (ec)
            return;

        // register a gid (not the id) to avoid AGAS holding a reference to this
        // component
        agas::register_name(launch::sync, instance_name_, gid, ec);
    }

    void supervision_manager::unregister_server_instance(error_code& ec) const
    {
        agas::unregister_name(launch::sync, instance_name_, ec);
        this->base_type::finalize();
    }

}    // namespace hpx::supervision::server
