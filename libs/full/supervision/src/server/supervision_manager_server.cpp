//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/format.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/supervision/server/activity_agent.hpp>
#include <hpx/supervision/server/agent.hpp>
#include <hpx/supervision/server/supervision_manager.hpp>
#include <hpx/supervision/supervision_api.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <map>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
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

        // Default deadline (relative to registration time) given to
        // await_terminal() waiters, bounding how long an abandoned waiter
        // (dropped future, or target that never publishes another event) can
        // remain in waiters_ before sweep_expired_waiters() erases it. Used
        // whenever await_terminal() is called with its `timeout` parameter
        // left at its sentinel value (see
        // supervision_manager::await_terminal).
        constexpr std::int64_t default_await_terminal_timeout_ms = 60000;
    }    // namespace

    namespace detail {

        void set_register_observer_snapshot_hook(std::function<void()> hook)
        {
            std::lock_guard<hpx::spinlock> l(register_observer_hook_mtx);
            register_observer_snapshot_hook = HPX_MOVE(hook);
        }
    }    // namespace detail

    supervision_manager::supervision_manager()
      : base_type(supervision::detail::supervision_manager_msb,
            supervision::detail::supervision_manager_lsb)
    {
    }

    supervision_manager::~supervision_manager()
    {
        // Cancel the sweep timer first. This alone is not sufficient to make
        // teardown safe, though: sweep_timer_callback() captures a raw
        // `this` and runs asynchronously on a thread pool, and
        // pool_timer::stop() only cancels a not-yet-fired wait, it does not
        // wait for an already-dispatched callback invocation to finish. So
        // also set shutting_down_ (checked by sweep_timer_callback() under
        // mtx_ before it touches anything else) and wait for
        // callbacks_in_flight_ to drain to zero, guaranteeing no invocation
        // is still running before any other member (in particular mtx_ and
        // waiters_) starts being destroyed.
        {
            std::lock_guard<hpx::spinlock> timer_l(sweep_timer_mtx_);
            if (sweep_timer_.is_valid())
            {
                sweep_timer_.stop();
            }
        }

        std::unique_lock<hpx::spinlock> l(mtx_);
        shutting_down_ = true;

        while (callbacks_in_flight_ > 0)
        {
            shutdown_cv_.wait(l);
        }
    }

    void supervision_manager::record_error(hpx::id_type const& target,
        std::uint64_t const expected_sequence_number, hpx::error_code const& ec)
    {
        lifecycle_event_notification notification;
        waiters_t to_resolve;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // Only apply the failure if the state that was being delivered when
            // the failure occurred is still the current state for this target.
            // Otherwise, a newer publish_event may have already superseded it,
            // and recording the (now stale) failure would incorrectly stomp
            // that newer state.
            if (auto const it = states_.find(target); it != states_.end() &&
                it->second.event_sequence_number == expected_sequence_number)
            {
                it->second.last_event = supervision::event::failed;
                it->second.timestamp = std::chrono::steady_clock::now();
                it->second.event_sequence_number =
                    it->second.event_sequence_number + 1;
                it->second.ec = ec;

                // The target just transitioned to a terminal failed state as a
                // result of observer delivery failing; resolve any
                // await_terminal() waiters registered for this (target, epoch)
                // accordingly.
                to_resolve = drain_terminal_waiters_locked(
                    l, target, it->second.epoch, it->second.last_event);

                notification = lifecycle_event_notification{.actor = target,
                    .event = it->second.last_event,
                    .event_time = it->second.timestamp,
                    .event_sequence_number = it->second.event_sequence_number,
                    .epoch = it->second.epoch,
                    .ec = it->second.ec};
            }
        }

        // always call resolve_terminal_waiters(), it returns right away if the
        // to_resolve list is empty
        resolve_terminal_waiters(notification, to_resolve);
    }

    // Drains any await_terminal() waiters registered for this exact (target,
    // epoch) pair once it has reached a terminal event; a no-op otherwise. The
    // returned promises are resolved by the caller after the lock has been
    // released.
    supervision_manager::waiters_t
    supervision_manager::drain_terminal_waiters_locked(
        std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
        std::uint64_t const epoch, event const last_event)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        waiters_t to_resolve;
        if (is_terminal(last_event))
        {
            if (auto const wit =
                    waiters_.find(waiter_key{.target = target, .epoch = epoch});
                wit != waiters_.end())
            {
                to_resolve = HPX_MOVE(wit->second);
                waiters_.erase(wit);
                recompute_earliest_deadline_locked(l);
            }
        }
        return to_resolve;
    }

    // Drains all await_terminal() waiters registered for this target at an
    // epoch below `epoch`, i.e. waiters that are now stale because the
    // target has entered a newer epoch. The caller invalidates the returned
    // promises after the lock has been released.
    supervision_manager::stale_waiters_t
    supervision_manager::drain_stale_waiters_locked(
        std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
        std::uint64_t const epoch)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        stale_waiters_t stale;

        auto const lo =
            waiters_.lower_bound(waiter_key{.target = target, .epoch = 0});
        auto const hi =
            waiters_.lower_bound(waiter_key{.target = target, .epoch = epoch});
        for (auto it = lo; it != hi; /**/)
        {
            stale.emplace_back(it->first.epoch, HPX_MOVE(it->second));
            it = waiters_.erase(it);
        }

        if (!stale.empty())
        {
            recompute_earliest_deadline_locked(l);
        }

        return stale;
    }

    // Recomputes earliest_deadline_ from scratch by folding over every
    // remaining waiter in waiters_. Called by drain_terminal_waiters_locked()
    // and drain_stale_waiters_locked(), which only touch a single target/epoch
    // bucket (or a small range of buckets) and therefore cannot otherwise tell
    // whether the waiter(s) they just erased held the current earliest
    // deadline. drain_expired_waiters_locked() does not use this helper: since
    // it already performs a full scan of waiters_, it derives the new earliest
    // deadline directly as a byproduct of that scan instead.
    void supervision_manager::recompute_earliest_deadline_locked(
        std::unique_lock<hpx::spinlock>& l)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        auto next_deadline = (std::chrono::steady_clock::time_point::max) ();
        for (auto const& val : waiters_ | std::views::values)
        {
            for (auto const& [_, deadline] : val)
            {
                next_deadline = (std::min) (next_deadline, deadline);
            }
        }
        earliest_deadline_ = next_deadline;
    }

    // Entering a new epoch resets the target's sequence number and
    // unconditionally records the event: a higher epoch starts a fresh
    // lifecycle for the target, regardless of what was last recorded for the
    // previous epoch.
    supervision_manager::apply_result
    supervision_manager::apply_new_epoch_locked(
        std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
        supervision::event const ev, std::uint64_t const epoch)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        if (!is_valid_transition(event::unknown, ev))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "supervision_manager::apply_new_epoch_locked",
                "invalid lifecycle event transition: a new epoch must begin "
                "with a started event");
        }

        lifecycle_state const state = {.actor = target,
            .last_event = ev,
            .timestamp = std::chrono::steady_clock::now(),
            .event_sequence_number = 1,
            .epoch = epoch};
        states_[target] = state;

        auto to_resolve =
            drain_terminal_waiters_locked(l, target, epoch, state.last_event);

        lifecycle_event_notification notification = {.actor = target,
            .event = state.last_event,
            .event_time = state.timestamp,
            .event_sequence_number = state.event_sequence_number,
            .epoch = state.epoch,
            .ec = state.ec};

        return {.notification = HPX_MOVE(notification),
            .to_resolve = HPX_MOVE(to_resolve)};
    }

    // Applies an event within the target's current epoch, subject to the
    // terminal-state latch and lifecycle transition validation. Returns
    // std::nullopt when a duplicate terminal event is absorbed (the caller
    // translates this to publish_result::already_terminal); throws (after
    // unlocking `l`) on an invalid transition or a failed insert.
    std::optional<supervision_manager::apply_result>
    supervision_manager::apply_current_epoch_locked(
        std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
        event const ev, std::uint64_t const epoch)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        auto const it = states_.find(target);
        event const prev_event =
            it != states_.end() ? it->second.last_event : event::unknown;

        // A terminal event (completed/failed) was reached, absorb any further
        // event without mutating state or notifying observers again.
        if (is_terminal(prev_event) && is_terminal(ev))
        {
            // exactly-once completion: the target already reached a terminal
            // event, ignore this (duplicate) publication
            return std::nullopt;
        }

        if (!is_valid_transition(prev_event, ev))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "supervision_manager::apply_current_epoch_locked",
                "invalid lifecycle event transition");
        }

        lifecycle_state state = {.actor = target,
            .last_event = ev,
            .timestamp = std::chrono::steady_clock::now(),
            .event_sequence_number = 1,
            .epoch = epoch};

        if (it != states_.end())
        {
            state.event_sequence_number = it->second.event_sequence_number + 1;
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
                    "supervision_manager::apply_current_epoch_locked",
                    "failed to insert event");
            }
        }

        auto to_resolve =
            drain_terminal_waiters_locked(l, target, epoch, state.last_event);

        lifecycle_event_notification notification = {.actor = target,
            .event = state.last_event,
            .event_time = state.timestamp,
            .event_sequence_number = state.event_sequence_number,
            .epoch = state.epoch,
            .ec = state.ec};

        return apply_result{HPX_MOVE(notification), HPX_MOVE(to_resolve)};
    }

    // Walks the entire waiters_ map and drains any waiter whose deadline is
    // at or before `now`, regardless of target/epoch: unlike
    // drain_stale_waiters_locked(), this is not scoped to a single target,
    // since the expiry sweep runs opportunistically and independently of
    // any particular publish_event()/await_terminal() call. The caller
    // invalidates the returned waiters after the lock has been released.
    supervision_manager::expired_waiters_t
    supervision_manager::drain_expired_waiters_locked(
        std::unique_lock<hpx::spinlock>& l,
        std::chrono::steady_clock::time_point const now)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        expired_waiters_t expired;
        auto next_deadline = (std::chrono::steady_clock::time_point::max) ();

        for (auto it = waiters_.begin(); it != waiters_.end(); /**/)
        {
            auto& entries = it->second;
            for (auto eit = entries.begin(); eit != entries.end(); /**/)
            {
                if (eit->deadline <= now)
                {
                    expired.push_back(expired_waiter{.target = it->first.target,
                        .epoch = it->first.epoch,
                        .promise = HPX_MOVE(eit->promise)});
                    eit = entries.erase(eit);
                }
                else
                {
                    // this waiter survives the sweep; fold it into the new
                    // earliest_deadline_ as we go, instead of re-scanning
                    // waiters_ again afterward just to recompute it
                    next_deadline = (std::min) (next_deadline, eit->deadline);

                    ++eit;
                }
            }

            it = entries.empty() ? waiters_.erase(it) : std::next(it);
        }

        earliest_deadline_ = next_deadline;
        return expired;
    }

    // Invalidates any await_terminal() waiters swept by
    // drain_expired_waiters_locked(), giving each an explicit, descriptive
    // exception instead of leaving their promises to be destroyed (which would
    // set an opaque 'broken_promise' on the attached future). Must be called
    // with mtx_ not held.
    void supervision_manager::invalidate_expired_waiters(
        expired_waiters_t& expired)
    {
        for (auto& [target, epoch, promise] : expired)
        {
            auto exception = HPX_GET_EXCEPTION(hpx::error::future_cancelled,
                "supervision_manager::invalidate_expired_waiters",
                hpx::util::format(
                    "await_terminal() waiter for target {} at epoch {} was "
                    "invalidated: it timed out before the target reached a "
                    "terminal event under that epoch or the epoch was "
                    "superseded",
                    target, epoch));

            promise.set_exception(exception);
        }
    }

    supervision_manager::stale_waiters_t
    supervision_manager::drain_all_waiters_for_target_locked(
        std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        // waiters_ is keyed by (target, epoch); starting at epoch 0 and walking
        // forward while target matches picks up every epoch entry for this
        // target, not just the stale ones below some cutoff.
        stale_waiters_t stale;
        auto it =
            waiters_.lower_bound(waiter_key{.target = target, .epoch = 0});
        while (it != waiters_.end() && it->first.target == target)
        {
            // Move the waiter out before erasing so its promise can be
            // resolved (with an explicit exception) after mtx_ is released.
            stale.emplace_back(it->first.epoch, HPX_MOVE(it->second));
            it = waiters_.erase(it);
        }
        if (!stale.empty())
        {
            // The removed waiters may have held the earliest pending
            // deadline; recompute it so the timer sweep doesn't fire on
            // stale state.
            recompute_earliest_deadline_locked(l);
        }
        return stale;
    }

    // Opportunistic, lazy cleanup for abandoned await_terminal() waiters:
    // called from both await_terminal() and publish_event() so that waiters
    // never reached by the exact-epoch or epoch-supersession drain paths (e.g.
    // because the caller dropped the returned future, or the target never
    // publishes another event) are swept as a fast path, ahead of the next
    // sweep_timer_ firing.
    void supervision_manager::sweep_expired_waiters()
    {
        expired_waiters_t expired;
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // No outstanding waiter can have expired yet: skip the full
            // waiters_ scan performed by drain_expired_waiters_locked()
            // entirely.
            auto const now = std::chrono::steady_clock::now();
            if (earliest_deadline_ > now)
            {
                return;
            }

            expired = drain_expired_waiters_locked(l, now);
        }
        invalidate_expired_waiters(expired);
    }

    // Arms (or re-arms) sweep_timer_ for earliest_deadline_ unless it is
    // already armed for a deadline at or before that, or waiters_ is currently
    // empty. sweep_timer_ is one-shot (see pool_timer), so every call site that
    // can move earliest_deadline_ earlier (await_terminal()) or that needs to
    // re-arm after the timer fired (sweep_timer_callback()) goes through this
    // helper to keep armed_deadline_ in sync.
    void supervision_manager::arm_sweep_timer_locked(
        std::unique_lock<hpx::spinlock>&& l)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        auto const no_deadline =
            (std::chrono::steady_clock::time_point::max) ();
        if (earliest_deadline_ == no_deadline)
        {
            // no waiters left to bound
            armed_deadline_ = no_deadline;
            return;
        }

        if (earliest_deadline_ >= armed_deadline_)
        {
            // already armed for a deadline at or before earliest_deadline_
            return;
        }

        // Record the new deadline while still holding mtx_ so concurrent
        // callers observe a consistent armed_deadline_, but release mtx_
        // before actually touching sweep_timer_ below: pool_timer::init()/
        // stop()/start() can themselves block or touch runtime-global
        // bookkeeping (e.g. register_pre_shutdown_function() on the first
        // start()), which must not happen while this object's busy-wait
        // spinlock is held. sweep_timer_mtx_ serializes concurrent arm
        // attempts against sweep_timer_ itself while mtx_ is unlocked.
        armed_deadline_ = earliest_deadline_;

        l.unlock();

        std::lock_guard<hpx::spinlock> timer_l(sweep_timer_mtx_);

        // initialize timer for the first time, if needed
        if (!sweep_timer_.is_valid())
        {
            sweep_timer_.init(
                hpx::bind_front(
                    &supervision_manager::sweep_timer_callback, this),
                hpx::function<void()>(), "supervision_manager_sweep_timer");
        }
        else
        {
            sweep_timer_.stop();
        }

        // re-arm timer to new earliest deadline
        auto const now = std::chrono::steady_clock::now();
        sweep_timer_.start(earliest_deadline_ > now ?
                earliest_deadline_ - now :
                std::chrono::steady_clock::duration::zero());
    }

    // Guaranteed backstop for idle targets: unlike the opportunistic sweeps in
    // await_terminal()/publish_event(), this fires on its own once armed,
    // regardless of any further locality activity, so a waiter for a target
    // that never sees another call is still swept and invalidated at or after
    // its deadline.
    bool supervision_manager::sweep_timer_callback()
    {
        expired_waiters_t expired;
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            if (shutting_down_)
            {
                // The destructor has begun tearing this object down (and may
                // already be waiting on shutdown_cv_ for this invocation, had
                // it been counted): do not touch any other member.
                return false;
            }
            ++callbacks_in_flight_;

            // the timer just fired, so it is no longer armed
            armed_deadline_ = (std::chrono::steady_clock::time_point::max) ();

            expired = drain_expired_waiters_locked(
                l, std::chrono::steady_clock::now());

            // re-arm for whatever waiters remain, so a target that stays idle
            // keeps being bounded without further locality activity
            arm_sweep_timer_locked(HPX_MOVE(l));
        }

        invalidate_expired_waiters(expired);

        {
            std::unique_lock<hpx::spinlock> l(mtx_);
            if (--callbacks_in_flight_ == 0 && shutting_down_)
            {
                shutdown_cv_.notify_all(HPX_MOVE(l));
            }
        }

        return true;
    }

    // Invalidates any await_terminal() waiters that were registered for an
    // epoch that has now been superseded, giving each an explicit, descriptive
    // exception instead of leaving their promises to be destroyed (which would
    // set an opaque 'broken_promise' on the attached future). Must be called
    // with mtx_ not held.
    void supervision_manager::invalidate_stale_waiters(
        hpx::id_type const& target, std::uint64_t const epoch,
        stale_waiters_t& stale)
    {
        for (auto& [waiter_epoch, promises] : stale)
        {
            auto exception = HPX_GET_EXCEPTION(hpx::error::stale_state,
                "supervision_manager::invalidate_stale_waiters",
                hpx::util::format(
                    "await_terminal() waiter for target {} was invalidated: "
                    "epoch advanced to {} before the awaited epoch {} "
                    "reached a terminal event",
                    target, epoch, waiter_epoch));

            for (auto& [promise, _] : promises)
            {
                promise.set_exception(exception);
            }
        }
    }

    // Resolves any await_terminal() waiters extracted by
    // drain_terminal_waiters_locked(). Must be called with mtx_ not held, so
    // arbitrary continuations attached to the waiters' futures don't run while
    // holding it.
    void supervision_manager::resolve_terminal_waiters(
        lifecycle_event_notification const& notification, waiters_t& to_resolve)
    {
        if (to_resolve.empty())
        {
            return;
        }

        lifecycle_state const resolved_state = {.actor = notification.actor,
            .last_event = notification.event,
            .timestamp = notification.event_time,
            .event_sequence_number = notification.event_sequence_number,
            .epoch = notification.epoch,
            .ec = notification.ec};

        for (auto& [promise, _] : to_resolve)
        {
            promise.set_value(resolved_state);
        }
    }

    publish_result supervision_manager::publish_event(
        hpx::id_type const& target, event const ev, std::uint64_t const epoch)
    {
        sweep_expired_waiters();

        lifecycle_event_notification notification;
        waiters_t to_resolve;
        stale_waiters_t stale;

        // Captured before states_ is (possibly) mutated below: true only for a
        // target that has never had a states_ entry before this call, i.e. this
        // is its first-ever publish_event. Drives the
        // activity_transition::first_event notification fired further down;
        // unaffected by any early-return path (stale_epoch/already_terminal)
        // taken while still holding the lock, since those paths never reach
        // that code.
        bool had_state_before;

        // Snapshot of activity_observers_ taken in the very same mtx_
        // critical section as the states_/activated_at_ mutation below (see
        // deliver_activity_notification() for why): guarantees that any
        // register_activity_observer() call race is resolved consistently,
        // either its own replay already reflects this target's new state (if
        // it acquires mtx_ after this critical section) or its agent is
        // already captured here and receives the live notification below (if
        // it acquires mtx_ before this critical section) - never both, never
        // neither.
        std::vector<observer_entry> activity_observers_snapshot;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            auto const it = states_.find(target);
            had_state_before = it != states_.end();

            std::uint64_t const current_ep =
                had_state_before ? it->second.epoch : 0;

            if (epoch < current_ep)
            {
                // stale/out-of-order publication for an epoch that has already
                // been superseded: reject without mutating state or notifying
                // observers
                return publish_result::stale_epoch;
            }

            if (epoch > current_ep)
            {
                // reject an illegal epoch opening before any waiter state is
                // touched, so a rejected 'publish' leaves waiters_ untouched
                if (!is_valid_transition(event::unknown, ev))
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "supervision_manager::publish_event",
                        "invalid lifecycle event transition: a new epoch must "
                        "begin with a started event");
                }

                stale = drain_stale_waiters_locked(l, target, epoch);

                auto&& result = apply_new_epoch_locked(l, target, ev, epoch);
                notification = HPX_MOVE(result.notification);
                to_resolve = HPX_MOVE(result.to_resolve);
            }
            else
            {
                auto result = apply_current_epoch_locked(l, target, ev, epoch);
                if (!result)
                {
                    return publish_result::already_terminal;
                }

                notification = HPX_MOVE(result->notification);
                to_resolve = HPX_MOVE(result->to_resolve);
            }

            // Record the target's original activation time for
            // register_activity_observer()'s replay logic, guarded by
            // try_emplace() so a target that was already activated via
            // register_observer() (first_observer) keeps that earlier time.
            // Only reached for a target's first-ever publish_event (see
            // had_state_before above); harmless to skip for the
            // already_terminal/stale_epoch early returns above, since neither
            // represents a new activation.
            if (!had_state_before)
            {
                activated_at_.try_emplace(target, notification.event_time);

                // See activity_observers_snapshot above: must be taken here,
                // before mtx_ is released below, not later after mtx_ has
                // already been released.
                activity_observers_snapshot = activity_observers_;
            }
        }

        invalidate_stale_waiters(target, epoch, stale);
        resolve_terminal_waiters(notification, to_resolve);

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

        // Delivery ordering: per-target register_observer callbacks (above)
        // fire before activity-observer callbacks (below), both synchronous for
        // local delivery. Only a target's very first publish_event ever
        // triggers activity_transition::first_event; later publications never
        // repeat it (and never trigger a deactivation, since latched state does
        // not disappear on the publishing path).
        if (!had_state_before)
        {
            activity_notification const activity_notification{.actor = target,
                .state = activity_state::active,
                .transition = activity_transition::first_event,
                .event_time = notification.event_time,
                .epoch = notification.epoch};

            deliver_activity_notification(
                activity_notification, activity_observers_snapshot)
                .get();
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
        for (auto const& [observer, filter] : observers)
        {
            // An observer scoped to a specific epoch does not get notified of
            // events published under any other epoch.
            if (auto const epoch_filter = filter;
                (epoch_filter != static_cast<std::uint64_t>(-1)) &&
                epoch_filter != notification.epoch)
            {
                continue;
            }

            auto agent = observer;

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

            bool deactivated = false;

            // Snapshot of activity_observers_ taken in the very same mtx_
            // critical section as the observers_/activated_at_ mutation below,
            // if it happens; see the matching comment in publish_event() for
            // why this must not instead be taken later, after mtx_ has already
            // been released.
            std::vector<observer_entry> activity_observers_snapshot;

            if (!keep_registered)
            {
                // remove observer from the given target
                std::unique_lock<hpx::spinlock> l(mtx_);

                deactivated = unregister_observer_target(target, agent);
                remove_target_from_agents_locked(l, agent, target);

                // Only truly untracked (per the has-ever-published/
                // has-observers predicate used by
                // register_activity_observer()'s replay logic) once
                // it also has no states_ entry; a target that has published
                // at least one event stays tracked even after its last
                // observer is removed.
                if (deactivated && !states_.contains(target))
                {
                    activated_at_.erase(target);
                }

                if (deactivated)
                {
                    activity_observers_snapshot = activity_observers_;
                }
            }

            // Fired with mtx_ released, mirroring unregister_observer() below.
            if (deactivated)
            {
                activity_notification const activity_notification{
                    .actor = target,
                    .state = activity_state::inactive,
                    .transition =
                        activity_transition::last_observer_unregistered,
                    .event_time = std::chrono::steady_clock::now(),
                    .epoch = current_epoch_for(target)};

                deliver_activity_notification(
                    activity_notification, activity_observers_snapshot)
                    .get();
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
        std::uint64_t const epoch_filter)
    {
        std::optional<lifecycle_event_notification> initial_notification;

        // Captured before observers_[target] is (possibly) mutated below: true
        // only if target currently has no per-target observer at all, i.e. the
        // entry this call adds below will be its first. Drives the
        // activity_transition::first_observer notification fired further down.
        bool target_had_no_observers;

        // Set under mtx_ below, alongside activated_at_, only if
        // target_had_no_observers; reused below (once mtx_ has been released)
        // as the first_observer notification's event_time, so it agrees with
        // the time register_activity_observer()'s replay logic records
        // as this target's original activation time.
        std::chrono::steady_clock::time_point activation_time;

        // Snapshot of activity_observers_ taken in the very same mtx_
        // critical section as the observers_/activated_at_ mutation below;
        // see the matching comment in publish_event() for why this must not
        // instead be taken later, after mtx_ has already been released.
        std::vector<observer_entry> activity_observers_snapshot;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // insert observer into table of registered observers
            auto it = observers_.find(target);
            target_had_no_observers = (it == observers_.end());
            if (target_had_no_observers)
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

            it->second.push_back(
                observer_entry{.agent = agent, .epoch_filter = epoch_filter});

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

            // Record the target's original activation time for
            // register_activity_observer()'s replay logic, guarded by
            // try_emplace() so a target that was already activated via
            // publish_event() (first_event) keeps that earlier time.
            if (target_had_no_observers)
            {
                activation_time = std::chrono::steady_clock::now();
                activated_at_.try_emplace(target, activation_time);

                // See activity_observers_snapshot above: must be taken here,
                // before mtx_ is released below, not later after mtx_ has
                // already been released.
                activity_observers_snapshot = activity_observers_;
            }

            // An existing target gets an initial notification. Keep a value
            // snapshot: do not let a subsequent publish replace its contents.
            // A freshly-registered observer scoped to a specific epoch
            // (epoch_filter engaged) does not receive this synchronous
            // snapshot if it belongs to a different epoch, for consistency
            // with the filtering applied in fire_events.
            if (auto const it3 = states_.find(target); it3 != states_.end())
            {
                if (lifecycle_state const& state = it3->second;
                    epoch_filter == static_cast<std::uint64_t>(-1) ||
                    epoch_filter == state.epoch)
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

        // Delivery ordering: the per-target register_observer replay above
        // fires before this activity-observer notification, both synchronous
        // for local delivery.
        if (target_had_no_observers)
        {
            activity_notification const activity_notification{.actor = target,
                .state = activity_state::active,
                .transition = activity_transition::first_observer,
                .event_time = activation_time,
                .epoch = current_epoch_for(target)};

            deliver_activity_notification(
                activity_notification, activity_observers_snapshot)
                .get();
        }

        return agent;
    }

    // Returns true if this removed the last remaining per-target observer for
    // `target` (its observer count just transitioned from one to zero); the
    // caller is responsible for firing the corresponding
    // activity_transition::last_observer_unregistered notification once
    // mtx_ has been released (this function is always called with mtx_
    // already held).
    bool supervision_manager::unregister_observer_target(
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
                return true;
            }
        }
        return false;
    }

    std::uint64_t supervision_manager::current_epoch_for(
        hpx::id_type const& target) const
    {
        std::scoped_lock<hpx::spinlock> l(mtx_);
        auto const it = states_.find(target);
        return it != states_.end() ? it->second.epoch : 0;
    }

    void supervision_manager::unregister_observer(
        hpx::id_type const& observer_handle)
    {
        // Targets whose last per-target observer was just removed by this call;
        // the corresponding activity_transition::last_observer_unregistered
        // notifications are fired below, once mtx_ has been released.
        std::vector<hpx::id_type> deactivated_targets;

        // Snapshot of activity_observers_ taken in the very same mtx_
        // critical section as the observers_/activated_at_ mutations below,
        // and reused for every target in deactivated_targets below (a single
        // snapshot is valid for all of them, since none of this function's
        // own mutations touch activity_observers_); see the matching comment
        // in publish_event() for why this must not instead be taken later,
        // after mtx_ has already been released.
        std::vector<observer_entry> activity_observers_snapshot;

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
                    if (unregister_observer_target(target, observer_handle))
                    {
                        deactivated_targets.push_back(target);

                        // See the matching comment in fire_event(): only erase
                        // activated_at_ if target is also not tracked via
                        // states_ (i.e. it never published an event).
                        if (!states_.contains(target))
                        {
                            activated_at_.erase(target);
                        }
                    }
                }
                agents_.erase(it);
            }
            else if (std::ranges::find(activity_observers_, observer_handle,
                         &observer_entry::agent) != activity_observers_.end())
            {
                // observer_handle was not registered via register_observer(),
                // but was found registered via register_activity_observer():
                // reject it before touching any state or dispatching
                // deactivate_and_wait below.
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "supervision_manager::unregister_observer",
                    "observer_handle was not returned by register_observer()");
            }
            else
            {
                // observer_handle was never returned by either registration
                // API.
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "supervision_manager::unregister_observer",
                    "observer_handle does not represent a handle previously "
                    "returned by register_observer()");
            }

            if (!deactivated_targets.empty())
            {
                activity_observers_snapshot = activity_observers_;
            }
        }

        for (hpx::id_type const& target : deactivated_targets)
        {
            activity_notification const activity_notification{.actor = target,
                .state = activity_state::inactive,
                .transition = activity_transition::last_observer_unregistered,
                .event_time = std::chrono::steady_clock::now(),
                .epoch = current_epoch_for(target)};

            deliver_activity_notification(
                activity_notification, activity_observers_snapshot)
                .get();
        }

        // A delivery action that was already queued may still reach the agent.
        // deactivate_and_wait fences such actions and drains any callback that
        // had already begun before this call returns.

        // prevent the agent callback from being run inline as it may block
        using action_type = agent_component::deactivate_and_wait_action;
        hpx::async(hpx::launch::task, action_type(), observer_handle).get();
    }

    // Removes the target from the agent's entry in agents_ (the agent ->
    // targets inverse map) and erases the entry entirely once it becomes empty.
    void supervision_manager::remove_target_from_agents_locked(
        std::unique_lock<hpx::spinlock>& l, hpx::id_type const& agent,
        hpx::id_type const& target)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        if (auto const it = agents_.find(agent); it != agents_.end())
        {
            std::erase(it->second, target);
            if (it->second.empty())
            {
                agents_.erase(it);
            }
        }
    }

    // Clears all locally tracked state for `target`: its recorded lifecycle
    // state (states_), any per-target observers still registered for it
    // (observers_, along with their corresponding entries in the agents_
    // inverse lookup), and its activated_at_ entry. Unlike
    // unregister_observer_target(), which removes a single observer_handle's
    // entry for a target and leaves its recorded lifecycle state intact, this
    // forgets every bit of local state associated with target, regardless of
    // any specific observer handle.
    void supervision_manager::remove_target(hpx::id_type const& target)
    {
        // Targets are considered tracked (see register_activity_observer()'s
        // replay logic) if they have a recorded lifecycle state or at least one
        // registered observer; captured before either map is mutated below to
        // decide whether a deactivation notification is warranted.
        bool was_tracked = false;

        // The epoch target was in immediately before its state was removed,
        // captured before state_ is erased below, for use in the
        // activity_transition::last_observer_unregistered notification fired
        // once mtx_ has been released (mirrors unregister_observer()).
        std::uint64_t epoch_at_removal = 0;

        // Snapshot of activity_observers_ taken in the very same mtx_ critical
        // section as the states_/observers_/activated_at_ mutations below; see
        // the matching comment in publish_event() for why this must not instead
        // be taken later, after mtx_ has already been released.
        std::vector<observer_entry> activity_observers_snapshot;

        // waiters registered for this target (all epochs)
        stale_waiters_t stale_waiters;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            if (auto const it2 = states_.find(target); it2 != states_.end())
            {
                was_tracked = true;
                epoch_at_removal = it2->second.epoch;
            }

            if (auto const it = observers_.find(target); it != observers_.end())
            {
                was_tracked = true;
                for (auto const& [agent, _] : it->second)
                {
                    // remove target from agents
                    remove_target_from_agents_locked(l, agent, target);
                }
                observers_.erase(it);
            }

            states_.erase(target);
            activated_at_.erase(target);

            // Drain every waiter registered for this target (all epochs) while
            // still holding mtx_, so none are left to be picked up later by the
            // deadline-based sweep.
            stale_waiters = drain_all_waiters_for_target_locked(l, target);

            if (was_tracked)
            {
                activity_observers_snapshot = activity_observers_;
            }
        }

        // Resolve the drained waiters' futures immediately with an explicit
        // exception, instead of letting them sit until their deadline expires.
        if (!stale_waiters.empty())
        {
            invalidate_stale_waiters(target, epoch_at_removal, stale_waiters);
        }

        if (was_tracked)
        {
            activity_notification const activity_notification{.actor = target,
                .state = activity_state::inactive,
                .transition = activity_transition::last_observer_unregistered,
                .event_time = std::chrono::steady_clock::now(),
                .epoch = epoch_at_removal};

            deliver_activity_notification(
                activity_notification, activity_observers_snapshot)
                .get();
        }
    }

    hpx::future<void> supervision_manager::deliver_activity_notification(
        activity_notification const& notification,
        std::vector<observer_entry> const& observers)
    {
        for (auto const& [observer, filter] : observers)
        {
            // An observer scoped to a specific epoch does not get notified of
            // transitions recorded under any other epoch.
            if (filter != static_cast<std::uint64_t>(-1) &&
                filter != notification.epoch)
            {
                continue;
            }

            try
            {
                fire_activity_event(observer, notification).get();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
                // Best effort: one activity observer's failure must not
                // prevent delivery to the remaining activity observers, and
                // there is no per-target latch to record this failure into
                // (unlike record_error() for per-target observers).
            }
        }

        return hpx::make_ready_future();
    }

    hpx::future<void> supervision_manager::fire_activity_event(
        hpx::id_type const& agent, activity_notification notification)
    {
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // check again if the agent is still registered as an activity
            // observer
            if (std::ranges::find(activity_observers_, agent,
                    &observer_entry::agent) == activity_observers_.end())
            {
                return hpx::make_ready_future();
            }
        }

        try
        {
            using action_type =
                activity_agent_component::invoke_if_active_action;
            hpx::future<bool> keep_registered = hpx::async(hpx::launch::task,
                action_type(), agent, HPX_MOVE(notification));

            if (!keep_registered.get())
            {
                std::unique_lock<hpx::spinlock> l(mtx_);
                std::erase_if(
                    activity_observers_, [&agent](observer_entry const& entry) {
                        return entry.agent == agent;
                    });
            }
        }
        catch (...)
        {
            return hpx::make_exceptional_future<void>(std::current_exception());
        }

        return hpx::make_ready_future();
    }

    hpx::id_type supervision_manager::register_activity_observer(
        hpx::id_type const& agent, std::uint64_t const epoch_filter)
    {
        // Registration-time replay: snapshot every currently-tracked target
        // (the same has-ever-published-via-states_/has-observers-via-
        // observers_ predicate that governs activated_at_ above) and install
        // `agent` into activity_observers_ inside the same critical section, so
        // no live transition recorded concurrently by publish_event()/
        // register_observer()/unregister_observer()/fire_event() can be missed
        // or delivered twice to `agent`. Each of those call sites mutates its
        // tracked state (states_/observers_/activated_at_) and snapshots
        // activity_observers_ (the set that will receive its live notification,
        // see deliver_activity_notification()) within one single mtx_ critical
        // section of its own. Since this call's snapshot-and-install below is
        // likewise a single critical section on the same mtx_, the two are
        // strictly ordered relative to each other: if the other call's critical
        // section runs first, its activity_observers_ snapshot cannot yet
        // include `agent` (installed only below), but the mutated state it just
        // recorded is already visible to the snapshot below, which replays it
        // as already_active; if this call's critical section runs first,
        // `agent` is already installed by the time the other call takes its
        // activity_observers_ snapshot, so it receives that live notification
        // instead, while the snapshot below - running before that mutation -
        // does not also replay it.
        std::vector<activity_notification> replay;

        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // Union of states_ and observers_ keys: a target counts as tracked
            // if it has ever published an event (states_ entry exists) or
            // currently has at least one per-target observer (observers_ entry
            // exists; unregister_observer_target() never leaves an empty entry
            // behind, see there). Deliberately not a merged reference count:
            // either signal on its own is sufficient.
            std::map<hpx::id_type, std::uint64_t> tracked;
            for (auto const& [target, state] : states_)
            {
                tracked.emplace(target, state.epoch);
            }

            for (auto const& key : observers_ | std::views::keys)
            {
                hpx::id_type const& target = key;
                if (auto const states_it = states_.find(target);
                    states_it != states_.end())
                {
                    tracked.try_emplace(target, states_it->second.epoch);
                }
                else
                {
                    tracked.try_emplace(target, 0);
                }
            }

            replay.reserve(tracked.size());
            for (auto const& [target, epoch] : tracked)
            {
                // Same epoch-filter semantics as
                // deliver_activity_notification(): a filter engaged to a
                // specific epoch skips any target whose (current) epoch does
                // not match, including here at replay time.
                if (epoch_filter != static_cast<std::uint64_t>(-1) &&
                    epoch_filter != epoch)
                {
                    continue;
                }

                // event_time reflects the target's original activation time
                // (see activated_at_ above), not the time of this registration,
                // per activity_notification::event_time.
                auto const activated_it = activated_at_.find(target);
                auto const event_time = activated_it != activated_at_.end() ?
                    activated_it->second :
                    std::chrono::steady_clock::now();

                replay.push_back(activity_notification{.actor = target,
                    .state = activity_state::active,
                    .transition = activity_transition::already_active,
                    .event_time = event_time,
                    .epoch = epoch});
            }

            activity_observers_.push_back(
                observer_entry{.agent = agent, .epoch_filter = epoch_filter});
        }

        // Deliver the replay outside mtx_, mirroring register_observer()'s
        // delivery of its own initial notification: fire_activity_event()
        // re-checks that `agent` is still registered before invoking it, so
        // this remains safe even if `agent` is concurrently unregistered.
        for (auto const& notification : replay)
        {
            try
            {
                fire_activity_event(agent, notification).get();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
                // Best effort, mirroring deliver_activity_notification(): one
                // replay delivery failing must not prevent delivery of the
                // remaining replay notifications, nor this registration call
                // from returning.
            }
        }

        return agent;
    }

    void supervision_manager::unregister_activity_observer(
        hpx::id_type const& observer_handle)
    {
        {
            std::unique_lock<hpx::spinlock> l(mtx_);

            // Remove the matching entry from activity_observers_, if any; a
            // handle returned by register_observer() (found in agents_) or one
            // never returned by either API is rejected below.
            std::size_t const removed = std::erase_if(activity_observers_,
                [&observer_handle](observer_entry const& entry) {
                    return entry.agent == observer_handle;
                });

            if (removed == 0)
            {
                if (agents_.contains(observer_handle))
                {
                    // observer_handle is valid, but was returned by
                    // register_observer(), not
                    // register_activity_observer(): reject it rather than
                    // silently treating it as a no-op.
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "supervision_manager::unregister_activity_observer",
                        "observer_handle was returned by register_observer(), "
                        "not register_activity_observer()");
                }

                // observer_handle was never returned by either registration
                // API.
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "supervision_manager::unregister_activity_observer",
                    "observer_handle does not represent a handle previously "
                    "returned by register_activity_observer()");
            }
        }

        // A delivery that was already in flight for this agent may still be
        // running (see fire_activity_event()); deactivate_and_wait fences such
        // deliveries and drains any callback that had already begun before this
        // call returns, mirroring unregister_observer() above.
        using action_type =
            activity_agent_component::deactivate_and_wait_action;
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

    hpx::future<lifecycle_state> supervision_manager::await_terminal(
        hpx::id_type const& target, std::uint64_t const epoch,
        std::chrono::steady_clock::duration const timeout)
    {
        sweep_expired_waiters();

        std::unique_lock<hpx::spinlock> l(mtx_);
        auto const it = states_.find(target);
        bool has_state = it != states_.end();
        if (has_state && it->second.epoch == epoch &&
            is_terminal(it->second.last_event))
        {
            lifecycle_state st = it->second;
            l.unlock();
            return hpx::make_ready_future(st);
        }

        if (has_state && it->second.epoch > epoch)
        {
            auto const previous_epoch = it->second.epoch;

            // the caller is asking about an epoch that has already been
            // superseded; a waiter registered under the stale epoch would only
            // ever be drained by a *future* epoch transition, so fail fast
            // instead of hanging indefinitely
            l.unlock();

            return hpx::make_exceptional_future<lifecycle_state>(
                HPX_GET_EXCEPTION(hpx::error::stale_state,
                    "supervision_manager::await_terminal",
                    hpx::util::format(
                        "invoking await_terminal() for target {} is invalid: "
                        "epoch has advanced to {} beyond the requested epoch "
                        "{}",
                        target, previous_epoch, epoch)));
        }

        // std::chrono::steady_clock::duration::max() (the default) defers to
        // the live default_await_terminal_timeout_ms; any other value overrides
        // the deadline for this call only.
        auto const effective_timeout =
            (timeout == (std::chrono::steady_clock::duration::max) ()) ?
            std::chrono::milliseconds(default_await_terminal_timeout_ms) :
            timeout;
        auto const now = std::chrono::steady_clock::now();
        auto const no_deadline =
            (std::chrono::steady_clock::time_point::max) ();

        // avoid overflowing the time_point for very large timeouts
        auto const deadline = effective_timeout >= no_deadline - now ?
            no_deadline :
            now + effective_timeout;
        hpx::promise<lifecycle_state> p;
        auto f = p.get_future();
        waiters_[waiter_key{.target = target, .epoch = epoch}].push_back(
            waiter_entry{HPX_MOVE(p), deadline});
        earliest_deadline_ = (std::min) (earliest_deadline_, deadline);

        // guarantee this waiter is swept at or after its deadline even if this
        // locality never sees another await_terminal()/publish_event() call for
        // target
        arm_sweep_timer_locked(HPX_MOVE(l));

        return f;
    }

    dispatch_outcome supervision_manager::check_admission(
        hpx::id_type const& target, std::uint64_t const epoch) const noexcept
    {
        std::scoped_lock<hpx::spinlock> l(mtx_);
        if (auto const it = states_.find(target); it != states_.end() &&
            it->second.epoch == epoch && is_terminal(it->second.last_event))
        {
            return dispatch_outcome::rejected_fenced;
        }
        return dispatch_outcome::admitted;
    }

    ///////////////////////////////////////////////////////////////////////////
    void supervision_manager::register_server_instance(
        char const* service_name, std::uint32_t locality_id, error_code& ec)
    {
        // set locality_id for this component
        if (locality_id == naming::invalid_locality_id)
        {
            locality_id = agas::get_locality_id();
        }

        this->base_type::set_locality_id(locality_id);

        // now register this supervision instance with AGAS
        std::string instance_name = supervision::service_name;
        instance_name += service_name;
        instance_name += supervision::server::supervision_manager_name;

        auto const gid = get_unmanaged_id().get_gid();
        naming::address const manager_address(agas::get_locality(),
            components::get_component_type<
                supervision::server::supervision_manager>(),
            this);
        agas::bind_gid_local(gid, manager_address, ec);
        if (ec)
            return;

        instance_name_ = service_name;

        // register a gid (not the id) to avoid AGAS holding a reference to this
        // component
        agas::register_name(launch::sync, instance_name, gid, ec);
    }

    void supervision_manager::unregister_server_instance(error_code& ec) const
    {
        if (!instance_name_.empty())
        {
            std::string instance_name = supervision::service_name;
            instance_name += instance_name_;
            instance_name += supervision::server::supervision_manager_name;

            agas::unregister_name(launch::sync, instance_name, ec);

            instance_name_.clear();
        }
    }
}    // namespace hpx::supervision::server
