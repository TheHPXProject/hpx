//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/supervision_dispatch/server/registry.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE()

using supervision_registry_component = hpx::supervision::server::registry;

HPX_REGISTER_COMPONENT(
    hpx::components::component<supervision_registry_component>,
    supervision_registry_component, hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(supervision_registry_component)

#include <hpx/config/warnings_suffix.hpp>
