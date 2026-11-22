//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

///////////////////////////////////////////////////////////////////////////////
// from hpx/runtime_local/component_startup_shutdown_base.hpp

/// This macro is used to register the given component factory with
/// Hpx.Plugin. This macro has to be used for each of the components.
#define HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY(RegistryType, componentname)    \
    HPX_PLUGIN_EXPORT(HPX_PLUGIN_COMPONENT_PREFIX,                             \
        ::hpx::components::component_startup_shutdown_base, RegistryType,      \
        componentname, startup_shutdown)                                       \
/**/
#define HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY_DYNAMIC(                        \
    RegistryType, componentname)                                               \
    HPX_PLUGIN_EXPORT_DYNAMIC(HPX_PLUGIN_COMPONENT_PREFIX,                     \
        ::hpx::components::component_startup_shutdown_base, RegistryType,      \
        componentname, startup_shutdown)                                       \
/**/

/// This macro is used to define the required Hpx.Plugin entry point for the
/// startup/shutdown registry. This macro has to be used in not more than one
/// compilation unit of a component module.
#define HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS()                              \
    HPX_PLUGIN_EXPORT_LIST(HPX_PLUGIN_COMPONENT_PREFIX, startup_shutdown)      \
    HPX_INIT_REGISTRY_STARTUP_SHUTDOWN_STATIC(                                 \
        HPX_PLUGIN_COMPONENT_PREFIX, startup_shutdown)                         \
/**/
#define HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS_DYNAMIC()                      \
    HPX_PLUGIN_EXPORT_LIST_DYNAMIC(                                            \
        HPX_PLUGIN_COMPONENT_PREFIX, startup_shutdown)                         \
    /**/
