//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/component_type.hpp>
#include <hpx/modules/naming_base.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_type_is_compatible
    {
        static bool call(naming::address const& addr) noexcept
        {
            return components::types_are_compatible(
                addr.type_, components::get_component_type<Component>());
        }
    };
}    // namespace hpx::traits
