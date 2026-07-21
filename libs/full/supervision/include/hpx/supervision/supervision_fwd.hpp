//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision/supervision_fwd.hpp
/// \page hpx::supervision::publish_event, hpx::supervision::query_state, hpx::supervision::register_observer, hpx::supervision::unregister_observer
/// \headerfile hpx/supervision.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <chrono>
#include <cstdint>

namespace hpx::supervision {

    /// \cond NOINTERNAL
    ///
    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline constexpr char const* const service_name =
        "/0/supervision/";

    HPX_CXX_EXPORT inline constexpr std::uint64_t supervision_manager_msb =
        0x100000011ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t supervision_manager_lsb =
        0x000000011ULL;

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class HPX_EXPORT supervision_manager;

    namespace server {

        HPX_CXX_EXPORT struct HPX_EXPORT supervision_manager;
    }    // namespace server

    HPX_CXX_EXPORT HPX_EXPORT supervision_manager& get_supervision_manager();
    /// \endcond

    ////////////////////////////////////////////////////////////////////////////
    // supervision API

    enum class event : std::uint8_t
    {
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
    /// \returns        A future that becomes ready once the event has been
    ///                 recorded by the supervision manager on \a locality.
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
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<void> publish_event(
        hpx::id_type const& locality, hpx::id_type const& target,
        hpx::supervision::event ev);

    /// \brief Publish a lifecycle event for a target actor on a possibly
    ///        remote locality, blocking until the operation has completed.
    ///
    /// This is the synchronous equivalent of
    /// \a publish_event(hpx::id_type const&, hpx::id_type const&, event).
    ///
    /// \param locality [in] The locality on which the supervision manager
    ///                 responsible for \a target is running.
    /// \param target   [in] The actor (or component) for which the event
    ///                 is published.
    /// \param ev       [in] The lifecycle event to publish for \a target.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \throws         hpx::exception if \a locality does not represent a
    ///                 locality, or if \a target does not represent a
    ///                 valid target, unless \a ec was initialized to
    ///                 \a hpx::throws. As long as \a ec is not
    ///                 pre-initialized to \a hpx::throws this function
    ///                 doesn't throw but returns the result code using
    ///                 the parameter \a ec.
    ///
    /// \note           Publishing is not idempotent: publishing the same
    ///                 event twice creates two distinct records with
    ///                 different timestamps.
    HPX_CXX_EXPORT HPX_EXPORT void publish_event(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::id_type const& target,
        hpx::supervision::event ev, hpx::error_code& ec = hpx::throws);

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
    /// \param ec     [in,out] this represents the error status on exit, if
    ///               this is pre-initialized to \a hpx::throws the
    ///               function will throw on error instead.
    ///
    /// \throws       hpx::exception if \a target does not represent a
    ///               valid target, unless \a ec was not pre-initialized to
    ///               \a hpx::throws.
    ///
    /// \note         Publishing is not idempotent: publishing the same
    ///               event twice creates two distinct records with
    ///               different timestamps.
    /// \note         Local observers of \a target are notified
    ///               synchronously as part of this call.
    HPX_CXX_EXPORT HPX_EXPORT void publish_event(hpx::id_type const& target,
        hpx::supervision::event ev, hpx::error_code& ec = throws);

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
    struct lifecycle_state
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

        /// Error code describing the outcome of the query, or a staleness
        /// indicator when the query was served from a remote/observer-side
        /// cache. A successful query yields \c hpx::make_success_code().
        hpx::error_code ec = hpx::make_success_code();
    };

    /// \cond NOINTERNAL
    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::output_archive& ar, lifecycle_state const&,
        unsigned int);
    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::input_archive& ar, lifecycle_state&, unsigned int);
    /// \endcond

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
    struct lifecycle_event_notification
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

        /// Error code describing the outcome of delivering this notification.
        /// A successful delivery yields \c hpx::make_success_code(); a non-success
        /// code may indicate staleness or a delivery failure that the observer
        /// callback should account for.
        hpx::error_code ec = hpx::make_success_code();
    };

    /// \cond NOINTERNAL
    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::output_archive& ar,
        lifecycle_event_notification const&, unsigned int);
    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::input_archive& ar, lifecycle_event_notification&,
        unsigned int);
    /// \endcond

    /// \brief Callback type used to observe lifecycle events for a supervised
    ///        actor.
    ///
    /// The callback is invoked with the \ref lifecycle_event_notification
    /// describing the event that occurred, and a \c hpx::error_code that
    /// reports the outcome of delivering the notification (e.g. staleness or
    /// delivery failures for remote observers).
    ///
    /// \note The callback must not throw. Any exception thrown from the
    ///       callback is logged and does not affect observer registration.
    /// \note The callback must not block indefinitely; local observers are
    ///       invoked synchronously from within the call that publishes the
    ///       event.
    using lifecycle_callback = std::function<void(
        lifecycle_event_notification const&, hpx::error_code&)>;

    /// \brief Register a callback to observe lifecycle events published by
    ///        a target actor running on a possibly remote locality.
    ///
    /// \param locality [in] The locality on which the callback should be
    ///                 registered.
    /// \param target   [in] The actor (or component) to observe.
    /// \param callback [in] The callback invoked whenever \a target
    ///                 publishes a lifecycle event.
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
        lifecycle_callback const& callback);

    /// \brief Register a callback to observe lifecycle events published by
    ///        a target actor running on a possibly remote locality, blocking
    ///        until the registration has completed.
    ///
    /// This is the synchronous equivalent of
    /// \a register_observer(hpx::id_type const&, hpx::id_type const&,
    /// lifecycle_callback const&).
    ///
    /// \param locality [in] The locality on which the callback should be
    ///                 registered.
    /// \param target   [in] The actor (or component) to observe.
    /// \param callback [in] The callback invoked whenever \a target
    ///                 publishes a lifecycle event.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
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
        hpx::error_code& ec = hpx::throws);

    /// \brief Register a callback to observe lifecycle events published by
    ///        a target actor on the local locality.
    ///
    /// \param target   [in] The actor (or component) to observe. Must be
    ///                 local to the calling locality.
    /// \param callback [in] The callback invoked whenever \a target
    ///                 publishes a lifecycle event.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
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
