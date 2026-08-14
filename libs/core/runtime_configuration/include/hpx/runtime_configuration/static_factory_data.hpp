//  Copyright (c) 2005-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <map>
#include <string>
#include <vector>

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct static_factory_load_data_type
    {
        char const* name;    // component name
        hpx::util::plugin::get_plugins_list_type get_factory;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool&
    get_initial_static_loading() noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT
        std::vector<static_factory_load_data_type>&
        get_static_module_data();
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_registry_module(
        static_factory_load_data_type const&);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool get_static_factory(
        std::string const& instance, util::plugin::get_plugins_list_type& f);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_registry_factory(
        static_factory_load_data_type const&);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool get_static_commandline(
        std::string const& instance, util::plugin::get_plugins_list_type& f);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_registry_commandline(
        static_factory_load_data_type const&);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool get_static_startup_shutdown(
        std::string const& instance, util::plugin::get_plugins_list_type& f);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_registry_startup_shutdown(
        static_factory_load_data_type const&);

    ///////////////////////////////////////////////////////////////////////////
    // Static plugin registries. Mirrors the component side above but keeps
    // plugin bookkeeping in its own maps so load_plugins_static can iterate
    // only plugin modules (component loaders instantiate
    // static_plugin_factory<component_registry_base>, which would cast
    // incorrectly if plugin entries lived in the component maps).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT
        std::vector<static_factory_load_data_type>&
        get_static_plugin_module_data();
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_registry_plugin_module(
        static_factory_load_data_type const&);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool get_static_plugin_factory(
        std::string const& instance, util::plugin::get_plugins_list_type& f);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_registry_plugin_factory(
        static_factory_load_data_type const&);
}    // namespace hpx::components
