//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/components_base.hpp>

#include <hpx/agas_base/detail/bootstrap_component_namespace.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace hpx::agas::detail {

    naming::address bootstrap_component_namespace::addr() const
    {
        return naming::address(agas::get_locality(),
            to_int(components::component_enum_type::agas_component_namespace),
            this->ptr());
    }

    hpx::id_type bootstrap_component_namespace::gid() const
    {
        return hpx::id_type(
            naming::gid_type(agas::component_ns_msb, agas::component_ns_lsb),
            hpx::id_type::management_type::unmanaged);
    }

    components::component_type bootstrap_component_namespace::bind_prefix(
        std::string const& key, std::uint32_t prefix)
    {
        return server_.bind_prefix(key, prefix);
    }

    components::component_type bootstrap_component_namespace::bind_name(
        std::string const& name)
    {
        return server_.bind_name(name);
    }

    std::vector<std::uint32_t> bootstrap_component_namespace::resolve_id(
        components::component_type key)
    {
        return server_.resolve_id(key);
    }

    bool bootstrap_component_namespace::unbind(std::string const& key)
    {
        return server_.unbind(key);
    }

    void bootstrap_component_namespace::iterate_types(
        iterate_types_function_type const& f)
    {
        return server_.iterate_types(f);
    }

    std::string bootstrap_component_namespace::get_component_type_name(
        components::component_type type)
    {
        return server_.get_component_type_name(type);
    }

    hpx::future<std::uint32_t>
    bootstrap_component_namespace::get_num_localities(
        components::component_type type)
    {
        return hpx::make_ready_future(server_.get_num_localities(type));
    }

    void bootstrap_component_namespace::register_server_instance(
        std::uint32_t locality_id)
    {
        HPX_ASSERT(locality_id == 0);
        HPX_UNUSED(locality_id);
        char const* servicename("locality#0/");
        server_.register_server_instance(servicename);
    }

    void bootstrap_component_namespace::unregister_server_instance(
        error_code& ec)
    {
        server_.unregister_server_instance(ec);
    }
}    // namespace hpx::agas::detail
