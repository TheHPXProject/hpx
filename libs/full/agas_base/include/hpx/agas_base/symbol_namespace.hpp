////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2012-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/agas_base/server/symbol_namespace.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT struct symbol_namespace
    {
        using server_type = server::symbol_namespace;
        using iterate_names_return_type = std::map<std::string, hpx::id_type>;

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

        static hpx::id_type symbol_namespace_locality(std::string const& key);

        symbol_namespace();
        ~symbol_namespace() = default;

        naming::address_type ptr() const;
        naming::address addr() const;
        hpx::id_type gid() const;

        hpx::future<bool> bind_async(
            std::string const& key, naming::gid_type const& gid) const;
        bool bind(std::string const& key, naming::gid_type const& gid) const;

        hpx::future<hpx::id_type> resolve_async(std::string const& key) const;
        hpx::id_type resolve(std::string const& key) const;

        hpx::future<hpx::id_type> unbind_async(std::string key) const;
        hpx::id_type unbind(std::string key) const;

        hpx::future<bool> on_event(std::string const& name,
            bool call_for_past_events, hpx::id_type lco) const;

        hpx::future<iterate_names_return_type> iterate_async(
            std::string const& pattern) const;
        iterate_names_return_type iterate(std::string const& pattern) const;

        void register_server_instance(std::uint32_t locality_id) const;
        void unregister_server_instance(error_code& ec) const;

        server::symbol_namespace& get_service() const
        {
            return *server_;
        }

    private:
        std::unique_ptr<server_type> server_{};
    };
}    // namespace hpx::agas

#include <hpx/config/warnings_suffix.hpp>
