//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_distributed.hpp>

#include <hpx/supervision_dispatch/sentinel.hpp>
#include <hpx/supervision_dispatch/server/sentinel.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::supervision {

    sentinel::sentinel(hpx::id_type const& target_locality)
      : base_type(hpx::new_<server::sentinel>(target_locality))
    {
    }

    sentinel::sentinel(std::string const& symbolic_name)
      : base_type(symbolic_name)
    {
    }

    sentinel::sentinel(hpx::future<hpx::id_type>&& f)
      : base_type(HPX_MOVE(f))
    {
    }

    hpx::future<hpx::supervision::publish_result> sentinel::start(
        std::uint64_t const epoch) const
    {
        using action_type = hpx::supervision::server::sentinel::start_action;
        return async(action_type(), this->get_id(), epoch);
    }

    publish_result sentinel::start(hpx::launch::sync_policy,
        std::uint64_t const epoch, hpx::error_code& ec) const
    {
        return start(epoch).get(ec);
    }

    hpx::future<bool> sentinel::register_name()
    {
        std::uint32_t const locality_id =
            hpx::naming::get_locality_id_from_id(this->get_id());
        std::string name = "/" + std::to_string(locality_id) +
            "/supervision_dispatch/sentinel";
        return this->register_as(HPX_MOVE(name));
    }

    bool sentinel::register_name(hpx::launch::sync_policy, hpx::error_code& ec)
    {
        return register_name().get(ec);
    }

    hpx::future<hpx::id_type> sentinel::unregister_name() const
    {
        return hpx::agas::unregister_name(this->registered_name());
    }

    hpx::id_type sentinel::unregister_name(
        hpx::launch::sync_policy, hpx::error_code& ec) const
    {
        return unregister_name().get(ec);
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
