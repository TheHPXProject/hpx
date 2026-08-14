////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/agas_base/gva.hpp>

#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT struct HPX_EXPORT primary_namespace
    {
        typedef hpx::tuple<naming::gid_type, gva, naming::gid_type>
            resolved_type;

        static naming::gid_type get_service_instance(
            std::uint32_t service_locality_id);

        static naming::gid_type get_service_instance(
            naming::gid_type const& dest, error_code& ec = throws);

        static naming::gid_type get_service_instance(hpx::id_type const& dest)
        {
            return get_service_instance(dest.get_gid());
        }

        static bool is_service_instance(naming::gid_type const& gid);

        static bool is_service_instance(hpx::id_type const& id)
        {
            return is_service_instance(id.get_gid());
        }

        primary_namespace();
        ~primary_namespace();

        naming::address::address_type ptr() const;
        naming::address addr() const;
        hpx::id_type gid() const;

        hpx::future<std::pair<hpx::id_type, naming::address>> begin_migration(
            naming::gid_type const& id);
        bool end_migration(naming::gid_type const& id);

        bool bind_gid(gva const& g, naming::gid_type const& id,
            naming::gid_type const& locality);
        future<bool> bind_gid_async(
            gva g, naming::gid_type id, naming::gid_type locality);

#if defined(HPX_HAVE_NETWORKING)
        void route(parcelset::parcel&& p,
            hpx::function<void(
                std::error_code const&, parcelset::parcel const&)>&& f);
#endif

        resolved_type resolve_gid(naming::gid_type const& id);
        hpx::future_or_value<resolved_type> resolve_full(naming::gid_type id);

        hpx::future_or_value<id_type> colocate(naming::gid_type id);

        naming::address unbind_gid(
            std::uint64_t count, naming::gid_type const& id);
        future<naming::address> unbind_gid_async(
            std::uint64_t count, naming::gid_type const& id);

        future_or_value<std::int64_t> increment_credit(std::int64_t credits,
            naming::gid_type lower, naming::gid_type upper);

        std::pair<naming::gid_type, naming::gid_type> allocate(
            std::uint64_t count);

        void set_local_locality(naming::gid_type const& g);

        void register_server_instance(std::uint32_t locality_id);
        void unregister_server_instance(error_code& ec);

        server::primary_namespace& get_service()
        {
            return *server_;
        }

    private:
        std::unique_ptr<server::primary_namespace> server_;
    };
}    // namespace hpx::agas

#include <hpx/config/warnings_suffix.hpp>
