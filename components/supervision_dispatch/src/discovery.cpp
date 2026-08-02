//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>

#include <hpx/supervision_dispatch/discovery.hpp>
#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/sentinel.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

    std::vector<hpx::supervision::discovered_peer> discover_peers(
        hpx::chrono::steady_duration const& timeout)
    {
        std::vector<hpx::id_type> const remote_localities =
            hpx::find_remote_localities();

        std::vector<sentinel> sentinel_clients;
        std::vector<registry> registry_clients;

        sentinel_clients.reserve(remote_localities.size());
        registry_clients.reserve(remote_localities.size());

        // Fire off all sentinel/registry lookups concurrently, up front, rather
        // than resolving them one locality at a time.
        for (hpx::id_type const& remote_locality : remote_localities)
        {
            std::uint32_t const locality_id =
                hpx::naming::get_locality_id_from_id(remote_locality);
            std::string const prefix =
                "/" + std::to_string(locality_id) + "/supervision_dispatch/";

            sentinel_clients.emplace_back(prefix + "sentinel");
            registry_clients.emplace_back(prefix + "registry");
        }

        // Bound the whole pass with a single suspend/wake across every
        // candidate, rather than a per-candidate wait_for (which would cost up
        // to remote_localities.size() * timeout in the worst case). Inputs are
        // left untouched by wait_all_for, so they remain individually queryable
        // via is_ready() below regardless of the returned status.
        hpx::wait_all_for_nothrow(timeout, sentinel_clients, registry_clients);

        std::vector<hpx::supervision::discovered_peer> peers;
        peers.reserve(remote_localities.size());

        for (std::size_t i = 0; i != remote_localities.size(); ++i)
        {
            // Only include peers whose sentinel *and* registry names both
            // resolved within the timeout; a locality that never called
            // register_name() (e.g. because it has not run init_supervision()
            // yet) is simply not participating in supervision, not an error.
            hpx::supervision::sentinel& s = sentinel_clients[i];
            hpx::supervision::registry& r = registry_clients[i];

            bool const sentinel_resolved = s.is_ready() && !s.has_exception();
            bool const registry_resolved = r.is_ready() && !r.has_exception();

            if (sentinel_resolved && registry_resolved)
            {
                peers.push_back(hpx::supervision::discovered_peer{
                    .locality = remote_localities[i],
                    .sentinel_client = HPX_MOVE(s),
                    .registry_client = HPX_MOVE(r)});
            }
        }

        return peers;
    }

    std::vector<hpx::id_type> fan_out_join(registry const& local_registry,
        std::vector<hpx::supervision::discovered_peer> const& peers)
    {
        // Fire off all join() calls concurrently, up front, rather than joining
        // one peer at a time; each individually reuses registry::join()'s
        // existing reservation/idempotency handling, so there is nothing
        // further to coordinate between them here.
        std::vector<hpx::future<hpx::id_type>> shadow_futures;
        shadow_futures.reserve(peers.size());

        for (hpx::supervision::discovered_peer const& peer : peers)
        {
            shadow_futures.push_back(
                local_registry.join(peer.sentinel_client, peer.locality));
        }

        return hpx::unwrap(HPX_MOVE(shadow_futures));
    }

    std::vector<hpx::id_type> discover_and_join(registry const& local_registry,
        hpx::chrono::steady_duration const& timeout)
    {
        std::vector<discovered_peer> const peers = discover_peers(timeout);
        return fan_out_join(local_registry, peers);
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
