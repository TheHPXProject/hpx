//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>

#include <cstddef>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    /// Create arrays of components using their default constructor
    HPX_CXX_EXPORT template <typename Component, typename... Ts>
    naming::gid_type create(Ts&&... ts);

    HPX_CXX_EXPORT template <typename Component, typename... Ts>
    naming::gid_type create_migrated(
        naming::gid_type const& gid, void** p, Ts&&... ts);

    HPX_CXX_EXPORT template <typename Component, typename... Ts>
    std::vector<naming::gid_type> bulk_create(std::size_t count, Ts&&... ts);

    HPX_CXX_EXPORT template <typename Component, typename... Ts>
    naming::gid_type construct(Ts&&... ts)
    {
        return create<Component>(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::components::server
