//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/agas_base/server/component_namespace.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace hpx::agas {

    HPX_CXX_EXPORT struct component_namespace
    {
        virtual ~component_namespace() = default;

        virtual naming::address::address_type ptr() const = 0;
        virtual naming::address addr() const = 0;
        virtual hpx::id_type gid() const = 0;

        virtual components::component_type bind_prefix(
            std::string const& key, std::uint32_t prefix) = 0;

        virtual components::component_type bind_name(
            std::string const& name) = 0;

        virtual std::vector<std::uint32_t> resolve_id(
            components::component_type key) = 0;

        virtual bool unbind(std::string const& key) = 0;

        virtual void iterate_types(iterate_types_function_type const& f) = 0;

        virtual std::string get_component_type_name(
            components::component_type type) = 0;

        virtual hpx::future<std::uint32_t> get_num_localities(
            components::component_type type) = 0;

        virtual void register_server_instance(std::uint32_t /*locality_id*/) {}

        virtual void unregister_server_instance(error_code& /*ec*/) {}

        server::component_namespace* get_service()
        {
            return nullptr;
        }
    };
}    // namespace hpx::agas
