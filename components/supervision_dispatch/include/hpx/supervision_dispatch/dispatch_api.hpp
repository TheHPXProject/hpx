//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision_dispatch/dispatch_api.hpp
/// \page hpx::supervision::init, hpx::supervision::finalize, hpx::supervision::is_initialized
/// \headerfile hpx/supervision_dispatch.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/supervision_dispatch/discovery.hpp>
#include <hpx/supervision_dispatch/export_definitions.hpp>

#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

    /// A small, copyable handle to the sentinel/registry pair owned by a
    /// successful init() call, returned once the supervision-dispatch
    /// runtime for this locality has reached the \a active state.
    struct supervision_handle
    {
        sentinel sentinel_client;
        registry registry_client;
    };

    /// Performs one-shot, idempotent initialization of the supervision-dispatch
    /// runtime for this locality. On the first successful call, this function
    /// atomically transitions the internal lifecycle state from
    /// \a uninitialized to \a initializing, after incrementing the internal
    /// epoch, then owns the entire startup sequence: it creates a local \a
    /// sentinel and \a registry for hpx::find_here(), publishes \a
    /// event::started on the sentinel *before* either component's symbol name
    /// is registered (so that no peer can discover and join a not-yet-started
    /// sentinel), registers both symbol names, and finally performs a single
    /// discover_and_join() pass to join every currently reachable peer
    /// registry. On success the state transitions to \a active; if any step
    /// throws, all partial state is rolled back and the lifecycle reverts to \a
    /// uninitialized so that a subsequent call can retry from scratch.
    ///
    /// Calling \a init() while already \a active is a no-op that resolves
    /// immediately without repeating any side effect. Calling it while another
    /// call is still \a initializing attaches the caller to the in-flight
    /// operation's outcome rather than racing it, so concurrent callers never
    /// create more than one sentinel/registry pair. Calling it while \a
    /// finalizing is in progress is rejected; callers should wait for
    /// finalize() to complete before re-initializing.
    ///
    /// \param discovery_timeout The maximum duration to wait, across all
    ///                          discovery candidates combined, for peer
    ///                          sentinel/registry symbol names to resolve
    ///                          during the discover_and_join() step (see
    ///                          discover_peers()).
    ///
    /// \return A shared future that becomes ready, holding a
    ///         \a supervision_handle for the sentinel/registry pair owned by
    ///         this init() cycle, once the supervision runtime has reached the
    ///         \a active state, or that becomes exceptional if initialization
    ///         fails.
    ///
    /// \note This function has no effect on any state that has not yet been
    ///       created by a successful call -- in particular, no symbol name is
    ///       ever registered and no event is ever published unless this
    ///       function (or a concurrent call it is attached to) actually runs
    ///       the initialization sequence.
    ///
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::shared_future<supervision_handle> init(
        hpx::chrono::steady_duration const& discovery_timeout =
            default_discovery_timeout);

    /// Performs one-shot, idempotent initialization of the supervision-dispatch
    /// runtime for this locality, blocking the calling thread until the
    /// operation completes. This is the synchronous counterpart of
    /// \a init(hpx::chrono::steady_duration const&): it carries out the exact
    /// same lifecycle transition and startup sequence (sentinel/registry
    /// creation, \a event::started publication, symbol name registration, and a
    /// single discover_and_join() pass), but waits for the resulting future to
    /// become ready (or exceptional) before returning, rather than handing the
    /// future back to the caller.
    ///
    /// \param policy              Tag selecting the blocking overload of
    ///                            \a init().
    /// \param discovery_timeout   The maximum duration to wait, across all
    ///                            discovery candidates combined, for peer
    ///                            sentinel/registry symbol names to resolve
    ///                            during the discover_and_join() step (see
    ///                            discover_peers()).
    ///
    /// \return A \a supervision_handle for the sentinel/registry pair owned
    ///         by this init() cycle.
    ///
    /// \throws hpx::exception if initialization could not complete (e.g. due to
    ///         a concurrent finalize() still in progress). Calling this while
    ///         already \a active is a no-op that returns immediately.
    ///
    /// \note As with the asynchronous overload, no symbol name is registered
    ///       and no event is published unless this call (or a concurrent call
    ///       it attaches to) actually performs the initialization sequence.
    HPX_SUPERVISION_DISPATCH_EXPORT supervision_handle init(
        hpx::launch::sync_policy policy,
        hpx::chrono::steady_duration const& discovery_timeout =
            default_discovery_timeout);

    /// Performs one-shot, idempotent teardown of the supervision-dispatch
    /// runtime previously started by init(). On the first call observing the \a
    /// active state, this function atomically transitions the lifecycle to \a
    /// finalizing, publishes \a event::completed for the local sentinel at the
    /// new epoch, unregisters both the sentinel and registry symbol names, and
    /// then releases ownership of both components (relying on \a client_base's
    /// default lifetime management to destroy them once no other references
    /// remain) before resetting the lifecycle back to \a uninitialized,
    /// allowing a subsequent init() call to re-arm the runtime from scratch.
    ///
    /// Calling \a finalize() when the runtime is \a uninitialized, still \a
    /// initializing, or already \a finalizing is a documented no-op: it
    /// resolves immediately without publishing any event, unregistering any
    /// symbol name, or otherwise touching component state.
    ///
    /// \note Because \a finalize() is a no-op unless the runtime is currently
    ///       \a active, it is always safe to call speculatively, e.g.
    ///       during shutdown, without first checking is_initialized().
    HPX_SUPERVISION_DISPATCH_EXPORT void finalize();

    /// Returns whether the supervision-dispatch runtime for this locality is
    /// currently \a active, i.e. has completed init() and has not yet been torn
    /// down by finalize().
    ///
    /// \return \a true if the lifecycle state is \a active, \a false if it is
    ///         \a uninitialized, \a initializing, or \a finalizing.
    ///
    /// \note This function never blocks and performs a single atomic load; it
    ///       is safe to call from any thread, including while init() or
    ///       finalize() is concurrently in flight.
    ///
    HPX_SUPERVISION_DISPATCH_EXPORT bool is_initialized() noexcept;

    /// \brief Query the current dispatch-cycle epoch of the supervision-
    ///        dispatch runtime on a possibly remote locality.
    ///
    /// This is the counter maintained internally by \a init() (incremented
    /// once per successful \a init() call on \a locality, and used as the
    /// epoch \a locality's own sentinel publishes \c event::started under -
    /// see \a run_init_sequence()). It is unrelated to any particular
    /// target's \a lifecycle_state::epoch (see \a query_state()): in
    /// particular, \a server::registry::join() uses this to seed a freshly
    /// joined peer's local shadow from the peer's own real epoch instead of
    /// guessing 0, so that shadow does not go stale the moment the peer's own
    /// first lifecycle notification mirrors in (see
    /// \a server::registry::register_observers()).
    ///
    /// \param locality [in] The locality whose dispatch-cycle epoch is
    ///                 queried.
    ///
    /// \return A future holding \a locality's current epoch (0 if \a init()
    ///         has never successfully completed on \a locality).
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::future<std::uint64_t> current_epoch(
        hpx::id_type const& locality);

    /// \brief Query the current dispatch-cycle epoch of the supervision-
    ///        dispatch runtime on a possibly remote locality, blocking until
    ///        the result is available.
    ///
    /// This is the synchronous equivalent of
    /// \a current_epoch(hpx::id_type const&).
    ///
    /// \param policy   Tag selecting the blocking overload of
    ///                 \a current_epoch().
    /// \param locality [in] The locality whose dispatch-cycle epoch is
    ///                 queried.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \return \a locality's current epoch (0 if \a init() has never
    ///         successfully completed on \a locality).
    HPX_SUPERVISION_DISPATCH_EXPORT std::uint64_t current_epoch(
        hpx::launch::sync_policy policy, hpx::id_type const& locality,
        hpx::error_code& ec = hpx::throws);

    /// \brief Query the last known lifecycle state of the sentinel owned by
    ///        \a handle, on the local locality.
    ///
    /// Convenience overload of
    /// \a hpx::supervision::query_state(hpx::id_type const&, hpx::id_type
    /// const&) that resolves the target from \a handle instead of requiring the
    /// caller to name it explicitly. Forwards to
    /// `hpx::supervision::query_state(hpx::find_here(),
    /// handle.sentinel_client.get_id())`.
    ///
    /// \param handle [in] The supervision_handle (typically returned by
    ///               init()) whose sentinel_client's state is queried.
    ///
    /// \return A future holding the last event recorded for
    ///         \a handle.sentinel_client, together with its timestamp,
    ///         sequence number, and epoch (see \a lifecycle_state::epoch -- in
    ///         particular, useful for recovering the epoch \a init() started
    ///         this handle's sentinel at, for use with publish_event()).
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::future<lifecycle_state> query_state(
        supervision_handle const& handle);

    /// \brief Query the last known lifecycle state of the sentinel owned by
    ///        \a handle, blocking until the result is available.
    ///
    /// This is the synchronous equivalent of
    /// \a query_state(supervision_handle const&).
    ///
    /// \param policy Tag selecting the blocking overload of query_state().
    /// \param handle [in] The supervision_handle whose sentinel_client's
    ///               state is queried.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \throws         hpx::exception if \a handle does not hold a valid
    ///                 sentinel client, unless \a ec was not pre-initialized to
    ///                 \a hpx::throws.
    ///
    /// \return The last event recorded for \a handle.sentinel_client.
    HPX_SUPERVISION_DISPATCH_EXPORT lifecycle_state query_state(
        hpx::launch::sync_policy policy, supervision_handle const& handle,
        hpx::error_code& ec = hpx::throws);

    /// \brief Query the last known lifecycle state of a discovered peer's
    ///        sentinel.
    ///
    /// Convenience overload of
    /// \a hpx::supervision::query_state(hpx::id_type const&, hpx::id_type
    /// const&) that resolves the (locality, target) pair from \a peer instead
    /// of requiring the caller to name them explicitly. Forwards to
    /// `hpx::supervision::query_state(peer.locality,
    /// peer.sentinel_client.get_id())`.
    ///
    /// \param handle [in] Unused beyond overload resolution and symmetry
    ///               with the self-query overload above; \a peer alone carries
    ///               everything this call needs.
    /// \param peer   [in] The peer (typically returned by discover_peers()/
    ///               discover_and_join()) whose sentinel's state is queried.
    ///
    /// \return A future holding the last event recorded for
    ///         \a peer.sentinel_client, as observed by the supervision
    ///         manager running on \a peer.locality.
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::future<lifecycle_state> query_state(
        supervision_handle const& handle, discovered_peer const& peer);

    /// \brief Query the last known lifecycle state of a discovered peer's
    ///        sentinel, blocking until the result is available.
    ///
    /// This is the synchronous equivalent of
    /// \a query_state(supervision_handle const&, discovered_peer const&).
    ///
    /// \param policy Tag selecting the blocking overload of query_state().
    /// \param handle [in] Unused beyond overload resolution; see the
    ///               asynchronous overload.
    /// \param peer   [in] The peer whose sentinel's state is queried.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \throws         hpx::exception if \a handle does not hold a valid
    ///                 sentinel client, unless \a ec was not pre-initialized to
    ///                 \a hpx::throws.
    ///
    /// \return The last event recorded for \a peer.sentinel_client.
    HPX_SUPERVISION_DISPATCH_EXPORT lifecycle_state query_state(
        hpx::launch::sync_policy policy, supervision_handle const& handle,
        discovered_peer const& peer, hpx::error_code& ec = hpx::throws);

    /// \brief Publish a lifecycle event for the sentinel owned by \a handle.
    ///
    /// Convenience overload of
    /// \a hpx::supervision::publish_event(hpx::id_type const&, hpx::id_type
    /// const&, event, std::uint64_t) that resolves locality and target from
    /// \a handle instead of requiring the caller to name them explicitly.
    /// Forwards to `hpx::supervision::publish_event(hpx::find_here(),
    /// handle.sentinel_client.get_id(), ev, epoch)`.
    ///
    /// \param handle [in] The supervision_handle (typically returned by
    ///               init()) whose sentinel_client the event is published for.
    /// \param ev     [in] The lifecycle event to publish.
    /// \param epoch  [in] The epoch this publication belongs to. Typically
    ///               obtained from a prior `query_state(hpx::launch::sync,
    ///               handle).epoch` call. See the raw-id overload for the
    ///               epoch-scoped semantics.
    ///
    /// \return A future that becomes ready once the event has been recorded,
    ///         holding the same \a publish_result values as the raw-id
    ///         overload.
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::future<publish_result> publish_event(
        supervision_handle const& handle, event ev, std::uint64_t epoch = 0);

    /// \brief Publish a lifecycle event for the sentinel owned by \a handle,
    ///        blocking until the operation has completed.
    ///
    /// This is the synchronous equivalent of
    /// \a publish_event(supervision_handle const&, event, std::uint64_t).
    ///
    /// \param policy [in] Tag selecting the blocking overload of
    ///               publish_event().
    /// \param handle [in] The supervision_handle whose sentinel_client the
    ///               event is published for.
    /// \param ev     [in] The lifecycle event to publish.
    /// \param epoch  [in] The epoch this publication belongs to. See the
    ///               asynchronous overload.
    /// \param ec       [in,out] this represents the error status on exit,
    ///                 if this is pre-initialized to \a hpx::throws the
    ///                 function will throw on error instead.
    ///
    /// \throws         hpx::exception if \a handle does not hold a valid
    ///                 sentinel client, unless \a ec was not pre-initialized to
    ///                 \a hpx::throws.
    ///
    /// \return The same \a publish_result values as the raw-id overload.
    HPX_SUPERVISION_DISPATCH_EXPORT publish_result publish_event(
        hpx::launch::sync_policy policy, supervision_handle const& handle,
        event ev, std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws);

}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
