//  Copyright (c) 2005-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/runtime_configuration/component_registry_base.hpp>
#include <hpx/runtime_configuration/plugin_registry_base.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT bool handle_ini_file(
        section& ini, std::string const& loc);
    HPX_CXX_CORE_EXPORT bool handle_ini_file_env(
        section& ini, char const* env_var, char const* file_suffix = nullptr);

    ///////////////////////////////////////////////////////////////////////////
    // read system and user specified ini files
    //
    // returns true if at least one alternative location has been read
    // successfully
    HPX_CXX_CORE_EXPORT bool init_ini_data_base(
        section& ini, std::string& hpx_ini_file);

    ///////////////////////////////////////////////////////////////////////////
    // load registry information for all statically registered modules
    HPX_CXX_CORE_EXPORT
    std::vector<std::shared_ptr<components::component_registry_base>>
    load_component_factory_static(util::section& ini, std::string name,
        hpx::util::plugin::get_plugins_list_type get_factory,
        error_code& ec = throws);

    ///////////////////////////////////////////////////////////////////////////
    // load plugin registry information for all statically registered plugin
    // modules. Injects `static = 1` into each generated [hpx.plugins.*]
    // section so load_plugins routes to the static loader at runtime.
    HPX_CXX_CORE_EXPORT
    std::vector<std::shared_ptr<plugins::plugin_registry_base>>
    load_plugin_factory_static(util::section& ini, std::string const& name,
        hpx::util::plugin::get_plugins_list_type get_factory,
        error_code& ec = throws);

    ///////////////////////////////////////////////////////////////////////////
    // global function to read component ini information
    HPX_CXX_CORE_EXPORT void merge_component_inis(section& ini);

    ///////////////////////////////////////////////////////////////////////////
    // iterate over all shared libraries in the given directory and construct
    // default ini settings assuming all of those are components
    HPX_CXX_CORE_EXPORT
    std::vector<std::shared_ptr<plugins::plugin_registry_base>>
    init_ini_data_default(std::string const& libs, section& ini,
        std::map<std::string, filesystem::path>& basenames,
        std::map<std::string, hpx::util::plugin::dll>& modules,
        std::vector<std::shared_ptr<components::component_registry_base>>&
            component_registries);
}    // namespace hpx::util
