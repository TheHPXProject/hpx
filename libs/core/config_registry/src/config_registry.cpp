//  Copyright (c) 2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/config_registry.hpp>

#include <vector>

namespace hpx::config_registry {

    namespace detail {

        [[nodiscard]] std::vector<module_config>& get_module_configs()
        {
            static std::vector<module_config> configs;
            return configs;
        }

        [[nodiscard]] std::vector<library_version>& get_library_versions()
        {
            static std::vector<library_version> versions;
            return versions;
        }
    }    // namespace detail

    std::vector<module_config> const& get_module_configs()
    {
        return detail::get_module_configs();
    }

    void add_module_config(module_config const& config)
    {
        detail::get_module_configs().push_back(config);
    }

    add_module_config_helper::add_module_config_helper(
        module_config const& config)
    {
        add_module_config(config);
    }

    std::vector<library_version> const& get_library_versions()
    {
        return detail::get_library_versions();
    }

    void add_library_version(library_version const& version)
    {
        detail::get_library_versions().push_back(version);
    }

    add_library_version_helper::add_library_version_helper(
        library_version const& version)
    {
        add_library_version(version);
    }
}    // namespace hpx::config_registry
