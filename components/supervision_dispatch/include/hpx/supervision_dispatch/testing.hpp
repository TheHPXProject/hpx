//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision_dispatch/testing.hpp
/// \page hpx::supervision::testing::local_snapshot_peers
/// \page hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing
/// \page hpx::supervision::testing::last_join_locality
/// \page hpx::supervision::testing::suspend_heartbeat_for_testing
/// \page hpx::supervision::testing::failure_detection_sweep_in_flight_for_testing
/// \page hpx::supervision::testing::stop_background_loops
/// \page hpx::supervision::testing::set_evict_fenced_hook_for_testing
/// \page hpx::supervision::testing::set_lifecycle_mirror_hook_for_testing
/// \page hpx::supervision::testing::set_evict_peer_hook_for_testing
/// \page hpx::supervision::testing::evict_peers_waiting_for_mirrors_for_testing
/// \headerfile hpx/supervision_dispatch.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/supervision_dispatch/export_definitions.hpp>

#include <chrono>
#include <cstddef>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace hpx::supervision::server {

    struct peer_snapshot;
}    // namespace hpx::supervision::server

namespace hpx::supervision::testing {

    /// Returns a snapshot of this locality's currently-joined, ready peers, as
    /// tracked by the local registry. Not part of the public dispatch API -
    /// exists so tests can inspect registry state directly.
    HPX_SUPERVISION_DISPATCH_EXPORT std::vector<server::peer_snapshot>
    local_snapshot_peers();

    /// Overrides the timeout failure_detection_loop() passes to await_terminal()
    /// on each sweep. Must be called before init() starts the loop to take
    /// effect for that lifecycle; has no effect on a sweep already in flight.
    /// Not part of the public dispatch API - exists only to make
    /// failure-detection tests deterministic and fast instead of bound by the
    /// real 60s default_discovery_timeout.
    ///
    /// \param timeout  The new poll timeout.
    HPX_SUPERVISION_DISPATCH_EXPORT void
    set_failure_detection_poll_timeout_for_testing(
        hpx::chrono::steady_duration const& timeout);

    /// Returns the locality most recently returned by join(), regardless of
    /// whether the register_observers() call that followed it went on to
    /// succeed or fail. Lets tests verify that a failed join() does not leak
    /// that locality's local supervision state (see
    /// registry::register_observers()'s catch block).
    HPX_SUPERVISION_DISPATCH_EXPORT hpx::id_type last_join_locality();

    /// Stops this locality's own heartbeat_loop() without finalizing the
    /// dispatcher, simulating a hard crash (all lifecycle activity ceases,
    /// including the periodic event::running pulse) while staying joined in
    /// peers' registries. Idempotent; no-op if init() was never called or the
    /// loop is already stopped. Not part of the public dispatch API - exists
    /// only so failure-detection tests can deterministically simulate silence
    /// instead of relying on the absence of explicit calls, which no longer
    /// implies silence now that heartbeats are mirrored onto observers'
    /// local supervision state.
    HPX_SUPERVISION_DISPATCH_EXPORT void suspend_heartbeat_for_testing();

    /// Reports whether failure_detection_loop() currently has one or more
    /// await_terminal() calls outstanding against unresponsive peers (i.e. a
    /// sweep is mid-flight). Returns false before init() is called, between
    /// sweeps, and after finalize(). Not part of the public dispatch API -
    /// exists only so tests can deterministically synchronize on a real
    /// in-flight sweep instead of sleeping and hoping the loop got there first.
    HPX_SUPERVISION_DISPATCH_EXPORT bool
    failure_detection_sweep_in_flight_for_testing();

    /// Stops this locality's failure_detection_loop() and heartbeat_loop()
    /// background tasks (joining both) without otherwise finalizing the
    /// dispatcher: unlike finalize(), this never publishes event::completed,
    /// never unregisters the registry symbol name, and never resets the
    /// lifecycle state back to uninitialized, so the sentinel's last published
    /// event stays stale and peers' failure_detection_loop() still eventually
    /// times out and observes target_fenced/rejected_fenced against this
    /// locality. Lets a caller depart the HPX runtime cleanly (e.g. via
    /// hpx::disconnect()) afterwards without leaving either background
    /// hpx::async task still running - simulating a worker that dies mid-epoch
    /// without corrupting AGAS or hanging runtime shutdown, as opposed to a
    /// true crash (no call at all) or a fully graceful departure (finalize()).
    /// Idempotent; no-op if init() was never called or the loops are already
    /// stopped. Not part of the public dispatch API - exists only for
    /// examples/tests that need to simulate this exact scenario
    /// deterministically.
    HPX_SUPERVISION_DISPATCH_EXPORT void stop_background_loops();

    /// Installs \p hook to be called by registry::evict_fenced() on this
    /// locality after it found the peer's local supervision state terminal
    /// and before it removes the peer's registry entry, i.e. in the window in
    /// which a newer lifecycle transition of the peer has to be detected (see
    /// registry::evict_fenced()). Pass an empty function to remove the hook.
    /// Not part of the public dispatch API - exists only so tests can force a
    /// lifecycle transition into that window deterministically.
    ///
    /// \param hook  The function to call, with no lock held.
    HPX_SUPERVISION_DISPATCH_EXPORT void set_evict_fenced_hook_for_testing(
        hpx::function<void()> hook);

    /// Installs \p hook to be called by a registry's lifecycle observer on this
    /// locality after it announced that it is mirroring a notification
    /// (peer_entry::mirrors_in_flight) and before it mirrors it, i.e. while
    /// registry::evict_fenced() has to treat the mirroring as in flight. Pass
    /// an empty function to remove the hook. Not part of the public dispatch
    /// API - exists only so tests can hold a mirroring in flight
    /// deterministically.
    ///
    /// \param hook  The function to call, with no lock held.
    HPX_SUPERVISION_DISPATCH_EXPORT void set_lifecycle_mirror_hook_for_testing(
        hpx::function<void()> hook);

    /// Installs \p hook to be called by registry::evict_peer() on this
    /// locality before it looks at the peer's registry entry, e.g. for the
    /// eviction a terminal lifecycle notification schedules. Pass an empty
    /// function to remove the hook. Not part of the public dispatch API -
    /// exists only so tests can hold such an eviction back deterministically.
    ///
    /// \param hook  The function to call, with no lock held.
    HPX_SUPERVISION_DISPATCH_EXPORT void set_evict_peer_hook_for_testing(
        hpx::function<void()> hook);

    /// Returns the number of registry::evict_peer() calls on this locality
    /// currently waiting for the lifecycle notifications being mirrored for
    /// the peer's entry to finish. Not part of the public dispatch API -
    /// exists only so tests can tell that an eviction has reached that wait.
    HPX_SUPERVISION_DISPATCH_EXPORT std::size_t
    evict_peers_waiting_for_mirrors_for_testing();
}    // namespace hpx::supervision::testing
