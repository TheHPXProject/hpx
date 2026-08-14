////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>

#include <cstdint>

namespace hpx { namespace test { namespace server {

    struct HPX_COMPONENT_EXPORT simple_mobile_object
      : components::component_base<simple_mobile_object>
    {
    public:
        simple_mobile_object() {}

        std::uint64_t get_lva()
        {
            return reinterpret_cast<std::uint64_t>(this);
        }

        HPX_DEFINE_COMPONENT_ACTION(
            simple_mobile_object, get_lva, get_lva_action)

        // the tests using this object rely on rebind-able gids
        naming::gid_type get_base_gid(
            naming::gid_type const& assign_gid = naming::invalid_gid) const
        {
            return get_base_gid_dynamic(assign_gid, get_current_address());
        }
    };

}}}    // namespace hpx::test::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::test::server::simple_mobile_object::get_lva_action,
    simple_mobile_object_get_lva_action)

#endif
