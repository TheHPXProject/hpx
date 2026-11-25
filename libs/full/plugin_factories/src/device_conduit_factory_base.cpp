//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/plugin_factories/device_conduit_factory_base.hpp>

#include <algorithm>
#include <vector>

namespace hpx::plugins {

    std::vector<device_conduit_factory_base*>& get_device_conduit_factories()
    {
        static std::vector<device_conduit_factory_base*> factories;
        return factories;
    }

    void add_device_conduit_factory(device_conduit_factory_base* factory)
    {
        auto& factories = get_device_conduit_factories();
        if (std::find(factories.begin(), factories.end(), factory) ==
            factories.end())
        {
            factories.push_back(factory);
        }
    }
}    // namespace hpx::plugins

#endif
