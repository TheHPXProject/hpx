//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/agas_base/component_namespace.hpp>
#include <hpx/agas_base/server/component_namespace.hpp>

#include <hpx/config/warnings_prefix.hpp>

using hpx::agas::server::component_namespace;

HPX_DEFINE_COMPONENT_NAME(component_namespace, hpx_component_namespace)
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(component_namespace,
    to_int(components::component_enum_type::agas_component_namespace))

HPX_REGISTER_ACTION_ID(
    hpx::agas::server::component_namespace::bind_prefix_action,
    component_namespace_bind_prefix_action,
    hpx::actions::component_namespace_bind_prefix_action_id)

HPX_REGISTER_ACTION_ID(hpx::agas::server::component_namespace::bind_name_action,
    component_namespace_bind_name_action,
    hpx::actions::component_namespace_bind_name_action_id)

HPX_REGISTER_ACTION_ID(
    hpx::agas::server::component_namespace::resolve_id_action,
    component_namespace_resolve_id_action,
    hpx::actions::component_namespace_resolve_id_action_id)

HPX_REGISTER_ACTION_ID(hpx::agas::server::component_namespace::unbind_action,
    component_namespace_unbind_action,
    hpx::actions::component_namespace_unbind_action_id)

HPX_REGISTER_ACTION_ID(
    hpx::agas::server::component_namespace::iterate_types_action,
    component_namespace_iterate_types_action,
    hpx::actions::component_namespace_iterate_types_action_id)

HPX_REGISTER_ACTION_ID(
    hpx::agas::server::component_namespace::get_component_type_name_action,
    component_namespace_get_component_type_action,
    hpx::actions::component_namespace_get_component_type_action_id)

HPX_REGISTER_ACTION_ID(
    hpx::agas::server::component_namespace::get_num_localities_action,
    component_namespace_get_num_localities_action,
    hpx::actions::component_namespace_get_num_localities_action_id)
