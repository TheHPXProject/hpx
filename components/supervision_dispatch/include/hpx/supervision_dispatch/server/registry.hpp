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
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/tracing.hpp>

#include <hpx/supervision_dispatch/export_definitions.hpp>
#include <hpx/supervision_dispatch/testing.hpp>

#include <cstdint>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::supervision {

    /// \brief Pairs the two values a caller obtains from a successful
    ///        \c registry::join() and needs to perform a fenced dispatch.
    ///
    /// \see dispatch_work()
    /// \see server::registry::join()
    struct joined_peer
    {
        /// \brief The real, colocatable destination for the wrapped action.
        ///
        /// The peer's locality id (i.e. \c peer_locality, the value
        /// originally passed into \c registry::join()). Forwarded to
        /// \c hpx::colocated() and used as the destination of
        /// \c hpx::sync(act, target, ts...) inside \c dispatch_work().
        hpx::id_type target;

        /// \brief The epoch at which this peer was joined.
        ///
        /// Recorded by \c registry::join() and used to identify which
        /// registry entry a later \c registry::leave() call refers to - so
        /// that a racing re-join of the same peer sentinel (which mints a
        /// fresh \c join_epoch of its own) cannot be mistaken for the join
        /// this \c joined_peer was returned from.
        std::uint64_t join_epoch = 0;
    };

    inline bool operator==(
        joined_peer const& lhs, joined_peer const& rhs) noexcept
    {
        return lhs.target == rhs.target && lhs.join_epoch == rhs.join_epoch;
    }
    inline bool operator!=(
        joined_peer const& lhs, joined_peer const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    HPX_SUPERVISION_DISPATCH_EXPORT std::ostream& operator<<(
        std::ostream& strm, joined_peer const& peer);
}    // namespace hpx::supervision

namespace hpx::supervision::server {

    class HPX_SUPERVISION_DISPATCH_EXPORT registry;

    ///////////////////////////////////////////////////////////////////////////
    /// Plain-data view of a single joined peer, safe to hand out to
    /// callers outside the registry (unlike peer_entry, does not expose
    /// the ready/evict_pending bookkeeping used internally to coordinate
    /// concurrent join()/evict_peer() calls).
    struct peer_snapshot
    {
        /// The peer sentinel this entry was joined against.
        hpx::id_type peer_sentinel;

        /// The locality that owns \c peer_sentinel, as recorded by
        /// join().
        hpx::id_type peer_locality;

        /// The epoch at which \c peer_sentinel was joined.
        std::uint64_t join_epoch;
    };

    class registry : public hpx::components::component_base<registry>
    {
    public:
        registry();

        /// Joins a peer sentinel and creates or reuses its local supervision
        /// state.
        ///
        /// \param peer_sentinel The peer sentinel to observe.
        /// \param peer_locality The locality that owns \p peer_sentinel.
        joined_peer join(hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality);

        /// \cond NOINTERNAL
        struct join_action
          : hpx::actions::make_action_t<decltype(&registry::join),
                &registry::join, join_action>
        {
        };
        /// \endcond

        /// Returns a point-in-time snapshot of all fully joined, non-evicting
        /// peers.
        ///
        /// Intended for a future failure-detection poller (or other local
        /// consumers) that needs a stable, lock-safe list of peers to iterate
        /// over without reaching into \c peers_ directly. Entries mid-join
        /// (ready == false) or already scheduled for eviction (evict_pending ==
        /// true) are excluded, so a poller never observes a half-registered or
        /// torn-down peer.
        ///
        /// \return A snapshot of every peer entry with \c ready == true and
        ///         \c evict_pending == false at the time of the call.
        std::vector<peer_snapshot> snapshot_peers() const;

        /// \cond NOINTERNAL
        struct snapshot_peers_action
          : hpx::actions::make_action_t<decltype(&registry::snapshot_peers),
                &registry::snapshot_peers, snapshot_peers_action>
        {
        };
        /// \endcond

        /// Removes a previously joined peer.
        ///
        /// \param peer_sentinel The peer sentinel to leave.
        /// \param peer_locality The locality that owns \p peer_sentinel, as
        ///        recorded by join().
        /// \param join_epoch Must match the join epoch currently recorded
        ///        for \p peer_sentinel (i.e. the epoch returned by the
        ///        corresponding join()). If \p peer_sentinel has since been
        ///        re-joined (fresh epoch) or already evicted, this call is
        ///        a no-op.
        void leave(hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality, std::uint64_t join_epoch);

        /// \cond NOINTERNAL
        struct leave_action
          : hpx::actions::make_action_t<decltype(&registry::leave),
                &registry::leave, leave_action>
        {
        };
        /// \endcond

    protected:
        // Returns {stored_peer_locality, stored_join_epoch, true} if
        // peer_sentinel is already fully joined (stored_peer_locality being
        // necessarily the one passed to the current call), or {hpx::id_type(),
        // 0, false} once this call has reserved a fresh entry for it and the
        // caller is responsible for completing the join.
        std::tuple<hpx::id_type, std::uint64_t, bool> reserve_ownership(
            hpx::id_type const& peer_sentinel);

        std::pair<hpx::id_type, hpx::id_type> register_observers(
            hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality, std::uint64_t join_epoch);

        // Evicts a peer that has reached a terminal lifecycle event: erases its
        // entry from peers_, unregisters its lifecycle_observer and
        // activity_observer on peer_locality, and (unless preserve_terminal_
        // state is set) removes peer_locality's local supervision state. Called
        // via hpx::post() from the (lock-free) lifecycle observer callback
        // installed in register_observers(), so that the mtx_ acquisition
        // needed to safely mutate peers_ happens on a separate task rather than
        // inline in that callback (see the comment on register_observers() for
        // why the callback itself must not take mtx_).
        //
        // `join_epoch` identifies which entry to evict: if peer_sentinel has
        // since been re-joined (fresh join_epoch) or already evicted by a
        // racing terminal notification, this is a no-op.
        //
        // `preserve_terminal_state` is true for terminal-notification-driven
        // evictions (leaving the peer's last published event in place as a
        // tombstone, see cleanup_peer()) and false for an explicit leave().
        void evict_peer(hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality, std::uint64_t join_epoch,
            hpx::id_type keep_alive, bool preserve_terminal_state);

        /// Unregisters a peer's observers and, unless
        ///  \p preserve_terminal_state is set, removes its locally tracked
        ///     supervision state.
        ///
        /// Performs the actual peer teardown deferred by evict_peer():
        /// unregisters \p lifecycle_observer and \p activity_observer on
        /// \p peer_locality, and removes \p peer_locality's locally tracked
        /// supervision state unless \p preserve_terminal_state is true, in
        /// which case that state - which was last set to the peer's terminal
        /// event by register_observers()'s lifecycle callback - is left in
        /// place as a tombstone rather than reset to "unknown". This lets any
        /// admission check still in flight against \p peer_locality (e.g.
        /// check_admission()) keep observing the fenced/terminal outcome after
        /// eviction has run, instead of racing against it. A subsequent join()
        /// re-seeding this locality with a fresh epoch naturally overwrites the
        /// tombstone. Kept separate from evict_peer() because evict_peer() only
        /// posts this work as a task (via hpx::post()) rather than performing
        /// it inline.
        ///
        /// \param peer_locality The locality that owns the observers, and whose
        ///        local supervision state is removed.
        /// \param lifecycle_observer The lifecycle observer id to unregister.
        /// \param activity_observer The activity observer id to unregister.
        /// \param preserve_terminal_state If true, leave peer_locality's
        ///        local supervision state in place instead of removing it.
        static void cleanup_peer(hpx::id_type const& peer_locality,
            hpx::id_type const& lifecycle_observer,
            hpx::id_type const& activity_observer,
            bool preserve_terminal_state);

    private:
        /// Internal bookkeeping for a single joined (or joining) peer.
        ///
        /// Not exposed outside the registry; see peer_snapshot for the
        /// plain-data subset safe to hand to external callers.
        struct peer_entry
        {
            /// The locality that owns this peer's sentinel, recorded by
            /// join() when the entry is marked ready. Surfaced read-only via
            /// peer_snapshot::peer_locality.
            hpx::id_type peer_locality;

            /// The observer id registered on peer_locality to receive this
            /// peer's terminal lifecycle notification (drives evict_peer()
            /// via hpx::post()).
            hpx::id_type lifecycle_observer;

            /// The observer id registered on peer_locality to receive this
            /// peer's activity notifications (used to keep peer_locality's
            /// local state current between terminal events).
            hpx::id_type activity_observer;

            /// The epoch at which this peer was joined, recorded by join()
            /// and used by evict_peer() (instead of shadow) to identify
            /// and used by evict_peer() to identify which entry a deferred
            /// eviction refers to.
            std::uint64_t join_epoch;

            /// False while a join() call has reserved ownership of this peer
            /// sentinel (i.e. is in the process of performing the remote
            /// observer registrations) but has not yet filled in the rest of
            /// the entry. Concurrent join() calls for the same peer wait on
            /// cond_ instead of racing to register duplicate observers.
            bool ready = false;

            /// True once a terminal notification has been observed for this
            /// peer but eviction has not yet completed (deferred via
            /// hpx::post() to evict_peer()). Ensures a terminal notification
            /// racing ahead of join() completion is deferred instead of
            /// dropped, and lets snapshot_peers() exclude a peer that is
            /// already tearing down.
            bool evict_pending = false;

            /// The preserve_terminal_state argument evict_peer() was called
            /// with when it set evict_pending above, remembered so that
            /// join()'s deferred cleanup_peer() call (once the join in progress
            /// completes) preserves or drops peer_locality's local supervision
            /// state consistently with what the racing terminal notification
            /// requested.
            bool evict_preserve_terminal_state = false;
        };

        mutable hpx::spinlock mtx_;
        hpx::lcos::local::detail::condition_variable cond_;

        std::map<hpx::id_type, peer_entry> peers_;
    };
}    // namespace hpx::supervision::server

HPX_REGISTER_ACTION_DECLARATION(hpx::supervision::server::registry::join_action,
    supervision_dispatch_registry_join_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::registry::snapshot_peers_action,
    supervision_dispatch_registry_snapshot_peers_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::registry::leave_action,
    supervision_dispatch_registry_leave_action)

#include <hpx/config/warnings_suffix.hpp>
