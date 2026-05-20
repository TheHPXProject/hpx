//  Copyright (c) 2015-2024 Hartmut Kaiser
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

#include <hpx/components/component_storage/server/migrate_to_storage.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE()

using component_storage_type = hpx::components::server::component_storage;

HPX_REGISTER_COMPONENT(hpx::components::component<component_storage_type>,
    component_storage_factory, hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(component_storage_type)

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_ACTION(
    hpx::components::server::component_storage::migrate_to_here_action,
    component_storage_migrate_component_to_here_action)
HPX_REGISTER_ACTION(
    hpx::components::server::component_storage::migrate_from_here_action,
    component_storage_migrate_component_from_here_action)
HPX_REGISTER_ACTION(hpx::components::server::component_storage::size_action,
    component_storage_size_action)
