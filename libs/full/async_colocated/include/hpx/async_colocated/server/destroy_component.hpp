//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011-2017 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT void destroy_component(
        naming::gid_type const& gid, naming::address const& addr);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    void destroy(naming::gid_type const& gid, naming::address const& addr)
    {
        // make sure this component is located here
        if (agas::get_locality_id() !=
            naming::get_locality_id_from_gid(addr.locality_))
        {
            // This component might have been migrated, find out where it is and
            // instruct that locality to delete it.
            destroy_component(gid, addr);
            return;
        }

        // make sure it's the correct component type
        components::component_type const type =
            components::get_component_type<typename Component::wrapped_type>();
        if (!types_are_compatible(type, addr.type_))
        {
            // FIXME: should the component be re-bound ?
            HPX_THROW_EXCEPTION(hpx::error::unknown_component_address,
                "destroy<Component>",
                "global id: {} is not bound to a component instance of type: "
                "{}  (it is bound to a {})",
                gid, get_component_type_name(type),
                get_component_type_name(addr.type_));
        }

        --instance_count(type);

        // delete the local instances

        // NOLINTNEXTLINE(bugprone-bitwise-pointer-cast)
        Component* c = hpx::bit_cast<Component*>(addr.address_);
        c->finalize();
        std::destroy_at(c);
        component_heap<Component>().free(c, 1);
    }

    HPX_CXX_EXPORT template <typename Component>
    void destroy(naming::gid_type const& gid)
    {
        naming::address addr;
        if (!agas::resolve_local(gid, addr))
        {
            HPX_THROW_EXCEPTION(hpx::error::unknown_component_address,
                "destroy<Component>",
                "global id: {} is not bound to any component instance", gid);
        }

        destroy<Component>(gid, addr);
    }
}    // namespace hpx::components::server
