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
#include <hpx/supervision_dispatch/dispatch_api.hpp>
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

    std::vector<discovered_peer> fan_out_join(registry const& local_registry,
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

        std::vector<discovered_peer> joined;
        joined.reserve(shadow_futures.size());

        for (std::size_t i = 0; i != shadow_futures.size(); ++i)
        {
            hpx::future<joined_peer>& f = shadow_futures[i];

            // peers[i] is still correctly correlated with shadow_futures[i]
            // here (this loop is the only place that index pairing is used), so
            // pair the surviving peer with its locality directly rather than
            // flattening to a bare locality id and forcing every caller to
            // re-derive the pairing (ambiguous once timed-out peers are
            // dropped).
            if (f.is_ready() && !f.has_exception())
            {
                // Surface the join_epoch join() already computed, so callers
                // can dispatch directly against {peer.locality,
                // peer.join_epoch} without a redundant follow-up join() call.
                discovered_peer joined_peer_entry = peers[i];
                joined_peer_entry.join_epoch = f.get().join_epoch;
                joined.push_back(HPX_MOVE(joined_peer_entry));
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

        return joined;
    }

    std::vector<discovered_peer> fan_out_join(supervision_handle const& handle,
        std::vector<discovered_peer> const& peers,
        hpx::chrono::steady_duration const& timeout)
    {
        return fan_out_join(handle.registry_client, peers, timeout);
    }

    std::vector<discovered_peer> discover_and_join(
        registry const& local_registry,
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
        std::vector<discovered_peer> joined =
            fan_out_join(local_registry, peers, remaining());

        // fan_out_join()'s success only means join() settled server-side; the
        // registry component's own bookkeeping (marking the entry ready /
        // non-evicting) may not yet be reflected by the very next
        // snapshot_peers() call. snapshot_peers() only ever returns fully
        // joined, non-evicting peers, so a joined locality simply won't
        // appear in the list until that settles. Poll briefly for that
        // visibility before returning, so callers (failure_detection_loop(),
        // find_locality_for()) never observe a "joined" peer the registry
        // snapshot doesn't yet reflect.
        for (;;)
        {
            hpx::error_code ec(hpx::throwmode::lightweight);
            auto const snapshot =
                local_registry.snapshot_peers(hpx::launch::sync, ec);

            bool const all_visible = !ec &&
                std::ranges::all_of(joined, [&](discovered_peer const& peer) {
                    return std::ranges::any_of(
                        snapshot, [&](server::peer_snapshot const& p) {
                            return p.peer_locality == peer.locality;
                        });
                });

            if (all_visible || std::chrono::steady_clock::now() >= deadline)
                break;

            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return joined;
    }

    std::vector<discovered_peer> discover_and_join(
        supervision_handle const& handle,
        hpx::chrono::steady_duration const& timeout)
    {
        return discover_and_join(handle.registry_client, timeout);
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
