//  Copyright (c) 2005-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/runtime_configuration/static_factory_data.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace hpx::components {

    bool& get_initial_static_loading() noexcept
    {
        static bool initial_static_loading = true;
        return initial_static_loading;
    }

    ///////////////////////////////////////////////////////////////////////////
    // There is no need to protect these global from thread concurrent access
    // as they are access during early startup only.
    std::vector<static_factory_load_data_type>& get_static_module_data()
    {
        static std::vector<static_factory_load_data_type>
            global_module_init_data;
        return global_module_init_data;
    }

    void init_registry_module(
        static_factory_load_data_type const& data)    //-V835
    {
        if (get_initial_static_loading())
        {
            get_static_module_data().push_back(data);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    static std::map<std::string, util::plugin::get_plugins_list_type>&
    get_static_factory_data()
    {
        static std::map<std::string, util::plugin::get_plugins_list_type>
            global_factory_init_data;
        return global_factory_init_data;
    }

    void init_registry_factory(
        static_factory_load_data_type const& data)    //-V835
    {
        if (get_initial_static_loading())
        {
            get_static_factory_data().emplace(data.name, data.get_factory);
        }
    }

    bool get_static_factory(
        std::string const& instance, util::plugin::get_plugins_list_type& f)
    {
        using map_type =
            std::map<std::string, util::plugin::get_plugins_list_type>;

        map_type const& m = get_static_factory_data();
        auto const it = m.find(instance);
        if (it == m.end())
            return false;

        f = it->second;
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    static std::map<std::string, util::plugin::get_plugins_list_type>&
    get_static_commandline_data()
    {
        static std::map<std::string, util::plugin::get_plugins_list_type>
            global_commandline_init_data;
        return global_commandline_init_data;
    }

    void init_registry_commandline(
        static_factory_load_data_type const& data)    //-V835
    {
        if (get_initial_static_loading())
        {
            get_static_commandline_data().emplace(data.name, data.get_factory);
        }
    }

    bool get_static_commandline(
        std::string const& instance, util::plugin::get_plugins_list_type& f)
    {
        using map_type =
            std::map<std::string, util::plugin::get_plugins_list_type>;

        map_type const& m = get_static_commandline_data();
        auto const it = m.find(instance);
        if (it == m.end())
            return false;

        f = it->second;
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    static std::map<std::string, util::plugin::get_plugins_list_type>&
    get_static_startup_shutdown_data()
    {
        static std::map<std::string, util::plugin::get_plugins_list_type>
            global_startup_shutdown_init_data;
        return global_startup_shutdown_init_data;
    }

    void init_registry_startup_shutdown(
        static_factory_load_data_type const& data)    //-V835
    {
        if (get_initial_static_loading())
        {
            get_static_startup_shutdown_data().emplace(
                data.name, data.get_factory);
        }
    }

    bool get_static_startup_shutdown(
        std::string const& instance, util::plugin::get_plugins_list_type& f)
    {
        using map_type =
            std::map<std::string, util::plugin::get_plugins_list_type>;

        map_type const& m = get_static_startup_shutdown_data();
        auto const it = m.find(instance);
        if (it == m.end())
            return false;

        f = it->second;
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::vector<static_factory_load_data_type>& get_static_plugin_module_data()
    {
        static std::vector<static_factory_load_data_type>
            global_plugin_module_init_data;
        return global_plugin_module_init_data;
    }

    void init_registry_plugin_module(
        static_factory_load_data_type const& data)    //-V835
    {
        if (get_initial_static_loading())
        {
            get_static_plugin_module_data().push_back(data);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    static std::map<std::string, util::plugin::get_plugins_list_type>&
    get_static_plugin_factory_data()
    {
        static std::map<std::string, util::plugin::get_plugins_list_type>
            global_plugin_factory_init_data;
        return global_plugin_factory_init_data;
    }

    void init_registry_plugin_factory(
        static_factory_load_data_type const& data)    //-V835
    {
        if (get_initial_static_loading())
        {
            get_static_plugin_factory_data().emplace(
                data.name, data.get_factory);
        }
    }

    bool get_static_plugin_factory(
        std::string const& instance, util::plugin::get_plugins_list_type& f)
    {
        using map_type =
            std::map<std::string, util::plugin::get_plugins_list_type>;

        map_type const& m = get_static_plugin_factory_data();
        auto const it = m.find(instance);
        if (it == m.end())
            return false;

        f = it->second;
        return true;
    }
}    // namespace hpx::components
