//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision_dispatch/registry.hpp
/// \page hpx::supervision::registry
/// \headerfile hpx/supervision_dispatch.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/supervision_dispatch/sentinel.hpp>
#include <hpx/supervision_dispatch/server/registry.hpp>

#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

    ///////////////////////////////////////////////////////////////////////////
    /// A registry is a lightweight, self-supervising handle for pairing with
    /// peer sentinels. It mirrors a peer's lifecycle state locally via join(),
    /// and can itself be discovered by name in AGAS like other
    /// supervision_dispatch components.
    class HPX_SUPERVISION_DISPATCH_EXPORT registry
      : public hpx::components::client_base<registry, server::registry>
    {
        using base_type =
            hpx::components::client_base<registry, server::registry>;

    public:
        /// Constructs an empty (not-yet-connected) registry client, suitable
        /// for later binding to an existing, remotely registered registry via
        /// the constructor taking a symbol name (see discover_peers()). Unlike
        /// the constructor below (taking the locality), this does not create a
        /// new server-side component.
        registry() = default;

        /// Constructs a registry by creating a new server-side component on the
        /// given target locality.
        ///
        /// \param target_locality The locality on which the underlying registry
        ///        component will be created.
        explicit registry(hpx::id_type const& target_locality);

        /// Connect to a peer's registry component.
        ///
        /// \param symbolic_name The symbol name of the peer's registry
        ///        component.
        explicit registry(std::string const& symbolic_name);

        /* implicit */ registry(hpx::future<hpx::id_type>&& f);

        /// Join a peer sentinel: create (or reuse) a local shadow target that
        /// mirrors the peer's lifecycle state, and register this registry as an
        /// observer of the peer's lifecycle/activity events.
        ///
        /// \param peer_sentinel The peer sentinel to join.
        /// \param peer_locality The locality on which the peer sentinel
        ///        resides.
        ///
        /// \return A future that becomes ready with the local shadow identifier
        ///         and the peer dispatch target.
        hpx::future<joined_peer> join(sentinel const& peer_sentinel,
            hpx::id_type const& peer_locality) const;

        /// \copydoc join(sentinel const&, hpx::id_type const&) const
        ///
        /// \param peer_sentinel The peer sentinel to join.
        /// \param peer_locality The locality on which the peer sentinel
        ///        resides.
        /// \param ec Used to hold the error code that results from the
        ///           operation instead of throwing an exception on failure.
        ///
        /// \return The local shadow identifier and the peer dispatch target.
        joined_peer join(hpx::launch::sync_policy,
            sentinel const& peer_sentinel, hpx::id_type const& peer_locality,
            hpx::error_code& ec = hpx::throws) const;

        /// Returns a point-in-time snapshot of all fully joined, non-evicting
        /// peers held by the underlying registry component.
        ///
        /// \return A future that becomes ready with one peer_snapshot per
        ///         peer that is fully joined and not pending eviction.
        hpx::future<std::vector<server::peer_snapshot>> snapshot_peers() const;

        /// \copydoc snapshot_peers() const
        ///
        /// \param ec Used to hold the error code that results from the
        ///           operation instead of throwing an exception on failure.
        ///
        /// \return One peer_snapshot per peer that is fully joined and not
        ///         pending eviction.
        std::vector<server::peer_snapshot> snapshot_peers(
            hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const;

        /// Attaches a fire-and-forget continuation to a not-yet-ready join()
        /// future so that, once it settles, its registration is retracted via
        /// leave() rather than left to persist unreported.
        ///
        /// Used by fan_out_join() for any join() that did not complete within
        /// its wait_all_for_nothrow() timeout: the join is silently excluded
        /// from that call's returned discovered_peer list, so this ensures its
        /// eventual side effects on the registry are discarded too, keeping
        /// "dropped means dropped" true for snapshot_peers() as well.
        /// Exceptions from the underlying join() are swallowed here (nothing to
        /// retract).
        ///
        /// \param peer_sentinel The peer sentinel to leave.
        /// \param peer_locality The locality that owns \p peer_sentinel.
        /// \param f The pending join() future to watch and, on success, undo.
        void leave(sentinel const& peer_sentinel,
            hpx::id_type const& peer_locality,
            hpx::future<joined_peer>&& f) const;

        /// Register this registry's id with AGAS under a name pinned to the
        /// locality that actually hosts its underlying server component
        /// (derived from the component's own id via
        /// hpx::naming::get_locality_id_from_id(), not the ambient
        /// hpx::get_locality_id() of the caller), so that discovery of this
        /// registry is unaffected by failures on any locality other than the
        /// one it actually lives on. May only be called once per registry
        /// instance, mirroring the call-once contract of the underlying
        /// client_base::register_as().
        ///
        /// \return A future that becomes ready with `true` if the name was
        ///         registered successfully, `false` otherwise.
        hpx::future<bool> register_name();

        /// \copydoc register_name()
        ///
        /// \param ec Used to hold the error code that results from the
        ///           operation instead of throwing an exception on failure.
        /// \return `true` if the name was registered successfully, `false`
        ///         otherwise.
        bool register_name(
            hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws);

        /// Remove this registry's AGAS name registration again, using the name
        /// most recently established by register_name() (retrieved via
        /// registered_name(), the single source of truth client_base already
        /// maintains for it). Calling this explicitly is safe even though
        /// destroying this registry also unregisters its name automatically
        /// (client_base::register_as()'s default manage_lifetime=true): the
        /// redundant unregister attempt on destruction is a silent no-op.
        ///
        /// \return A future that becomes ready with the id that was registered
        ///         under the removed name.
        hpx::future<hpx::id_type> unregister_name() const;

        /// \copydoc unregister_name()
        ///
        /// \param ec Used to hold the error code that results from the
        ///           operation instead of throwing an exception on failure.
        /// \return The id that was registered under the removed name.
        hpx::id_type unregister_name(
            hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const;
    };
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
