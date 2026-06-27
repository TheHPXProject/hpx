//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/component_type.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename ServerComponent>
    struct stub_base
    {
        using server_component_type = ServerComponent;

        ///////////////////////////////////////////////////////////////////////
        // expose component type of the corresponding server component
        static components::component_type get_component_type() noexcept
        {
            return components::get_component_type<ServerComponent>();
        }
    };
}    // namespace hpx::components
