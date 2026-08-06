//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_distributed.hpp>

#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/server/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

    registry::registry(hpx::id_type const& target_locality)
      : base_type(hpx::new_<server::registry>(target_locality))
    {
    }

    registry::registry(std::string const& symbolic_name)
      : base_type(symbolic_name)
    {
    }

    registry::registry(hpx::future<hpx::id_type>&& f)
      : base_type(HPX_MOVE(f))
    {
    }

    hpx::future<joined_peer> registry::join(
        sentinel const& peer_sentinel, hpx::id_type const& peer_locality) const
    {
        using action_type = server::registry::join_action;
        return hpx::async(action_type(), this->get_id(), peer_sentinel.get_id(),
            peer_locality);
    }

    joined_peer registry::join(hpx::launch::sync_policy,
        sentinel const& peer_sentinel, hpx::id_type const& peer_locality,
        hpx::error_code& ec) const
    {
        using action_type = server::registry::join_action;
        return hpx::async(hpx::launch::sync, action_type(), this->get_id(),
            peer_sentinel.get_id(), peer_locality)
            .get(ec);
    }

    hpx::future<std::vector<server::peer_snapshot>> registry::snapshot_peers()
        const
    {
        using action_type = server::registry::snapshot_peers_action;
        return hpx::async(action_type(), this->get_id());
    }

    std::vector<server::peer_snapshot> registry::snapshot_peers(
        hpx::launch::sync_policy, hpx::error_code& ec) const
    {
        using action_type = server::registry::snapshot_peers_action;
        return hpx::async(hpx::launch::sync, action_type(), this->get_id())
            .get(ec);
    }

    void registry::leave(sentinel const& peer_sentinel,
        hpx::id_type const& peer_locality, hpx::future<joined_peer>&& f) const
    {
        // Fire-and-forget: leave_action (via evict_peer()) is safe to invoke
        // redundantly/late, so dropping the resulting future is fine.
        f.then(hpx::launch::async,
            [id = get_id(), sentinel_id = peer_sentinel.get_id(),
                peer_locality](hpx::future<joined_peer>&& fut) {
                if (fut.has_exception())
                {
                    return;    // nothing was registered; nothing to retract
                }

                joined_peer const joined = fut.get();
                using action_type = server::registry::leave_action;
                hpx::post(action_type(), id, sentinel_id, peer_locality,
                    joined.join_epoch);
            });
    }

    hpx::future<bool> registry::register_name()
    {
        std::uint32_t const locality_id =
            hpx::naming::get_locality_id_from_id(this->get_id());
        std::string name = "/" + std::to_string(locality_id) +
            "/supervision_dispatch/registry";
        return this->register_as(HPX_MOVE(name));
    }

    bool registry::register_name(hpx::launch::sync_policy, hpx::error_code& ec)
    {
        return register_name().get(ec);
    }

    hpx::future<hpx::id_type> registry::unregister_name() const
    {
        return hpx::agas::unregister_name(this->registered_name());
    }

    hpx::id_type registry::unregister_name(
        hpx::launch::sync_policy, hpx::error_code& ec) const
    {
        return unregister_name().get(ec);
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
