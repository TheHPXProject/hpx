//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/supervision/server/supervision_manager.hpp>
#include <hpx/supervision/supervision_fwd.hpp>

#include <cstdint>
#include <memory>

namespace hpx::supervision {

    ////////////////////////////////////////////////////////////////////////////
    class supervision_manager
    {
    public:
        explicit supervision_manager(util::runtime_configuration const& ini);

        // supervision API
        void publish_event(hpx::id_type const& target, event ev,
            hpx::error_code& ec = throws) const;

        lifecycle_state query_state(
            hpx::id_type const& target, hpx::error_code& ec = throws) const;

        hpx::id_type register_observer(hpx::id_type const& target,
            hpx::id_type const& agent, hpx::error_code& ec = throws) const;

        void unregister_observer(hpx::id_type const& observer_handle,
            hpx::error_code& ec = throws) const;

        // Helper functions
        void register_server_instance(error_code& ec = throws) const;
        void unregister_server_instance(error_code& ec = throws) const;

        static naming::gid_type get_service_instance(
            naming::gid_type const& dest, error_code& ec = throws);
        static naming::gid_type get_service_instance(
            std::uint32_t service_locality_id);

        static naming::gid_type get_service_instance(hpx::id_type const& dest)
        {
            return get_service_instance(dest.get_gid());
        }

    private:
        std::unique_ptr<server::supervision_manager> server_{};
    };

}    // namespace hpx::supervision
