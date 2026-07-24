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

    void supervision_manager::record_error(hpx::id_type const& target,
        std::uint64_t const expected_sequence_number, hpx::error_code const& ec)
    {
        std::unique_lock<hpx::spinlock> l(mtx_);

        // Only apply the failure if the state that was being delivered when the
        // failure occurred is still the current state for this target.
        // Otherwise, a newer publish_event may have already superseded it, and
        // recording the (now stale) failure would incorrectly stomp that newer
        // state.
        if (auto const it = states_.find(target); it != states_.end() &&
            it->second.event_sequence_number == expected_sequence_number)
        {
            it->second.last_event = supervision::event::failed;
            it->second.timestamp = std::chrono::steady_clock::now();
            it->second.event_sequence_number =
                it->second.event_sequence_number + 1;
            it->second.ec = ec;
        }
    }

    publish_result supervision_manager::publish_event(
        hpx::id_type const& target, event const ev, std::uint64_t epoch)
    {
        lifecycle_event_notification notification;
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            auto const epoch_it = current_epoch_.find(target);
            std::uint64_t const current_ep =
                epoch_it != current_epoch_.end() ? epoch_it->second : 0;

            if (epoch < current_ep)
            {
                // stale/out-of-order publication for an epoch that has
                // already been superseded: reject without mutating state or
                // notifying observers
                return publish_result::stale_epoch;
            }

            auto const it = states_.find(target);

            if (epoch > current_ep)
            {
                // Entering a new epoch resets the target's sequence number
                // and unconditionally records the event: a higher epoch
                // starts a fresh lifecycle for the target, regardless of
                // what was last recorded for the previous epoch.
                current_epoch_[target] = epoch;

                lifecycle_state const state = {.actor = target,
                    .last_event = ev,
                    .timestamp = std::chrono::steady_clock::now(),
                    .event_sequence_number = 1,
                    .epoch = epoch};
                states_[target] = state;

                notification = {.actor = target,
                    .event = state.last_event,
                    .event_time = state.timestamp,
                    .event_sequence_number = state.event_sequence_number,
                    .epoch = state.epoch,
                    .ec = state.ec};
            }
            else
            {
                // epoch == current_ep: apply within the target's current epoch,
                // subject to the terminal-state latch and lifecycle transition
                // validation.
                event const prev_event = it != states_.end() ?
                    it->second.last_event :
                    event::unknown;

                // A terminal event (completed/failed) was reached, absorb any
                // further event without mutating state or notifying observers
                // again.
                if (is_terminal(prev_event) && is_terminal(ev))
                {
                    // exactly-once completion: the target already reached a
                    // terminal event, ignore this (duplicate) publication
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
                    .event_sequence_number = 1,
                    .epoch = epoch};

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
                    .epoch = state.epoch,
                    .ec = state.ec};
            }
        }

        // now fire event for all observers of this target
        auto f = fire_events(target, notification);
        try
        {
            f.get();
        }
        catch (...)
        {
            record_error(target, notification.event_sequence_number,
                hpx::make_error_code(std::current_exception()));
        }

        return publish_result::applied;
    }

    hpx::future<void> supervision_manager::fire_events(
        hpx::id_type const& target,
        lifecycle_event_notification const& notification)
    {
        std::vector<observer_entry> observers;
        {
            std::unique_lock<hpx::spinlock> l(mtx_);
            if (auto const it = observers_.find(target); it != observers_.end())
            {
                observers = it->second;
            }
        }

        hpx::future<void> f;
        for (auto const& observer : observers)
        {
            // An observer scoped to a specific epoch does not get notified of
            // events published under any other epoch.
            if (auto epoch_filter = observer.epoch_filter;
                epoch_filter.has_value() && *epoch_filter != notification.epoch)
            {
                continue;
            }

            auto agent = observer.agent;

            if (f.valid())
            {
                f = f.then([this, target, agent, notification](
                               hpx::future<void>&& prev_f) {
                    try
                    {
                        prev_f.get();
                    }
                    catch (...)
                    {
                        record_error(target, notification.event_sequence_number,
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
        lifecycle_event_notification notification)
    {
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // check again if the agent is still registered as an observer for
            // the given target
            auto const it2 = observers_.find(target);
            if (it2 == observers_.end())
            {
                return hpx::make_ready_future(true);
            }

            if (std::ranges::find(it2->second, agent, &observer_entry::agent) ==
                it2->second.end())
            {
                return hpx::make_ready_future(true);
            }
        }

        // prevent the agent callback from being run inline as it may block
        using action_type = agent_component::invoke_if_active_action;
        auto fut = hpx::async(
            hpx::launch::task, action_type(), agent, HPX_MOVE(notification));

        return fut.then([this, target, agent](hpx::future<bool>&& f) mutable {
            bool keep_registered = true;
            std::exception_ptr ep;
            try
            {
                keep_registered = f.get();
            }
            catch (...)
            {
                ep = std::current_exception();
            }

            if (!keep_registered)
            {
                // remove observer from the given target
                std::unique_lock<hpx::spinlock> l(mtx_);

                unregister_observer_target(target, agent);
                if (auto const it = agents_.find(agent); it != agents_.end())
                {
                    agents_.erase(it);
                }
            }

            // rethrow exception, if any
            if (ep)
            {
                std::rethrow_exception(ep);
            }
        });
    }

    hpx::id_type supervision_manager::register_observer(
        hpx::id_type const& target, hpx::id_type const& agent,
        std::optional<std::uint64_t> epoch_filter)
    {
        std::optional<lifecycle_event_notification> initial_notification;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // insert observer into table of registered observers
            auto it = observers_.find(target);
            if (it == observers_.end())
            {
                auto [it2, inserted] = observers_.insert(
                    std::make_pair(target, std::vector<observer_entry>()));
                if (!inserted)
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::no_success,
                        "supervision_manager::register_observer",
                        "failed to register observer");
                }
                it = it2;
            }
            else if (std::ranges::find(it->second, agent,
                         &observer_entry::agent) != it->second.end())
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::no_success,
                    "supervision_manager::register_observer",
                    "observer already registered for target");
            }

            it->second.push_back(observer_entry{agent, epoch_filter});

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
            // A freshly-registered observer scoped to a specific epoch
            // (epoch_filter engaged) does not receive this synchronous
            // snapshot if it belongs to a different epoch, for consistency
            // with the filtering applied in fire_events.
            if (auto const it3 = states_.find(target); it3 != states_.end())
            {
                if (lifecycle_state const& state = it3->second;
                    !epoch_filter.has_value() || *epoch_filter == state.epoch)
                {
                    initial_notification = lifecycle_event_notification{
                        .actor = target,
                        .event = state.last_event,
                        .event_time = state.timestamp,
                        .event_sequence_number = state.event_sequence_number,
                        .epoch = state.epoch,
                        .ec = state.ec};
                }
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
            // Capture the sequence number before the notification is moved out
            // below, so record_error() can still compare it against the
            // (possibly since-updated) state for target.
            auto const initial_sequence =
                initial_notification->event_sequence_number;

            auto f = fire_event(target, agent, HPX_MOVE(*initial_notification));
            try
            {
                f.get();
            }
            catch (...)
            {
                record_error(target, initial_sequence,
                    hpx::make_error_code(std::current_exception()));
            }
        }
        return agent;
    }

    void supervision_manager::unregister_observer_target(
        hpx::id_type const& target, hpx::id_type const& observer_handle)
    {
        if (auto const it = observers_.find(target); it != observers_.end())
        {
            // it2 refers to observer in observer target list

            // delete observer from list
            if (auto const it2 = std::ranges::find(
                    it->second, observer_handle, &observer_entry::agent);
                it2 != it->second.end())
            {
                it->second.erase(it2);
            }

            // delete list of targets from given observer
            if (it->second.empty())
            {
                observers_.erase(it);
            }
        }
    }

    void supervision_manager::unregister_observer(
        hpx::id_type const& observer_handle)
    {
        {
            // remove observer from all targets
            std::unique_lock<hpx::spinlock> l(mtx_);

            // locate targets the given observer was registered to
            if (auto const it = agents_.find(observer_handle);
                it != agents_.end())
            {
                // remove observer from all targets
                for (hpx::id_type const& target : it->second)
                {
                    unregister_observer_target(target, observer_handle);
                }
                agents_.erase(it);
            }
        }

        // A delivery action that was already queued may still reach the agent.
        // deactivate_and_wait fences such actions and drains any callback that
        // had already begun before this call returns.

        // prevent the agent callback from being run inline as it may block
        using action_type = agent_component::deactivate_and_wait_action;
        hpx::async(hpx::launch::task, action_type(), observer_handle).get();
    }

    lifecycle_state supervision_manager::query_state(hpx::id_type const& target)
    {
        {
            std::scoped_lock<hpx::spinlock> l(mtx_);
            if (auto const it = states_.find(target); it != states_.end())
            {
                return it->second;
            }
        }

        // No event has ever been recorded for this target: the returned
        // (default) state may be stale, e.g. because the corresponding
        // publication has not yet been observed on this locality.
        return {.actor = target,
            .timestamp = std::chrono::steady_clock::now(),
            .ec = hpx::error_code(hpx::error::stale_state,
                "no locally recorded lifecycle state is available for the "
                "requested target, the returned state may be stale",
                hpx::throwmode::lightweight)};
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
