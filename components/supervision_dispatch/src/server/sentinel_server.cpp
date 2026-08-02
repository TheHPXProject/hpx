//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>

#include <hpx/supervision_dispatch/server/sentinel.hpp>

#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

HPX_REGISTER_ACTION(hpx::supervision::server::sentinel::start_action,
    supervision_dispatch_sentinel_start_action)

namespace hpx::supervision::server {

    sentinel::sentinel() = default;

    hpx::supervision::publish_result sentinel::start(
        std::uint64_t const epoch) const
    {
        return hpx::supervision::publish_event(hpx::launch::sync,
            hpx::find_here(), get_unmanaged_id(),
            hpx::supervision::event::started, epoch);
    }
}    // namespace hpx::supervision::server

#include <hpx/config/warnings_suffix.hpp>
