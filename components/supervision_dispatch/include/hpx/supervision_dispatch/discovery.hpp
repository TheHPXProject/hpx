//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision_dispatch/discovery.hpp
/// \page hpx::supervision::discover_peers, hpx::supervision::fan_out_join, hpx::supervision::discover_and_join
/// \headerfile hpx/supervision_dispatch.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/supervision_dispatch/export_definitions.hpp>
#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/sentinel.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

    // Forward declaration only: supervision_handle is defined in
    // dispatch_api.hpp, which itself includes this header (for discovered_peer/
    // sentinel/registry) - including it back here would create a cycle. The
    // supervision_handle-taking overloads below only ever need a reference to
    // it, so a forward declaration is sufficient here; discovery.cpp includes
    // dispatch_api.hpp for the full definition.
    struct supervision_handle;

    // Default bound for discover_peers(): matches the supervision module's own
    // default await_terminal() timeout (see default_await_terminal_timeout_ms
    // in supervision_manager_server.cpp) - long enough to absorb AGAS/
    // peer-startup jitter, short enough to keep a one-time discovery pass cheap
    // and bounded.
    inline constexpr std::chrono::milliseconds default_discovery_timeout{60000};

    // Sentinel discovered_peer::join_epoch value meaning "never joined" (see
    // below). Deliberately distinct from any real join epoch: a successful
    // registry::join() can legitimately return an epoch of 0 for a peer
    // whose epoch counter is still at its initial value (see
    // registry_server.cpp's seed_epoch), so 0 cannot double as "unjoined"
    // without misclassifying that peer as invalid.
    inline constexpr std::uint64_t unjoined_epoch =
        (std::numeric_limits<std::uint64_t>::max)();

    // A peer whose supervision_dispatch sentinel and registry names (see
    // sentinel::register_name()/registry::register_name()) were both
    // successfully resolved by discover_peers().
    struct discovered_peer
    {
        hpx::id_type locality;
        sentinel sentinel_client;
        registry registry_client;

        // The epoch this peer was joined at, as returned by the
        // local_registry.join(sentinel_client, locality) call that
        // fan_out_join()/discover_and_join() already perform internally. Left
        // at its default (unjoined_epoch) for peers returned by
        // discover_peers() alone, which never calls join(). Together with
        // locality - which is already the same value as joined_peer::target -
        // this carries everything dispatch_work() needs to fence a call against
        // this peer, without requiring a separate join() call at the use site.
        std::uint64_t join_epoch = unjoined_epoch;
    };

    /// Performs a one-time discovery pull for supervision_dispatch peers:
    /// concurrently resolves the pinned sentinel and registry names (see
    /// register_name()) of every remote locality
    /// (hpx::find_remote_localities(), which already excludes this locality),
    /// bounded by a single wait for \a timeout across all candidates.
    ///
    /// Because constructing a sentinel/registry client from a symbolic name
    /// (see client_base's symbolic-name constructor) waits/resolves once the
    /// symbol is bound rather than failing fast on one that is not (yet)
    /// registered, localities that never called register_name() (e.g. because
    /// they have not run init_supervision() yet) would otherwise hang this call
    /// indefinitely. Instead, once \a timeout elapses, only the candidates
    /// whose sentinel *and* registry clients had both resolved by then (checked
    /// via the non-blocking is_ready()) are returned; every other candidate is
    /// simply excluded as not currently participating in supervision -
    /// discover_peers() itself never fails or throws on their account.
    ///
    /// \param timeout  The maximum duration to wait, across all candidates
    ///                 combined, for their sentinel and registry names to
    ///                 resolve.
    ///
    /// \return         The peers whose sentinel and registry clients both
    ///                 resolved within \a timeout, paired with the numeric
    ///                 locality id they are pinned to.
    HPX_SUPERVISION_DISPATCH_EXPORT
    std::vector<discovered_peer> discover_peers(
        chrono::steady_duration const& timeout = default_discovery_timeout);

    /// Reactively fans out join() calls from \a local_registry to every peer in
    /// \a peers (typically the result of a prior discover_peers() call):
    /// for each entry, this calls `local_registry.join(peer.sentinel_client,
    /// peer.locality)`.
    ///
    /// This reuses registry::join()'s existing reservation/idempotency
    /// machinery unchanged (see server::registry::reserve_ownership()) rather
    /// than duplicating any of it, so calling fan_out_join() more than once
    /// with (partially) overlapping peer lists - or racing with an inbound
    /// join() call from one of those same peers, e.g. because that peer is
    /// concurrently running its own fan_out_join() against this locality - is
    /// safe and never creates more than one shadow per peer sentinel.
    ///
    /// fan_out_join() itself is reactive only: it performs exactly one round of
    /// join() calls for the given \a peers and starts no background/polling
    /// activity of its own.
    ///
    /// \param local_registry The registry to join every peer in \a peers to.
    /// \param peers          The peers to join, typically returned by a prior
    ///                       discover_peers() call.
    /// \param timeout  The maximum duration to wait, across all peers
    ///                 combined, for their join() calls to settle. Peers whose
    ///                 join() has not settled (or failed) within this bound are
    ///                 silently dropped from the result, rather than hanging or
    ///                 throwing.
    ///
    /// \return The peer/shadow pairs for peers whose join() calls settled
    ///         successfully within \a timeout, in the same relative order as
    ///         \a peers (with unsuccessful/timed-out peers omitted, rather
    ///         than left as gaps). Each returned discovered_peer::join_epoch
    ///         is populated from the corresponding join() call, so callers
    ///         can dispatch against `{peer.locality, peer.join_epoch}`
    ///         directly, without needing a separate join() call to obtain it.
    HPX_SUPERVISION_DISPATCH_EXPORT std::vector<discovered_peer> fan_out_join(
        registry const& local_registry,
        std::vector<discovered_peer> const& peers,
        chrono::steady_duration const& timeout = default_discovery_timeout);

    /// Convenience overload of fan_out_join() that joins every peer in
    /// \a peers to \a handle.registry_client, i.e. the registry owned by a
    /// prior init() call, rather than requiring the caller to name that
    /// registry explicitly. Forwards directly to the registry-taking overload.
    ///
    /// \param handle The supervision_handle (typically returned by init())
    ///               whose registry_client every peer in \a peers is joined to.
    /// \param peers          The peers to join, typically returned by a prior
    ///                       discover_peers() call.
    /// \param timeout  The maximum duration to wait, across all peers
    ///                 combined, for their join() calls to settle.
    ///
    /// \return The peer/shadow pairs for peers whose join() calls settled
    ///         successfully within \a timeout, as described for the
    ///         registry-taking overload above.
    HPX_SUPERVISION_DISPATCH_EXPORT std::vector<discovered_peer> fan_out_join(
        supervision_handle const& handle,
        std::vector<discovered_peer> const& peers,
        chrono::steady_duration const& timeout = default_discovery_timeout);

    /// Composes a single reactive discovery-and-join pass for
    /// supervision_dispatch peers: performs exactly one discover_peers() pull
    /// to resolve the pinned sentinel and registry names of every remote
    /// locality (hpx::find_remote_localities() already excludes this locality,
    /// so no separate self-exclusion is needed here), then fans the resulting
    /// peer list out to \a local_registry via a single fan_out_join() pass.
    ///
    /// discover_and_join() introduces no state, polling thread, timer, or
    /// repeated broadcast of its own - it is a pure composition of
    /// discover_peers() and fan_out_join(), each of which is itself bounded to
    /// a single round of resolution/join calls. Because fan_out_join() reuses
    /// registry::join()'s existing reservation/idempotency machinery unchanged
    /// (see server::registry::reserve_ownership()), calling discover_and_join()
    /// more than once - whether to pick up newly started peers or purely by
    /// accident - is safe and never creates more than one shadow per peer
    /// sentinel; repeated invocations simply return the (stable) shadow ids
    /// already owned for peers that were previously joined.
    ///
    /// \param local_registry The registry to join every discovered peer to.
    /// \param timeout        The maximum duration to wait, across all discovery
    ///                       candidates combined, for their sentinel and
    ///                       registry names to resolve (see discover_peers()).
    ///
    /// \return The peer/shadow pair that \a local_registry created (or
    ///         already had) for each peer discovered and successfully joined
    ///         within \a timeout, in the same relative order as returned by
    ///         discover_peers().
    HPX_SUPERVISION_DISPATCH_EXPORT std::vector<discovered_peer>
    discover_and_join(registry const& local_registry,
        chrono::steady_duration const& timeout = default_discovery_timeout);

    /// Convenience overload of discover_and_join() that discovers and joins
    /// peers to \a handle.registry_client, i.e. the registry owned by a prior
    /// init() call, rather than requiring the caller to name that registry
    /// explicitly. Forwards directly to the registry-taking overload.
    ///
    /// \param handle  The supervision_handle (typically returned by init())
    ///                whose registry_client every discovered peer is joined to.
    /// \param timeout The maximum duration to wait, across all discovery
    ///                candidates combined, for their sentinel and registry
    ///                names to resolve (see discover_peers()).
    ///
    /// \return The peer/shadow pair for each peer discovered and successfully
    ///         joined within \a timeout, as described for the registry-taking
    ///         overload above.
    HPX_SUPERVISION_DISPATCH_EXPORT std::vector<discovered_peer>
    discover_and_join(supervision_handle const& handle,
        chrono::steady_duration const& timeout = default_discovery_timeout);

}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
