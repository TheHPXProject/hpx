//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \headerfile hpx/supervision_dispatch/testing.hpp
/// \page hpx::supervision::testing::local_snapshot_peers, hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing, hpx::supervision::testing::last_join_shadow, hpx::supervision::testing::suspend_heartbeat_for_testing, hpx::supervision::testing::failure_detection_sweep_in_flight_for_testing
/// \headerfile hpx/supervision_dispatch.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/supervision_dispatch/export_definitions.hpp>
#include <hpx/supervision_dispatch/shadow_id.hpp>

#include <chrono>
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

    /// Returns the shadow target most recently minted by join(), regardless of
    /// whether the register_observers() call that followed it went on to succeed
    /// or fail. Lets tests verify that a failed join() does not leak the
    /// shadow's locally tracked supervision state (see
    /// registry::register_observers()'s catch block).
    HPX_SUPERVISION_DISPATCH_EXPORT shadow_id last_join_shadow();

    /// Stops this locality's own heartbeat_loop() without finalizing the
    /// dispatcher, simulating a hard crash (all lifecycle activity ceases,
    /// including the periodic event::running pulse) while staying joined in
    /// peers' registries. Idempotent; no-op if init() was never called or the
    /// loop is already stopped. Not part of the public dispatch API - exists
    /// only so failure-detection tests can deterministically simulate silence
    /// instead of relying on the absence of explicit calls, which no longer
    /// implies silence now that heartbeats are mirrored onto observers'
    /// shadows.
    HPX_SUPERVISION_DISPATCH_EXPORT void suspend_heartbeat_for_testing();

    /// Reports whether failure_detection_loop() currently has one or more
    /// await_terminal() calls outstanding against unresponsive peers (i.e. a
    /// sweep is mid-flight). Returns false before init() is called, between
    /// sweeps, and after finalize(). Not part of the public dispatch API -
    /// exists only so tests can deterministically synchronize on a real
    /// in-flight sweep instead of sleeping and hoping the loop got there first.
    HPX_SUPERVISION_DISPATCH_EXPORT bool
    failure_detection_sweep_in_flight_for_testing();

}    // namespace hpx::supervision::testing
