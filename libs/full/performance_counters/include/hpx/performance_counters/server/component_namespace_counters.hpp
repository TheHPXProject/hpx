//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>

#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::agas {

    // Create statistics counter for component namespace on this component
    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    component_namespace_statistics_counter(std::string const& name);

    HPX_DEFINE_PLAIN_ACTION(HPX_CXX_EXPORT,
        component_namespace_statistics_counter,
        component_namespace_statistics_counter_action);
}    // namespace hpx::agas

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::component_namespace_statistics_counter_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::component_namespace_statistics_counter_action,
    component_namespace_statistics_counter_action)
