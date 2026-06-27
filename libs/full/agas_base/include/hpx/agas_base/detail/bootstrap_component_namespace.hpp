//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2012-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/agas_base/component_namespace.hpp>
#include <hpx/agas_base/server/component_namespace.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace hpx::agas::detail {

    struct bootstrap_component_namespace : component_namespace
    {
        using iterate_types_function_type = hpx::distributed::function<void(
            std::string const&, components::component_type)>;

        naming::address::address_type ptr() const
        {
            return const_cast<server::component_namespace*>(&server_);
        }
        naming::address addr() const;
        hpx::id_type gid() const;

        components::component_type bind_prefix(
            std::string const& key, std::uint32_t prefix);

        components::component_type bind_name(std::string const& name);

        std::vector<std::uint32_t> resolve_id(components::component_type key);

        bool unbind(std::string const& key);

        void iterate_types(iterate_types_function_type const& f);

        std::string get_component_type_name(components::component_type type);

        hpx::future<std::uint32_t> get_num_localities(
            components::component_type type);

        void register_server_instance(std::uint32_t locality_id);

        void unregister_server_instance(error_code& ec);

        server::component_namespace* get_service()
        {
            return &server_;
        }

    private:
        server::component_namespace server_;
    };
}    // namespace hpx::agas::detail
