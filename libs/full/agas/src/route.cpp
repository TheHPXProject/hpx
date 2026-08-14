//  Copyright (c) 2011 Vinay C Amatya
//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/agas/addressing_service.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::detail {

    void update_agas_cache(hpx::naming::gid_type const& gid,
        hpx::naming::address const& addr, std::uint64_t count,
        std::uint64_t offset)
    {
        hpx::agas::update_cache_entry(gid, addr, count, offset);
    }
}    // namespace hpx::detail

HPX_PLAIN_ACTION_ID(hpx::detail::update_agas_cache, update_agas_cache_action,
    hpx::actions::update_agas_cache_action_id)

namespace hpx::agas::server {

    void route_impl(primary_namespace& server, parcelset::parcel&& p)
    {
        LPT_(debug).format("agas::server::route_impl: {}", p.parcel_id());

        naming::gid_type const& gid = p.destination();
        naming::address& addr = p.addr();
        primary_namespace::resolved_type cache_address;

        // resolve destination addresses, we should be able to resolve all of
        // them, otherwise it's an error
        {
            std::unique_lock<primary_namespace::mutex_type> l(server.mutex());

            error_code& ec = throws;

            // wait for any migration to be completed
            if (naming::detail::is_migratable(gid))
            {
                server.wait_for_migration_locked(l, gid, ec);
            }

            cache_address = server.resolve_gid_locked(l, gid, ec);

            if (ec || hpx::get<0>(cache_address) == naming::invalid_gid)
            {
                l.unlock();

                HPX_THROWS_IF(ec, hpx::error::no_success,
                    "primary_namespace::route",
                    "can't route parcel to unknown gid: {}", gid);

                return;
            }

            // retain don't store in cache flag
            if (!naming::detail::store_in_cache(gid) &&
                !naming::is_locality(gid))
            {
                naming::detail::set_dont_store_in_cache(
                    hpx::get<0>(cache_address));
            }

            gva const g = hpx::get<1>(cache_address)
                              .resolve(gid, hpx::get<0>(cache_address));

            addr.locality_ = g.prefix;
            addr.type_ = g.type;
            addr.address_ = g.lva();
        }

        hpx::id_type const source = p.source_id();

        // either send the parcel on its way or execute actions locally
        if (naming::get_locality_id_from_gid(addr.locality_) ==
            agas::get_locality_id())
        {
            // destination is local
            if (p.schedule_action())
            {
                // object was migrated, route again
                agas::route(HPX_MOVE(p),
                    &hpx::parcelset::detail::parcel_route_handler,
                    threads::thread_priority::normal);
            }
        }
        else
        {
            // destination is remote
            hpx::parcelset::put_parcel(HPX_MOVE(p));
        }

        runtime const& rt = get_runtime();
        if (rt.get_state() < hpx::state::pre_shutdown)
        {
            // asynchronously update cache on source locality update remote
            // cache if the id is not flagged otherwise
            naming::gid_type const& id = hpx::get<0>(cache_address);
            if (id && naming::detail::store_in_cache(id))
            {
                gva const& g = hpx::get<1>(cache_address);
                naming::address address(g.prefix, g.type, g.lva());

                HPX_ASSERT(naming::is_locality(source));

                hpx::post<update_agas_cache_action>(
                    source, id, address, g.count, g.offset);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    struct init_route_function
    {
        init_route_function()
        {
            server::route = &route_impl;
        }
    };

    init_route_function init;
}    // namespace hpx::agas::server

#endif
