//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/components/process/process.hpp>
#include <hpx/components/process/server/child.hpp>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality, We register the module dynamically
// as no executable links against it.
HPX_REGISTER_COMPONENT_MODULE()

using child_type = hpx::components::process::server::child;

HPX_REGISTER_COMPONENT(hpx::components::component<child_type>,
    hpx_components_process_child_factory,
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(child_type)
