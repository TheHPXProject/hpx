//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2012-2024 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <hpx/agas_base/detail/bootstrap_locality_namespace.hpp>
#include <hpx/agas_base/server/locality_namespace.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hpx::agas::detail {

    bootstrap_locality_namespace::bootstrap_locality_namespace(
        server::primary_namespace* primary)
      : server_(primary)
    {
    }

    naming::address bootstrap_locality_namespace::addr() const
    {
        return naming::address(agas::get_locality(),
            to_int(components::component_enum_type::agas_locality_namespace),
            this->ptr());
    }

    hpx::id_type bootstrap_locality_namespace::gid() const
    {
        return hpx::id_type(
            naming::gid_type(agas::locality_ns_msb, agas::locality_ns_lsb),
            hpx::id_type::management_type::unmanaged);
    }

    std::uint32_t bootstrap_locality_namespace::allocate(
        parcelset::endpoints_type const& endpoints, std::uint64_t count,
        std::uint32_t num_threads, naming::gid_type const& suggested_prefix)
    {
        return server_.allocate(
            endpoints, count, num_threads, suggested_prefix);
    }

    void bootstrap_locality_namespace::free(naming::gid_type const& locality)
    {
        server_.free(locality);
    }

    std::vector<std::uint32_t> bootstrap_locality_namespace::localities()
    {
        return server_.localities();
    }

    parcelset::endpoints_type bootstrap_locality_namespace::resolve_locality(
        naming::gid_type const& locality)
    {
        return server_.resolve_locality(locality);
    }

    std::uint32_t bootstrap_locality_namespace::get_num_localities()
    {
        return server_.get_num_localities();
    }

    hpx::future<std::uint32_t>
    bootstrap_locality_namespace::get_num_localities_async()
    {
        return hpx::make_ready_future(server_.get_num_localities());
    }

    std::vector<std::uint32_t> bootstrap_locality_namespace::get_num_threads()
    {
        return server_.get_num_threads();
    }

    hpx::future<std::vector<std::uint32_t>>
    bootstrap_locality_namespace::get_num_threads_async()
    {
        return hpx::make_ready_future(server_.get_num_threads());
    }

    std::uint32_t bootstrap_locality_namespace::get_num_overall_threads()
    {
        return server_.get_num_overall_threads();
    }

    hpx::future<std::uint32_t>
    bootstrap_locality_namespace::get_num_overall_threads_async()
    {
        return hpx::make_ready_future(server_.get_num_overall_threads());
    }

    void bootstrap_locality_namespace::register_server_instance(
        std::uint32_t locality_id)
    {
        HPX_ASSERT(locality_id == 0);
        HPX_UNUSED(locality_id);
        char const* servicename("locality#0/");
        server_.register_server_instance(servicename);
    }

    void bootstrap_locality_namespace::unregister_server_instance(
        error_code& ec)
    {
        server_.unregister_server_instance(ec);
    }
}    // namespace hpx::agas::detail
