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

#include <algorithm>
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

    std::vector<shadow_id> fan_out_join(registry const& local_registry,
        std::vector<discovered_peer> const& peers,
        hpx::chrono::steady_duration const& timeout)
    {
        std::vector<hpx::future<joined_peer>> shadow_futures;
        shadow_futures.reserve(peers.size());

        for (discovered_peer const& peer : peers)
        {
            shadow_futures.push_back(
                local_registry.join(peer.sentinel_client, peer.locality));
        }

        // Bound the join phase the same way discover_peers() bounds resolution:
        // a peer that resolved during discovery but tears down its sentinel_/
        // registry_ mid-join (e.g. a concurrent finalize()) must not be able to
        // hang this call indefinitely. Inputs are left untouched by
        // wait_all_for, so is_ready()/has_exception() below remain individually
        // queryable regardless of the returned status.
        hpx::wait_all_for_nothrow(timeout, shadow_futures);

        std::vector<shadow_id> joined_ids;
        joined_ids.reserve(shadow_futures.size());

        for (std::size_t i = 0; i != shadow_futures.size(); ++i)
        {
            hpx::future<joined_peer>& f = shadow_futures[i];

            // Only keep peers whose join() settled successfully within the
            // timeout; anything still pending or that failed is dropped rather
            // than propagated - mirrors discover_peers()'s own "missing peer
            // is not an error" contract.
            if (f.is_ready() && !f.has_exception())
            {
                joined_ids.push_back(f.get().shadow);
                continue;
            }

            // Timed out: once this join() eventually settles, undo its
            // registration so registry::snapshot_peers() cannot surface a peer
            // that fan_out_join() never reported as joined. evict_peer() is
            // already safe to call redundantly/late (no-op if re-joined or
            // already evicted), so this only needs to fire-and-forget.
            local_registry.leave(
                peers[i].sentinel_client, peers[i].locality, HPX_MOVE(f));
        }

        return joined_ids;
    }

    std::vector<shadow_id> discover_and_join(registry const& local_registry,
        hpx::chrono::steady_duration const& timeout)
    {
        // One shared budget for discovery, join, and visibility polling.
        auto const deadline = timeout.from_now();
        auto const remaining = [&] {
            return hpx::chrono::steady_duration(
                std::chrono::steady_clock::now() >= deadline ?
                    std::chrono::steady_clock::duration::zero() :
                    deadline - std::chrono::steady_clock::now());
        };

        std::vector<discovered_peer> const peers = discover_peers(remaining());
        std::vector<shadow_id> joined =
            fan_out_join(local_registry, peers, remaining());

        // fan_out_join()'s success only means join() settled server-side; the
        // registry component's own bookkeeping (marking the entry ready /
        // non-evicting) may not yet be reflected by the very next
        // snapshot_peers() call. snapshot_peers() only ever returns fully
        // joined, non-evicting peers, so a joined shadow_id simply won't
        // appear in the list until that settles. Poll briefly for that
        // visibility before returning, so callers (failure_detection_loop(),
        // find_shadow_for()) never observe a "joined" peer the registry
        // snapshot doesn't yet reflect.
        for (;;)
        {
            auto const snapshot =
                local_registry.snapshot_peers(hpx::launch::sync);

            bool const all_visible =
                std::ranges::all_of(joined, [&](shadow_id const& id) {
                    return std::ranges::any_of(
                        snapshot, [&](server::peer_snapshot const& p) {
                            return p.shadow == id;
                        });
                });

            if (all_visible || std::chrono::steady_clock::now() >= deadline)
                break;

            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return joined;
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
