//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision/supervision_api.hpp
/// \page hpx::supervision::publish_event, hpx::supervision::query_state, hpx::supervision::register_observer, hpx::supervision::unregister_observer
/// \headerfile hpx/supervision.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/supervision/supervision_fwd.hpp>

#include <chrono>
#include <cstdint>
#include <optional>

namespace hpx::supervision {

    ////////////////////////////////////////////////////////////////////////////
    // supervision API

    HPX_CXX_EXPORT enum class event : std::uint8_t {
        /// Sentinel value; no lifecycle event has been recorded yet.
        unknown,
        /// The actor/component has been initialized and is ready to run.
        started,
        /// The actor/component is actively executing.
        running,
        /// A graceful pause has been initiated for the actor/component.
        suspending,
        /// The actor/component has reached normal completion.
        completed,
        /// The actor/component has terminated abnormally.
        failed,
        /// The locality hosting the actor/component is becoming unavailable
        /// (e.g. due to node failure or planned decommissioning).
        losing_locality,
    };

    // is_terminal() is declared in supervision_fwd.hpp; its definition is
    // deferred to here since it needs the full definition of `event`.
    HPX_CXX_EXPORT constexpr bool is_terminal(event const ev) noexcept
    {
        return ev == event::completed || ev == event::failed;
    }

    /// \brief Outcome of a call to \a publish_event().
    ///
    /// \c event::completed and \c event::failed are terminal: once either
    /// has been recorded for a target within a given epoch, later terminal
    /// publications for the same target/epoch become no-ops, reported via
    /// \c already_terminal. Non-terminal transitions remain invalid.
    HPX_CXX_EXPORT enum class publish_result : std::uint8_t {
        /// The event was recorded and observers (if any) were notified.
        applied,
        /// The target had already reached a terminal event (\c completed
        /// or \c failed) within the current epoch; the call was a no-op
        /// and no observers were notified.
        already_terminal,
        /// \a epoch was lower than the target's current epoch; the call was
        /// a stale/out-of-order publication, rejected as a no-op without
        /// mutating state or notifying observers.
        stale_epoch,
    };

    /// \brief Publish a lifecycle event for a target actor on a possibly
    ///        remote locality.
    ///
    /// The function \a publish_event() asynchronously notifies the
    /// supervision manager running on \a locality that \a target has
    /// reached the lifecycle state \a ev. Registered observers of
    /// \a target are notified as a result.
    ///
    /// \param locality [in] The locality on which the supervision manager
    ///                 responsible for \a target is running.
    /// \param target   [in] The actor (or component) for which the event
    ///                 is published.
    /// \param ev       [in] The lifecycle event to publish for \a target.
    ///
    /// \param epoch    [in] The epoch this publication belongs to. Publications
    ///                 are compared against the target's current epoch:
    ///                 publications for a lower epoch are rejected as stale
    ///                 no-ops, publications for a higher epoch reset the
    ///                 target's sequence number and become the new current
    ///                 epoch, and publications for the current epoch are
    ///                 applied normally (subject to terminal-state latching).
    ///
    /// \returns        A future that becomes ready once the event has been
    ///                 recorded by the supervision manager on \a locality,
    ///                 holding \c publish_result::applied,
    ///                 \c publish_result::already_terminal if \a target had
    ///                 already reached a terminal event (\c completed or
    ///                 \c failed) within \a epoch, or
    ///                 \c publish_result::stale_epoch if \a epoch is lower
    ///                 than the target's current epoch.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a
    ///                 valid target. The returned future becomes
    ///                 exceptional if the operation fails.
    ///
    /// \note           Publishing is not idempotent: publishing the same
    ///                 event twice creates two distinct records with
    ///                 different timestamps.
    /// \note           Events are immediately visible to observers on the
    ///                 same locality as \a target; remote observers become
    ///                 aware of the event within about 1-2 parcel
    ///                 round-trips to the AGAS authority managing the
    ///                 actor's registration.
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<publish_result> publish_event(
        hpx::id_type const& locality, hpx::id_type const& target,
        hpx::supervision::event ev, std::uint64_t epoch = 0);

    /// \brief Publish a lifecycle event for a target actor on a possibly remote
    ///        locality, blocking until the operation has completed.
    ///
    /// This is the synchronous equivalent of
    /// \a publish_event(hpx::id_type const&, hpx::id_type const&, event).
    ///
    /// \param locality [in] The locality on which the supervision manager
    ///                 responsible for \a target is running.
    /// \param target   [in] The actor (or component) for which the event
    ///                 is published.
    /// \param ev       [in] The lifecycle event to publish for \a target.
    /// \param epoch    [in] The epoch this publication belongs to. See the
    ///                 asynchronous remote overload for the epoch-scoped
    ///                 semantics.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a valid
    ///                 target, unless \a ec was not pre-initialized to
    ///                 \a hpx::throws. As long as \a ec is not
    ///                 pre-initialized to \a hpx::throws this function doesn't
    ///                 throw but returns the result code using the parameter \a
    ///                 ec.
    ///
    /// \returns        \c publish_result::applied,
    ///                 \c publish_result::already_terminal if \a target had
    ///                 already reached a terminal event (\c completed or
    ///                 \c failed) within \a epoch, or
    ///                 \c publish_result::stale_epoch if \a epoch is lower
    ///                 than the target's current epoch.
    ///
    /// \note           Publishing non-terminal events is not idempotent:
    ///                 publishing the same event twice creates two distinct
    ///                 records with different timestamps. \c event::completed
    ///                 and \c event::failed are latched: the first terminal
    ///                 publication for a target wins, and every later terminal
    ///                 publication for that target is a no-op that returns \c
    ///                 publish_result::already_terminal.
    HPX_CXX_EXPORT HPX_EXPORT publish_result publish_event(
        hpx::launch::sync_policy, hpx::id_type const& locality,
        hpx::id_type const& target, hpx::supervision::event ev,
        std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws);

    /// \brief Publish a lifecycle event for a target actor on the local
    ///        locality.
    ///
    /// This overload publishes \a ev directly through the local
    /// supervision manager without going through AGAS or a remote parcel.
    /// It is intended to be called from within an actor or action running
    /// on the same locality as \a target.
    ///
    /// \param target [in] The actor (or component) for which the event is
    ///               published. Must be local to the calling locality.
    /// \param ev     [in] The lifecycle event to publish for \a target.
    /// \param epoch  [in] The epoch this publication belongs to. See the
    ///               asynchronous remote overload for the epoch-scoped
    ///               semantics.
    /// \param ec     [in,out] this represents the error status on exit, if
    ///               this is pre-initialized to \a hpx::throws the
    ///               function will throw on error instead.
    ///
    /// \throws       hpx::exception if \a target does not represent a
    ///               valid target, unless \a ec was not pre-initialized to
    ///               \a hpx::throws.
    ///
    /// \returns      \c publish_result::applied,
    ///               \c publish_result::already_terminal if \a target had
    ///               already reached a terminal event (\c completed or
    ///               \c failed) within \a epoch, or
    ///               \c publish_result::stale_epoch if \a epoch is lower
    ///               than the target's current epoch.
    ///
    /// \note         Publishing non-terminal events is not idempotent:
    ///               publishing the same event twice creates two distinct
    ///               records with different timestamps. \c event::completed
    ///               and \c event::failed are latched: the first terminal
    ///               publication for a target wins, and every later
    ///               terminal publication for that target is a no-op that
    ///               returns \c publish_result::already_terminal.
    /// \note         Local observers of \a target are notified
    ///               synchronously as part of this call.
    HPX_CXX_EXPORT HPX_EXPORT publish_result publish_event(
        hpx::id_type const& target, hpx::supervision::event ev,
        std::uint64_t epoch = 0, hpx::error_code& ec = throws);

    /// \brief Snapshot of the most recently observed lifecycle event for a
    ///        supervised actor.
    ///
    /// A `lifecycle_state` value is returned by \ref hpx::supervision::query_state
    /// and represents the last event recorded in the runtime's event log for the
    /// queried actor.
    ///
    /// \par Semantics:
    /// - Reflects the last event observed locally, i.e. the most recent value in
    ///   the runtime's event log for \ref actor.
    /// - If \ref actor is remote, the query is forwarded to the actor's home
    ///   locality and completes with a latency of roughly one parcel round-trip.
    /// - If the query originates from a remote caller, \ref ec may carry a
    ///   staleness error code indicating that the returned state could be stale
    ///   because the corresponding notification parcel has not yet been
    ///   delivered to the caller's locality.
    /// - \ref event_sequence_number allows callers to detect gaps, e.g. when an
    ///   observer callback was skipped due to a network failure.
    ///
    /// \see hpx::supervision::event
    /// \see hpx::supervision::query_state
    /// \see hpx::supervision::lifecycle_event_notification
    HPX_CXX_EXPORT struct lifecycle_state
    {
        /// The global id of the actor this state describes.
        hpx::id_type actor;

        /// The most recently observed lifecycle event for \ref actor. Defaults
        /// to \c hpx::supervision::event::unknown if no event has been recorded
        /// yet.
        event last_event = supervision::event::unknown;

        /// The time at which \ref last_event was recorded, as observed by the
        /// runtime managing \ref actor's registration.
        std::chrono::steady_clock::time_point timestamp;

        /// Monotonically increasing sequence number associated with
        /// \ref last_event. Enables callers to detect gaps caused by dropped or
        /// undelivered notifications.
        std::uint64_t event_sequence_number = 0;

        /// The epoch \ref last_event was published under. A publication for a
        /// higher epoch than previously seen resets \ref event_sequence_number
        /// and becomes the target's new current epoch; a publication for a
        /// lower epoch is rejected as stale.
        std::uint64_t epoch = 0;

        /// Error code describing the outcome of the query, or a staleness
        /// indicator when the query was served from a remote/observer-side
        /// cache. A successful query yields \c hpx::make_success_code().
        hpx::error_code ec = hpx::make_success_code();
    };

    /// \brief Determine whether transitioning from lifecycle event \a from
    ///        to lifecycle event \a to is permitted by the supervision
    ///        lifecycle state machine.
    ///
    /// \a completed and \a failed are terminal states and have no outgoing
    /// transitions. \a losing_locality is only reachable from \a started,
    /// \a running, or \a suspending, and may only transition to \a failed.
    /// A missing prior event is represented as \a event::unknown, from
    /// which the only valid transition is to \a started.
    HPX_CXX_EXPORT HPX_EXPORT bool is_valid_transition(
        event from, event to) noexcept;

    /// \brief Query the last known lifecycle state of a target actor on a
    ///        possibly remote locality.
    ///
    /// The function \a query_state() asynchronously retrieves the most
    /// recently published lifecycle event for \a target, as observed by
    /// the supervision manager running on \a locality.
    ///
    /// \param locality [in] The locality on which the supervision manager
    ///                 responsible for \a target is running.
    /// \param target   [in] The actor (or component) whose state is
    ///                 queried.
    ///
    /// \returns        A future holding the last event recorded for
    ///                 \a target (i.e., the most recent value in the
    ///                 runtime's event log), together with its timestamp
    ///                 and sequence number.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a
    ///                 valid target. The returned future becomes
    ///                 exceptional if the operation fails.
    ///
    /// \note           If \a target is not local to \a locality, the query
    ///                 is forwarded to the actor's home locality, adding
    ///                 about one parcel round-trip of latency.
    /// \note           The returned \a lifecycle_state::ec may carry a
    ///                 staleness error code, indicating that the returned
    ///                 state may be stale if a preceding observer parcel
    ///                 has not yet been delivered.
    /// \note           The \a lifecycle_state::event_sequence_number
    ///                 allows clients to detect gaps, e.g. if an observer
    ///                 callback was skipped due to a network failure.
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<lifecycle_state> query_state(
        hpx::id_type const& locality, hpx::id_type const& target);

    /// \brief Query the last known lifecycle state of a target actor on a
    ///        possibly remote locality, blocking until the result is available.
    ///
    /// This is the synchronous equivalent of
    /// \a query_state(hpx::id_type const&, hpx::id_type const&).
    ///
    /// \param locality [in] The locality on which the supervision manager
    ///                 responsible for \a target is running.
    /// \param target   [in] The actor (or component) whose state is
    ///                 queried.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \returns        The last event recorded for \a target, together
    ///                 with its timestamp and sequence number.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a
    ///                 valid target, unless \a ec was not pre-initialized to
    ///                 \a hpx::throws.
    ///
    /// \note           The returned \a lifecycle_state::ec may carry a
    ///                 staleness error code, indicating that the returned
    ///                 state may be stale if a preceding observer parcel
    ///                 has not yet been delivered.
    HPX_CXX_EXPORT HPX_EXPORT lifecycle_state query_state(
        hpx::launch::sync_policy, hpx::id_type const& locality,
        hpx::id_type const& target, hpx::error_code& ec = hpx::throws);

    /// \brief Query the last known lifecycle state of a target actor on
    ///        the local locality.
    ///
    /// \param target [in] The actor (or component) whose state is
    ///               queried. Must be local to the calling locality.
    /// \param ec     [in,out] this represents the error status on exit, if
    ///               this is pre-initialized to \a hpx::throws the
    ///               function will throw on error instead.
    ///
    /// \returns      The last event recorded locally for \a target,
    ///               together with its timestamp and sequence number.
    ///
    /// \throws       hpx::exception if \a target does not represent a
    ///               valid target, unless \a ec was not pre-initialized to
    ///               \a hpx::throws.
    HPX_CXX_EXPORT HPX_EXPORT lifecycle_state query_state(
        hpx::id_type const& target, hpx::error_code& ec = hpx::throws);

    /// \brief Payload delivered to a registered lifecycle observer callback
    ///        whenever a supervised actor publishes a lifecycle event.
    ///
    /// A `lifecycle_event_notification` is constructed by the supervision
    /// manager each time \ref hpx::supervision::publish_event is invoked for
    /// a target actor that has one or more registered observers (see
    /// \ref hpx::supervision::register_observer). It is delivered synchronously
    /// to local observers within the publishing call, and asynchronously via a
    /// dedicated parcel to remote observers.
    ///
    /// \note Instances are serializable so that they may be transmitted to
    ///       remote observers across localities.
    ///
    /// \see hpx::supervision::event
    /// \see hpx::supervision::lifecycle_callback
    /// \see hpx::supervision::register_observer
    /// \see hpx::supervision::publish_event
    HPX_CXX_EXPORT struct lifecycle_event_notification
    {
        /// The global id of the actor that published the event.
        hpx::id_type actor;

        /// The lifecycle event that was published. Defaults to
        /// \c hpx::supervision::event::unknown if no event has been recorded.
        supervision::event event = supervision::event::unknown;

        /// The time at which the event was published, as observed by the
        /// runtime managing \ref actor's registration.
        std::chrono::steady_clock::time_point event_time;

        /// Monotonically increasing sequence number assigned to this event.
        /// Allows observers to detect gaps caused by dropped or undelivered
        /// notifications (e.g., due to network failure).
        std::uint64_t event_sequence_number = 0;

        /// The epoch \ref event was published under.
        std::uint64_t epoch = 0;

        /// Error code describing the outcome of delivering this notification.
        /// A successful delivery yields \c hpx::make_success_code(); a non-success
        /// code may indicate staleness or a delivery failure that the observer
        /// callback should account for.
        hpx::error_code ec = hpx::make_success_code();
    };

#if defined(DOXYGEN)
    /// \brief Callback type used to observe lifecycle events for a supervised
    ///        actor.
    ///
    /// The callback is invoked with the \ref lifecycle_event_notification
    /// describing the event that occurred. Delivery status (e.g. staleness
    /// or delivery failures for remote observers) is reported via
    /// \c notification.ec.
    ///
    /// \returns `true` if the observer should stay registered, `false`
    ///          otherwise (i.e., `false` will unregister the observer and no
    ///          further callbacks will be invoked for the given observer).
    ///
    /// \note Any exception thrown from the callback is logged and does not
    ///       affect observer registration.
    /// \note The callback must not block indefinitely; local observers are
    ///       invoked synchronously from within the call that publishes the
    ///       event.
    HPX_CXX_EXPORT using lifecycle_callback =
        std::function<bool(lifecycle_event_notification const&)>;
#endif

    /// \brief Register a callback to observe lifecycle events published by
    ///        a target actor running on a possibly remote locality.
    ///
    /// \param locality     [in] The locality on which the callback should
    ///                     be registered.
    /// \param target       [in] The actor (or component) to observe.
    /// \param callback     [in] The callback invoked whenever \a target
    ///                     publishes a lifecycle event.
    /// \param epoch_filter [in] If set, restricts notifications to events
    ///                     published under this epoch; events published
    ///                     under any other epoch are silently skipped for
    ///                     this observer. If \c std::nullopt (the
    ///                     default), the observer is notified regardless
    ///                     of epoch.
    ///
    /// \returns        A future holding the global id of the observer
    ///                 handle. This handle must be passed to
    ///                 \a unregister_observer() in order to stop receiving
    ///                 notifications.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a
    ///                 valid target. The returned future becomes
    ///                 exceptional if the operation fails.
    ///
    /// \note           If \a locality is the locality \a target is running
    ///                 on, the callback is invoked synchronously from
    ///                 within the publish call (best effort; the callback
    ///                 must not block indefinitely).
    /// \note           If \a locality differs from the locality \a target
    ///                 is running on, the callback is invoked
    ///                 asynchronously via a dedicated parcel, with retry
    ///                 semantics (e.g., 3 attempts over 500 ms before
    ///                 logging failure).
    /// \note           The callback must not throw; exceptions thrown from
    ///                 it are logged and do not affect the observer
    ///                 registration.
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<hpx::id_type> register_observer(
        hpx::id_type const& locality, hpx::id_type const& target,
        lifecycle_callback const& callback,
        std::optional<std::uint64_t> epoch_filter = std::nullopt);

    /// \brief Register a callback to observe lifecycle events published by
    ///        a target actor running on a possibly remote locality, blocking
    ///        until the registration has completed.
    ///
    /// This is the synchronous equivalent of
    /// \a register_observer(hpx::id_type const&, hpx::id_type const&,
    /// lifecycle_callback const&).
    ///
    /// \param locality     [in] The locality on which the callback should
    ///                     be registered.
    /// \param target       [in] The actor (or component) to observe.
    /// \param callback     [in] The callback invoked whenever \a target
    ///                     publishes a lifecycle event.
    /// \param epoch_filter [in] If set, restricts notifications to events
    ///                     published under this epoch; see the
    ///                     asynchronous overload for details.
    /// \param ec           [in,out] this represents the error status on
    ///                     exit, if this is pre-initialized to
    ///                     \a hpx::throws the function will throw on error
    ///                     instead.
    ///
    /// \returns        The global id of the observer handle. This handle
    ///                 must be passed to \a unregister_observer() in order
    ///                 to stop receiving notifications.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a
    ///                 valid target, unless \a ec was not pre-initialized to
    ///                 \a hpx::throws.
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type register_observer(
        hpx::launch::sync_policy, hpx::id_type const& locality,
        hpx::id_type const& target, lifecycle_callback const& callback,
        std::optional<std::uint64_t> epoch_filter = std::nullopt,
        hpx::error_code& ec = hpx::throws);

    /// \brief Register a callback to observe lifecycle events published by
    ///        a target actor on the local locality.
    ///
    /// \param target       [in] The actor (or component) to observe. Must
    ///                     be local to the calling locality.
    /// \param callback     [in] The callback invoked whenever \a target
    ///                     publishes a lifecycle event.
    /// \param epoch_filter [in] If set, restricts notifications to events
    ///                     published under this epoch; events published
    ///                     under any other epoch are silently skipped for
    ///                     this observer. If \c std::nullopt (the
    ///                     default), the observer is notified regardless
    ///                     of epoch.
    /// \param ec           [in,out] this represents the error status on
    ///                     exit, if this is pre-initialized to
    ///                     \a hpx::throws the function will throw on error
    ///                     instead.
    ///
    /// \returns        The global id of the observer handle. This handle
    ///                 must be passed to \a unregister_observer() in order
    ///                 to stop receiving notifications.
    ///
    /// \throws         hpx::exception if \a target does not represent a
    ///                 valid target, unless \a ec was initialized to
    ///                 \a hpx::throws.
    ///
    /// \note           The callback is invoked synchronously from within
    ///                 the publish call (best effort; the callback must
    ///                 not block indefinitely) and must not throw;
    ///                 exceptions thrown from it are logged and do not
    ///                 affect the observer registration.
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type register_observer(
        hpx::id_type const& target, lifecycle_callback const& callback,
        std::optional<std::uint64_t> epoch_filter = std::nullopt,
        hpx::error_code& ec = hpx::throws);

    /// \brief Unregister a previously registered observer on a possibly
    ///        remote locality.
    ///
    /// \param locality        [in] The locality on which the observer was
    ///                        registered.
    /// \param observer_handle [in] The handle returned by a prior call to
    ///                        \a register_observer().
    ///
    /// \returns               A future that becomes ready once the
    ///                        observer has been unregistered.
    ///
    /// \throws                hpx::exception if \a locality does not
    ///                        represent a locality, or if
    ///                        \a observer_handle does not represent a
    ///                        valid observer handle. The returned future
    ///                        becomes exceptional if the operation
    ///                        fails.
    ///
    /// \note                  Once the returned future has become ready,
    ///                        no orphaned callbacks fire after
    ///                        unregistration completes.
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<void> unregister_observer(
        hpx::id_type const& locality, hpx::id_type const& observer_handle);

    /// \brief Unregister a previously registered observer on a possibly
    ///        remote locality, blocking until the operation has completed.
    ///
    /// This is the synchronous equivalent of
    /// \a unregister_observer(hpx::id_type const&, hpx::id_type const&).
    ///
    /// \param locality        [in] The locality on which the observer was
    ///                        registered.
    /// \param observer_handle [in] The handle returned by a prior call to
    ///                        \a register_observer().
    /// \param ec              [in,out] this represents the error status on
    ///                        exit, if this pre-initialized to
    ///                        \a hpx::throws the function will throw on
    ///                        error instead.
    ///
    /// \throws                hpx::exception if \a locality does not
    ///                        represent a locality, or if
    ///                        \a observer_handle does not represent a
    ///                        valid observer handle, unless \a ec was
    ///                        initialized not to \a hpx::throws.
    ///
    /// \note                  No orphaned callbacks fire after
    ///                        unregistration completes.
    HPX_CXX_EXPORT HPX_EXPORT void unregister_observer(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::id_type const& observer_handle,
        hpx::error_code& ec = hpx::throws);

    /// \brief Unregister a previously registered observer on the local
    ///        locality.
    ///
    /// \param observer_handle [in] The handle returned by a prior call to
    ///                        \a register_observer(). Must be local to the
    ///                        calling locality.
    /// \param ec              [in,out] this represents the error status on
    ///                        exit, if this is pre-initialized to
    ///                        \a hpx::throws the function will throw on
    ///                        error instead.
    ///
    /// \throws                hpx::exception if \a observer_handle does
    ///                        not represent a valid observer handle,
    ///                        unless \a ec was not pre-initialized to
    ///                        \a hpx::throws.
    ///
    /// \note                  No orphaned callbacks fire after
    ///                        unregistration completes.
    HPX_CXX_EXPORT HPX_EXPORT void unregister_observer(
        hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws);
}    // namespace hpx::supervision
