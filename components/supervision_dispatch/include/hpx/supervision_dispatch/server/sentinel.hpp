//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/tracing.hpp>

#include <hpx/supervision_dispatch/export_definitions.hpp>

#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::supervision::server {

    ///////////////////////////////////////////////////////////////////////////
    class HPX_SUPERVISION_DISPATCH_EXPORT sentinel
      : public hpx::components::component_base<sentinel>
    {
    public:
        sentinel();

        /// Publish event::started for this sentinel at the given epoch.
        ///
        /// \param epoch The epoch associated with the event.
        ///
        /// \return The result of publishing the event.
        hpx::supervision::publish_result start(std::uint64_t epoch) const;

        struct start_action
          : hpx::actions::make_action_t<decltype(&sentinel::start),
                &sentinel::start, start_action>
        {
        };
    };
}    // namespace hpx::supervision::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::supervision::server::sentinel::start_action,
    supervision_dispatch_sentinel_start_action)

#include <hpx/config/warnings_suffix.hpp>
