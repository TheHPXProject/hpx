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

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

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
    /// \return A shared future that becomes ready (with a void value) once the
    ///         supervision runtime has reached the \a active state, or that
    ///         becomes exceptional if initialization could not complete (e.g.
    ///         due to a concurrent finalize still in progress).
    ///
    /// \note This function has no effect on any state that has not yet been
    ///       created by a successful call -- in particular, no symbol name is
    ///       ever registered and no event is ever published unless this
    ///       function (or a concurrent call it is attached to) actually runs
    ///       the initialization sequence.
    ///
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::shared_future<void> init(
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
    /// \param launch::sync_policy Tag selecting the blocking overload of
    ///                            \a init().
    /// \param discovery_timeout   The maximum duration to wait, across all
    ///                            discovery candidates combined, for peer
    ///                            sentinel/registry symbol names to resolve
    ///                            during the discover_and_join() step (see
    ///                            discover_peers()).
    ///
    /// \throws hpx::exception if initialization could not complete (e.g. due to
    ///         a concurrent finalize() still in progress). Calling this while
    ///         already \a active is a no-op that returns immediately.
    ///
    /// \note As with the asynchronous overload, no symbol name is registered
    ///       and no event is published unless this call (or a concurrent call
    ///       it attaches to) actually performs the initialization sequence.
    HPX_SUPERVISION_DISPATCH_EXPORT void init(hpx::launch::sync_policy,
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

}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
