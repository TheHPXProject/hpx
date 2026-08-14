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
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/supervision/supervision_api.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hpx::supervision::server {

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
        std::uint64_t epoch_filter;
    };

    struct waiter_key
    {
        hpx::id_type target;
        std::uint64_t epoch;

        friend bool operator<(waiter_key const& lhs, waiter_key const& rhs)
        {
            if (lhs.target != rhs.target)
                return lhs.target < rhs.target;
            return lhs.epoch < rhs.epoch;
        }

        friend bool operator==(waiter_key const& lhs, waiter_key const& rhs)
        {
            return lhs.target == rhs.target && lhs.epoch == rhs.epoch;
        }
    };

    // A pending await_terminal() waiter together with the point in time after
    // which it is considered abandoned. deadline bounds how long a waiter can
    // stay in waiters_ if it is never reached via the exact-epoch or
    // epoch-supersession drain paths, e.g. because the caller dropped the
    // returned future or the target never publishes another event.
    struct waiter_entry
    {
        hpx::promise<lifecycle_state> promise;
        std::chrono::steady_clock::time_point deadline;
    };

    struct supervision_manager
      : hpx::components::fixed_component_base<supervision_manager>
    {
        using base_type = components::fixed_component_base<supervision_manager>;

        supervision_manager();

        ~supervision_manager();

        void register_server_instance(char const* service_name,
            std::uint32_t locality_id, error_code& ec = throws);
        void unregister_server_instance(error_code& ec = throws) const;

        // Supervision API implementation

        /// \brief Applies epoch/terminal-latch state-mutation rules,
        ///        resolves any await_terminal() waiters affected by the
        ///        transition, and invokes registered per-target lifecycle
        ///        observers and activity observers for this call.
        ///
        /// \param target Target whose supervision state is updated. Must
        ///               live on this locality.
        /// \param ev Lifecycle event to apply.
        /// \param epoch Epoch associated with \p ev.
        ///
        /// \returns The result of applying \p ev to target's state.
        publish_result publish_event(
            hpx::id_type const& target, event ev, std::uint64_t epoch);

        /// \cond NOINTERNAL
        struct publish_event_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::publish_event),
                &supervision_manager::publish_event, publish_event_action>
        {
        };
        /// \endcond

        /// \cond NOINTERNAL
        /// \brief Applies the same epoch/terminal-latch state-mutation
        ///        rules as publish_event() and resolves any
        ///        await_terminal() waiters affected by the transition, but
        ///        never invokes registered per-target lifecycle observers
        ///        or activity observers for this call.
        ///
        /// Meant purely as a state write for consumers of
        /// query_state()/check_admission()/await_terminal(), not as a
        /// notification mechanism - in particular, calling this from
        /// within a lifecycle observer callback registered for \p target
        /// cannot re-invoke that observer, unlike publish_event(). Not
        /// exposed as an action: like check_admission(), this is only ever
        /// meant to be called on the locality \p target lives on.
        ///
        /// \param target Target whose supervision state is updated. Must
        ///               live on this locality.
        /// \param ev Lifecycle event to apply.
        /// \param epoch Epoch associated with \p ev.
        ///
        /// \returns The result of applying \p ev to target's state.
        publish_result publish_event_no_notify(
            hpx::id_type const& target, event ev, std::uint64_t epoch);
        /// \endcond

        lifecycle_state query_state(hpx::id_type const& target);

        /// \cond NOINTERNAL
        struct query_state_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::query_state),
                &supervision_manager::query_state, query_state_action>
        {
        };
        /// \endcond

        hpx::id_type register_observer(hpx::id_type const& target,
            hpx::id_type const& agent,
            std::uint64_t epoch_filter = static_cast<std::uint64_t>(-1));

        /// \cond NOINTERNAL
        struct register_observer_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::register_observer),
                &supervision_manager::register_observer,
                register_observer_action>
        {
        };
        /// \endcond

        void unregister_observer(hpx::id_type const& observer_handle);

        /// \cond NOINTERNAL
        struct unregister_observer_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::unregister_observer),
                &supervision_manager::unregister_observer,
                unregister_observer_action>
        {
        };
        /// \endcond

        /// \brief Clears all locally tracked state for `target`.
        ///
        /// Unlike unregister_observer(), which removes a single previously
        /// registered observer handle (and leaves any recorded lifecycle state
        /// for its target(s) intact), remove_target() unconditionally forgets
        /// every piece of local bookkeeping this supervision manager holds for
        /// `target` - its recorded lifecycle state and current epoch (see
        /// publish_event()), and any per-target observers still registered for
        /// it (see register_observer()) - regardless of any specific observer
        /// handle. Intended for callers that know `target` will never be
        /// queried or observed again locally (e.g. after a failed registration
        /// that seeded some state for it, or once a peer has been evicted) and
        /// want to reclaim that local state instead of letting it accumulate
        /// indefinitely.
        void remove_target(hpx::id_type const& target);

        /// \cond NOINTERNAL
        struct remove_target_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::remove_target),
                &supervision_manager::remove_target, remove_target_action>
        {
        };
        /// \endcond

        /// \brief Unconditionally clears all locally tracked state.
        ///
        /// Snapshots every target currently present in states_ under mtx_,
        /// releases the lock, then calls remove_target() for each one in
        /// turn, reusing its existing per-target teardown (waiter
        /// invalidation, activity-observer notifications, agent/state
        /// cleanup) instead of duplicating that logic here. Targets added
        /// concurrently with (or after) the snapshot are left untouched.
        /// If states_ is empty at snapshot time, this is a no-op: tidy()
        /// never reports an error and never fires any notification of its
        /// own for the "nothing to do" case. Local-only: unlike
        /// remove_target(), this is not exposed as a remote action.
        void tidy();

        /// \brief Registers a locality-scoped activity observer.
        ///
        /// Replays targets that are already active when registration takes its
        /// snapshot.
        ///
        /// The snapshot of the target's tracked state and the insertion of the
        /// observer into the tracked set happen atomically under `mtx_`, which
        /// guarantees the observer receives exactly one notification for the
        /// target's current state: either the replay (if already active at
        /// registration time) or a live transition that raced with
        /// registration, but never both and never neither.
        ///
        /// Delivery of that notification (replay or live) happens after `mtx_`
        /// is released and is therefore not ordered relative to any other
        /// concurrent live notification for the same target; callers must not
        /// assume replay is delivered before or after a racing live event.
        hpx::id_type register_activity_observer(hpx::id_type const& agent,
            std::uint64_t epoch_filter = static_cast<std::uint64_t>(-1));

        /// \cond NOINTERNAL
        struct register_activity_observer_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::register_activity_observer),
                &supervision_manager::register_activity_observer,
                register_activity_observer_action>
        {
        };
        /// \endcond

        /// \brief Unregisters an activity observer.
        ///
        /// The handle must have been returned by register_activity_observer().
        /// As with unregister_observer(), no orphaned callbacks fire after this
        /// call completes. `observer_handle` must have been returned by
        /// register_activity_observer(); a handle returned by
        /// register_observer() instead (i.e. found in agents_ rather than
        /// activity_observers_) is rejected.
        void unregister_activity_observer(hpx::id_type const& observer_handle);

        /// \cond NOINTERNAL
        struct unregister_activity_observer_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::unregister_activity_observer),
                &supervision_manager::unregister_activity_observer,
                unregister_activity_observer_action>
        {
        };
        /// \endcond

        // Resolve once `target` reaches a terminal event (`completed` or
        // `failed`) within `epoch`. If `target` has already reached a terminal
        // event within `epoch` at the time of the call, the returned future is
        // ready immediately. Otherwise, the returned future becomes ready when
        // `publish_event` next records a terminal event for `target` under the
        // exact same epoch. Waiters registered for an epoch that is superseded
        // by a higher epoch (see the epoch semantics of `publish_event`) never
        // reach a terminal event under that epoch; instead, their future
        // becomes exceptional with an `hpx::error::stale_state` exception
        // describing the epoch that superseded them. A waiter that is reached
        // by neither of these two paths (e.g. the caller drops the returned
        // future, or `target` never publishes another event) is bounded by a
        // deadline set from `timeout` (relative to registration time), or a
        // built-in default if `timeout` is left at its sentinel value
        // `std::chrono::steady_clock::duration::max()`: it is swept and its
        // future made exceptional with an `hpx::error::future_cancelled`
        // exception at or after that deadline passes, so entries in waiters_
        // cannot accumulate indefinitely. This is enforced by sweep_timer_
        // independently of any further activity for `target`, in addition to
        // the opportunistic sweep performed by this function and
        // publish_event() as a fast path.
        hpx::future<lifecycle_state> await_terminal(hpx::id_type const& target,
            std::uint64_t epoch = 0,
            std::chrono::steady_clock::duration timeout =
                (std::chrono::steady_clock::duration::max) ());

        /// \cond NOINTERNAL
        struct await_terminal_action
          : hpx::actions::make_action_t<
                decltype(&supervision_manager::await_terminal),
                &supervision_manager::await_terminal, await_terminal_action>
        {
        };
        /// \endcond

        // Pure local read of the terminal latch state maintained for
        // `target`; see hpx::supervision::check_admission() for the exact
        // semantics. Not exposed as an action: unlike publish_event()/
        // query_state(), this is only ever meant to be called on the
        // locality `target` lives on.
        dispatch_outcome check_admission(
            hpx::id_type const& target, std::uint64_t epoch = 0) const noexcept;

    protected:
        hpx::future<void> fire_events(hpx::id_type const& target,
            lifecycle_event_notification const& notification);
        hpx::future<void> fire_event(hpx::id_type const& target,
            hpx::id_type const& agent,
            lifecycle_event_notification notification);

        // Delivers `notification` to each entry in `observers` (a snapshot of
        // activity_observers_ taken by the caller), skipping any observer whose
        // epoch_filter does not match. Local delivery is synchronous: each
        // observer's callback has already run by the time the returned future
        // becomes ready. One observer's failure does not prevent delivery to
        // the remaining observers. Used by publish_event()/
        // register_observer()/unregister_observer()/fire_event() so that the
        // activity_observers_ snapshot determining delivery of a
        // first_event/first_observer/last_observer_unregistered transition is
        // taken in the very same mtx_ critical section as the
        // states_/observers_ mutation that drives the transition, instead of in
        // a later, separate critical section: otherwise, a
        // register_activity_observer() call that acquires mtx_ in between would
        // install its agent into activity_observers_ in time to receive this
        // transition's live notification *and* see the already-mutated state in
        // its own replay snapshot, delivering the same transition to that agent
        // twice (or, symmetrically, delivering it zero times).
        hpx::future<void> deliver_activity_notification(
            activity_notification const& notification,
            std::vector<observer_entry> const& observers);

        // Delivers `notification` to the single activity observer identified by
        // `agent`, if it is still registered. Removes `agent` from
        // activity_observers_ if its callback signals it no longer wants to be
        // invoked.
        hpx::future<void> fire_activity_event(
            hpx::id_type const& agent, activity_notification notification);

        void record_error(hpx::id_type const& target,
            std::uint64_t expected_sequence_number, hpx::error_code const& ec);

        // Returns true if this removed the last remaining per-target observer
        // for `target` (i.e. its per-target observer count just transitioned
        // from one to zero), so the caller can fire an
        // activity_transition::last_observer_unregistered notification once
        // mtx_ has been released.
        bool unregister_observer_target(
            hpx::id_type const& target, hpx::id_type const& observer_handle);

        // Returns the epoch currently recorded for `target`, or 0 if no epoch
        // has been recorded yet. Used to populate the `epoch` field of activity
        // notifications that are not themselves triggered by a publish_event()
        // call (i.e. first_observer/ last_observer_unregistered).
        std::uint64_t current_epoch_for(hpx::id_type const& target) const;

        using waiters_t = std::vector<waiter_entry>;
        using stale_waiters_t =
            std::vector<std::pair<std::uint64_t, waiters_t>>;

        // Result of apply_event_and_resolve(): shared by publish_event() and
        // publish_event_no_notify(), which differ only in whether they go on
        // to deliver `notification`/the first_event activity transition to
        // observers_/activity_observers_ once result == applied.
        struct apply_event_outcome
        {
            publish_result result;
            lifecycle_event_notification notification;
            bool had_state_before = false;
            std::vector<observer_entry> activity_observers_snapshot;
        };

        // Shared implementation of publish_event()/publish_event_no_notify():
        // performs the epoch/terminal-latch state mutation and resolves any
        // await_terminal() waiters affected by it, but stops short of
        // delivering anything to observers_/activity_observers_, which is left
        // to the caller.
        apply_event_outcome apply_event_and_resolve(
            hpx::id_type const& target, event ev, std::uint64_t epoch);

        // A waiter that was swept because its deadline passed, together with
        // the (target, epoch) it was registered for; unlike stale_waiters_t,
        // expired waiters are not scoped to a single target since the expiry
        // sweep walks the entire waiters_ map.
        struct expired_waiter
        {
            hpx::id_type target;
            std::uint64_t epoch;
            hpx::promise<lifecycle_state> promise;
        };
        using expired_waiters_t = std::vector<expired_waiter>;

        // Helpers used to implement publish_event(); see the .cpp file for
        // details on the locking discipline each of these follows.
        struct apply_result
        {
            lifecycle_event_notification notification;
            waiters_t to_resolve;
        };

        waiters_t drain_terminal_waiters_locked(
            std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
            std::uint64_t epoch, event last_event);

        stale_waiters_t drain_stale_waiters_locked(
            std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
            std::uint64_t epoch);

        apply_result apply_new_epoch_locked(std::unique_lock<hpx::spinlock>& l,
            hpx::id_type const& target, supervision::event ev,
            std::uint64_t epoch);

        std::optional<apply_result> apply_current_epoch_locked(
            std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target,
            supervision::event ev, std::uint64_t epoch);

        static void invalidate_stale_waiters(hpx::id_type const& target,
            std::uint64_t epoch, stale_waiters_t& stale);

        static void resolve_terminal_waiters(
            lifecycle_event_notification const& notification,
            waiters_t& to_resolve);

        // Walks the entire waiters_ map and drains any waiter whose deadline is
        // at or before `now`, regardless of target/epoch. Also recomputes
        // earliest_deadline_ as a byproduct of this scan, folding over the
        // deadlines of the waiters that survive. The caller invalidates the
        // returned waiters after the lock has been released.
        expired_waiters_t drain_expired_waiters_locked(
            std::unique_lock<hpx::spinlock>& l,
            std::chrono::steady_clock::time_point now);

        static void invalidate_expired_waiters(expired_waiters_t& expired);

        // Drains *all* waiters registered for `target`, across every epoch
        // (unlike drain_stale_waiters_locked, which only drains epochs below a
        // cutoff). Used when the target is being removed entirely, so no waiter
        // should be left behind regardless of which epoch it was registered
        // under.
        stale_waiters_t drain_all_waiters_for_target_locked(
            std::unique_lock<hpx::spinlock>& l, hpx::id_type const& target);

        // Opportunistic, lazy cleanup for abandoned await_terminal() waiters:
        // acquires mtx_, drains any waiter past its deadline, then invalidates
        // the drained waiters after releasing the lock. Called from both
        // await_terminal() and publish_event() so that abandoned waiters are
        // bounded without a dedicated background sweep thread. Skips the
        // full-map scan in drain_expired_waiters_locked() entirely whenever
        // earliest_deadline_ indicates no waiter could possibly have expired
        // yet.
        void sweep_expired_waiters();

        // Arms (or re-arms) sweep_timer_ so it fires at or after
        // earliest_deadline_, unless it is already armed for a deadline at or
        // before that; this is the guaranteed backstop that lets
        // sweep_expired_waiters() reach waiters registered for a target whose
        // locality never sees another await_terminal()/publish_event() call. A
        // no-op if waiters_ is currently empty. Called with mtx_ held (both
        // after a new (possibly earlier) deadline is registered in
        // await_terminal() and from sweep_timer_callback() itself, to re-arm
        // for the next earliest deadline once the timer fires) and returns
        // with mtx_ held again, but internally releases mtx_ (via
        // sweep_timer_mtx_ instead) around the actual pool_timer calls; see
        // the .cpp file for why.
        void arm_sweep_timer_locked(std::unique_lock<hpx::spinlock>&& l);

        // Callback invoked by sweep_timer_ when it fires: drains and
        // invalidates any waiters past their deadline, then re-arms the timer
        // (via arm_sweep_timer_locked()) for whatever deadline is earliest
        // among the waiters that remain. sweep_timer_ is one-shot, so re-arming
        // here is what turns it into a recurring backstop.
        bool sweep_timer_callback();

        // Recomputes earliest_deadline_ from scratch by folding over every
        // remaining waiter in waiters_. Called by
        // drain_terminal_waiters_locked() and drain_stale_waiters_locked(),
        // which only touch a single target/epoch bucket (or a small range of
        // buckets) and therefore cannot otherwise tell whether the waiter(s)
        // they just erased held the current earliest deadline.
        // drain_expired_waiters_locked() does not use this helper: since it
        // already performs a full scan of waiters_, it derives the new earliest
        // deadline directly as a byproduct of that scan instead.
        void recompute_earliest_deadline_locked(
            std::unique_lock<hpx::spinlock>& l);

        // Removes the target from the agent's entry in agents_ (the agent ->
        // targets inverse map) and erases the entry entirely once it becomes
        // empty.
        void remove_target_from_agents_locked(
            std::unique_lock<hpx::spinlock>& l, hpx::id_type const& agent,
            hpx::id_type const& target);

    private:
        mutable hpx::spinlock mtx_;

        // Serializes the actual pool_timer::init()/stop()/start() calls made by
        // arm_sweep_timer_locked() and by the destructor, which perform them
        // with mtx_ released (see arm_sweep_timer_locked() for why):
        // sweep_timer_ is not safe against concurrent init()/stop()/start()
        // calls, so this dedicated (non-busy-wait) lock protects it instead of
        // mtx_.
        hpx::spinlock sweep_timer_mtx_;

        mutable std::string instance_name_;

        // registered events: targets -> states
        std::map<hpx::id_type, lifecycle_state> states_;
        // registered observers: targets -> observer entries (agent +
        // optional epoch filter)
        std::map<hpx::id_type, std::vector<observer_entry>> observers_;
        // inverse lookup for agents: agents -> targets
        std::map<hpx::id_type, std::vector<hpx::id_type>> agents_;

        // registered activity observers (see register_activity_observer()):
        // locality-scoped, not keyed by target, so every entry here is
        // delivered every activation/ deactivation transition recorded for any
        // target this manager tracks (filtered only by each entry's own
        // epoch_filter). Kept entirely separate from observers_/agents_ above
        // so that this collection only ever holds handles returned by
        // register_activity_observer(), never ones returned by
        // register_observer().
        std::vector<observer_entry> activity_observers_;

        // Records, for every target currently tracked (i.e. present in states_
        // or observers_, see register_activity_observer()), the time it
        // originally transitioned to activity_state::active. Populated the
        // first time a target's tracked state flips from false to true (in
        // publish_event()/register_observer(), guarded by try_emplace() so a
        // later trigger never overwrites the original activation time), and
        // erased once a target is no longer tracked by either signal (in
        // unregister_observer_target()'s callers). Consulted by
        // register_activity_observer() to populate the event_time of an
        // activity_transition::already_active replay with the target's original
        // activation time rather than the time of registration, per
        // activity_notification::event_time.
        std::map<hpx::id_type, std::chrono::steady_clock::time_point>
            activated_at_;

        // one-shot waiters for await_terminal(): (target, epoch) -> pending
        // promises (each paired with a deadline). Resolved and erased from this
        // map by publish_event() as soon as the corresponding target/epoch pair
        // reaches a terminal event; set to an explicit exception and erased
        // instead if that epoch is superseded first, or if its deadline passes
        // before either of those happens (see sweep_expired_waiters()).
        std::map<waiter_key, waiters_t> waiters_;

        // Earliest deadline among all waiters currently in waiters_, or
        // std::chrono::steady_clock::time_point::max() if waiters_ is empty.
        // Let sweep_expired_waiters() skip the full-map scan performed by
        // drain_expired_waiters_locked() when no waiter could possibly have
        // expired yet; kept up to date whenever waiters_ is mutated (see
        // await_terminal(), drain_expired_waiters_locked(), and
        // recompute_earliest_deadline_locked()).
        std::chrono::steady_clock::time_point earliest_deadline_ =
            (std::chrono::steady_clock::time_point::max) ();

        // Independent backstop for sweep_expired_waiters(): guarantees that
        // idle targets (whose locality sees no further await_terminal() or
        // publish_event() call) still get their expired waiters swept and
        // invalidated at or after each waiter's deadline, instead of relying
        // solely on such later activity to trigger the opportunistic sweep
        // calls. Armed/re-armed via arm_sweep_timer_locked(), see there for
        // details; drained via shutting_down_/callbacks_in_flight_ (below) in
        // the destructor before any other member is destroyed.
        hpx::util::pool_timer sweep_timer_;

        // The deadline sweep_timer_ is currently armed for, or
        // steady_clock::time_point::max() if it is not currently armed. Kept in
        // sync with sweep_timer_ under mtx_ so arm_sweep_timer_locked() can
        // tell whether the timer needs to be (re-)armed for an earlier deadline
        // than the one it is already running for.
        std::chrono::steady_clock::time_point armed_deadline_ =
            (std::chrono::steady_clock::time_point::max) ();

        // Set by the destructor, under mtx_, before waiting for any
        // sweep_timer_callback() invocation already in flight (tracked by
        // callbacks_in_flight_) to finish; checked by sweep_timer_callback()
        // under mtx_ before it touches any other member, so a callback
        // dispatched concurrently with destruction never touches this object
        // once teardown has begun. This is needed because pool_timer::stop()
        // only cancels a not-yet-fired wait: it does not wait for an
        // already-dispatched callback invocation (sweep_timer_ is bound to a
        // raw `this`) to finish running.
        bool shutting_down_ = false;

        // Number of sweep_timer_callback() invocations currently executing. The
        // destructor waits (via shutdown_cv_) until this reaches zero after
        // setting shutting_down_, guaranteeing no invocation is still touching
        // mtx_/waiters_/etc. before the rest of this object's state starts
        // being destroyed. Mirrors agent_component::deactivate_and_wait() (see
        // agent_server.cpp).
        std::size_t callbacks_in_flight_ = 0;
        hpx::lcos::local::detail::condition_variable shutdown_cv_;
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
HPX_REGISTER_ACTION_DECLARATION(hpx::supervision::server::supervision_manager::
                                    register_activity_observer_action,
    supervision_register_activity_observer_action)
HPX_REGISTER_ACTION_DECLARATION(hpx::supervision::server::supervision_manager::
                                    unregister_activity_observer_action,
    supervision_unregister_activity_observer_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::query_state_action,
    supervision_query_state_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::await_terminal_action,
    supervision_await_terminal_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::supervision_manager::remove_target_action,
    supervision_remove_target_action)
