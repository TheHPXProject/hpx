//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2013-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::plugins {

    HPX_CXX_EXPORT template <typename PluginType>
    struct unique_plugin_name
    {
        static_assert(sizeof(PluginType) == 0, "plugin name is not defined");
    };
}    // namespace hpx::plugins
