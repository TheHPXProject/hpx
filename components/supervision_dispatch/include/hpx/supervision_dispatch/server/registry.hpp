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

#include <map>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::supervision::server {

    namespace detail {

        // Testing infrastructure support: returns the shadow target most
        // recently minted by join(), regardless of whether the
        // register_observers() call that followed it went on to succeed or
        // fail. Lets tests verify that a failed join() does not leak the
        // shadow's locally tracked supervision state (see
        // registry::register_observers()'s catch block).
        HPX_SUPERVISION_DISPATCH_EXPORT hpx::id_type last_join_shadow();
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    class HPX_SUPERVISION_DISPATCH_EXPORT registry
      : public hpx::components::component_base<registry>
    {
    public:
        registry();

        /// Joins a peer sentinel and creates or reuses its shadow.
        ///
        /// \param peer_sentinel The peer sentinel to observe.
        /// \param peer_locality The locality that owns \p peer_sentinel.
        hpx::id_type join(hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality);

        struct join_action
          : hpx::actions::make_action_t<decltype(&registry::join),
                &registry::join, join_action>
        {
        };

    protected:
        std::pair<hpx::id_type, bool> reserve_ownership(
            hpx::id_type const& peer_sentinel);
        std::pair<hpx::id_type, hpx::id_type> register_observers(
            hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality, hpx::id_type const& shadow);

        // Evicts a peer that has reached a terminal lifecycle event: erases its
        // entry from peers_, unregisters its lifecycle_observer and
        // activity_observer on peer_locality, and removes the shadow's local
        // state. Called via hpx::post() from the (lock-free) lifecycle observer
        // callback installed in register_observers(), so that the mtx_
        // acquisition needed to safely mutate peers_ happens on a separate task
        // rather than inline in that callback (see the comment on
        // register_observers() for why the callback itself must not take mtx_).
        // `shadow` identifies which entry to evict: if peer_sentinel has since
        // been re-joined (fresh shadow) or already evicted by a racing terminal
        // notification, this is a no-op.
        void evict_peer(hpx::id_type const& peer_sentinel,
            hpx::id_type const& peer_locality, hpx::id_type const& shadow);

        /// Unregisters a peer's observers and removes its shadow state.
        ///
        /// Performs the actual peer teardown deferred by evict_peer():
        /// unregisters \p lifecycle_observer and \p activity_observer on
        /// \p peer_locality and removes the shadow's locally tracked
        /// supervision state. Kept separate from evict_peer() because
        /// evict_peer() only posts this work as a task (via hpx::post())
        /// rather than performing it inline.
        ///
        /// \param peer_locality The locality that owns the observers.
        /// \param lifecycle_observer The lifecycle observer id to unregister.
        /// \param activity_observer The activity observer id to unregister.
        /// \param shadow The peer's shadow whose local state is removed.
        static void cleanup_peer(hpx::id_type const& peer_locality,
            hpx::id_type const& lifecycle_observer,
            hpx::id_type const& activity_observer, hpx::id_type const& shadow);

    private:
        struct peer_entry
        {
            hpx::id_type shadow;
            hpx::id_type lifecycle_observer;
            hpx::id_type activity_observer;

            // False while a join() call has reserved ownership of this peer
            // sentinel (i.e. is in the process of performing the remote
            // observer registrations) but has not yet filled in the rest of
            // the entry. Concurrent join() calls for the same peer wait on
            // cond_ instead of racing to register duplicate observers.
            bool ready = false;

            // Ensure that a terminal notification racing ahead of join()
            // completion can be deferred instead of dropped.
            bool evict_pending = false;
        };

        hpx::spinlock mtx_;
        hpx::lcos::local::detail::condition_variable cond_;

        std::map<hpx::id_type, peer_entry> peers_;
    };
}    // namespace hpx::supervision::server

HPX_REGISTER_ACTION_DECLARATION(hpx::supervision::server::registry::join_action,
    supervision_dispatch_registry_join_action)

#include <hpx/config/warnings_suffix.hpp>
