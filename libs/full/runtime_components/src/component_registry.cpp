//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2017      Thomas Heller
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/components_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/string_util.hpp>

#include <hpx/runtime_components/component_registry.hpp>

#include <algorithm>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::detail {

    void get_component_info(std::vector<std::string>& fillini,
        std::string const& filepath, bool is_static, char const* name,
        char const* component_string, factory_state state, char const* more)
    {
        fillini.emplace_back(std::string("[hpx.components.") + name + "]");
        fillini.emplace_back(std::string("name = ") + component_string);

        if (!is_static)
        {
            if (filepath.empty())
            {
                fillini.emplace_back(std::string("path = ") +
                    util::find_prefixes("/hpx", component_string));
            }
            else
            {
                fillini.emplace_back(std::string("path = ") + filepath);
            }
        }

        switch (state)
        {
        case hpx::components::factory_state::enabled:
            fillini.emplace_back("enabled = 1");
            break;
        case hpx::components::factory_state::disabled:
            fillini.emplace_back("enabled = 0");
            break;
        case hpx::components::factory_state::check:
            fillini.emplace_back("enabled = $[hpx.components.load_external]");
            break;
        }

        if (is_static)
        {
            fillini.emplace_back("static = 1");
        }

        if (more)
        {
            std::vector<std::string> data;
            hpx::string_util::split(
                data, more, hpx::string_util::is_any_of("\n"));
            std::copy(data.begin(), data.end(), std::back_inserter(fillini));
        }
    }

    bool is_component_enabled(char const* name)
    {
        hpx::util::runtime_configuration const& config = hpx::get_config();
        std::string enabled_entry = config.get_entry(
            std::string("hpx.components.") + name + ".enabled", "0");

        std::transform(enabled_entry.begin(), enabled_entry.end(),
            enabled_entry.begin(), [](char c) { return std::tolower(c); });

        if (enabled_entry == "no" || enabled_entry == "false" ||
            enabled_entry == "0")
        {
            LRT_(info).format("plugin factory disabled: {}", name);
            return false;    // this component has been disabled
        }
        return true;
    }
}    // namespace hpx::components::detail
