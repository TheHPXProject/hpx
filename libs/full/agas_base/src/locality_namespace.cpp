//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2011-2024 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/agas_base/locality_namespace.hpp>
#include <hpx/agas_base/server/locality_namespace.hpp>

#include <hpx/config/warnings_prefix.hpp>

using hpx::agas::server::locality_namespace;

HPX_DEFINE_COMPONENT_NAME(locality_namespace, hpx_locality_namespace)
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(locality_namespace,
    to_int(hpx::components::component_enum_type::agas_locality_namespace))

HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(hpx::parcelset::endpoints_type,
    parcelset_endpoints_type,
    hpx::actions::base_lco_with_value_parcelset_endpoints_get,
    hpx::actions::base_lco_with_value_parcelset_endpoints_set)

HPX_REGISTER_ACTION_ID(locality_namespace::allocate_action,
    locality_namespace_allocate_action,
    hpx::actions::locality_namespace_allocate_action_id)

HPX_REGISTER_ACTION_ID(locality_namespace::free_action,
    locality_namespace_free_action,
    hpx::actions::locality_namespace_free_action_id)

HPX_REGISTER_ACTION_ID(locality_namespace::localities_action,
    locality_namespace_localities_action,
    hpx::actions::locality_namespace_localities_action_id)

HPX_REGISTER_ACTION_ID(locality_namespace::resolve_locality_action,
    locality_namespace_resolve_locality_action,
    hpx::actions::locality_namespace_resolve_locality_action_id)

HPX_REGISTER_ACTION_ID(locality_namespace::get_num_localities_action,
    locality_namespace_get_num_localities_action,
    hpx::actions::locality_namespace_get_num_localities_action_id)

HPX_REGISTER_ACTION_ID(locality_namespace::get_num_threads_action,
    locality_namespace_get_num_threads_action,
    hpx::actions::locality_namespace_get_num_threads_action_id)

HPX_REGISTER_ACTION_ID(locality_namespace::get_num_overall_threads_action,
    locality_namespace_get_num_overall_threads_action,
    hpx::actions::locality_namespace_get_num_overall_threads_action_id)
