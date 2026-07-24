//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/supervision/supervision_api.hpp>
#include <hpx/supervision/supervision_manager.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace hpx::supervision {

    supervision_manager::supervision_manager(util::runtime_configuration const&)
      : server_(std::make_unique<server::supervision_manager>())
    {
    }

    void supervision_manager::register_server_instance(error_code& ec) const
    {
        if (!server_)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::supervision::supervision_manager::register_server_"
                "instance",
                "server is not registered");
            return;
        }

        // register root server
        auto const locality_id = agas::get_locality_id();
        std::string const str("locality#" + std::to_string(locality_id) + "/");
        server_->register_server_instance(str.c_str(), locality_id, ec);
    }

    void supervision_manager::unregister_server_instance(error_code& ec) const
    {
        if (!server_)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::supervision::supervision_manager::unregister_server_"
                "instance",
                "server is not registered");
            return;
        }

        server_->unregister_server_instance(ec);
    }

    publish_result supervision_manager::publish_event(
        hpx::id_type const& target, event const ev, std::uint64_t const epoch,
        hpx::error_code& ec) const
    {
        if (!server_)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::supervision::supervision_manager::publish_event",
                "server is not registered");
            return publish_result::already_terminal;
        }

        auto const result = server_->publish_event(target, ev, epoch);
        if (&ec != &throws)
            ec = make_success_code();
        return result;
    }

    lifecycle_state supervision_manager::query_state(
        hpx::id_type const& target, hpx::error_code& ec) const
    {
        if (!server_)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::supervision::supervision_manager::query_state",
                "server is not registered");
            return {};
        }

        auto result = server_->query_state(target);
        if (&ec != &throws)
            ec = make_success_code();
        return result;
    }

    hpx::id_type supervision_manager::register_observer(
        hpx::id_type const& target, hpx::id_type const& agent,
        std::optional<std::uint64_t> epoch_filter, hpx::error_code& ec) const
    {
        if (!server_)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::supervision::supervision_manager::register_observer",
                "server is not registered");
            return {};
        }

        auto result =
            server_->register_observer(target, agent, HPX_MOVE(epoch_filter));
        if (&ec != &throws)
            ec = make_success_code();
        return result;
    }

    void supervision_manager::unregister_observer(
        hpx::id_type const& observer_handle, hpx::error_code& ec) const
    {
        if (!server_)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "hpx::supervision::supervision_manager::unregister_observer",
                "server is not registered");
            return;
        }

        server_->unregister_observer(observer_handle);
        if (&ec != &throws)
            ec = make_success_code();
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type supervision_manager::get_service_instance(
        std::uint32_t const service_locality_id)
    {
        naming::gid_type const service(
            supervision::detail::supervision_manager_msb,
            supervision::detail::supervision_manager_lsb);
        return naming::replace_locality_id(service, service_locality_id);
    }

    naming::gid_type supervision_manager::get_service_instance(
        naming::gid_type const& dest, error_code& ec)
    {
        std::uint32_t const service_locality_id =
            naming::get_locality_id_from_gid(dest);
        if (service_locality_id == naming::invalid_locality_id)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "supervision_manager::get_service_instance",
                "can't retrieve a valid locality id from global address "
                "({1}): ",
                dest);
            return naming::gid_type();
        }
        return get_service_instance(service_locality_id);
    }

}    // namespace hpx::supervision
