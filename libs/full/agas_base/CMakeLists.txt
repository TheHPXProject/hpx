# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(agas_base_headers
    hpx/agas_base/agas_fwd.hpp
    hpx/agas_base/component_namespace.hpp
    hpx/agas_base/detail/bootstrap_component_namespace.hpp
    hpx/agas_base/detail/bootstrap_locality_namespace.hpp
    hpx/agas_base/detail/hosted_component_namespace.hpp
    hpx/agas_base/detail/hosted_locality_namespace.hpp
    hpx/agas_base/gva.hpp
    hpx/agas_base/locality_namespace.hpp
    hpx/agas_base/primary_namespace.hpp
    hpx/agas_base/route.hpp
    hpx/agas_base/server/component_namespace.hpp
    hpx/agas_base/server/locality_namespace.hpp
    hpx/agas_base/server/primary_namespace.hpp
    hpx/agas_base/server/symbol_namespace.hpp
    hpx/agas_base/symbol_namespace.hpp
)

# cmake-format: off
set(agas_base_compat_headers
    hpx/runtime/agas/gva.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/component_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/locality_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/primary_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/detail/bootstrap_component_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/detail/bootstrap_locality_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/detail/hosted_component_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/detail/hosted_locality_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/server/component_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/server/locality_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/server/primary_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/server/symbol_namespace.hpp => hpx/modules/agas_base.hpp
    hpx/runtime/agas/symbol_namespace.hpp => hpx/modules/agas_base.hpp
)
# cmake-format: on

set(agas_base_sources
    component_namespace.cpp
    detail/bootstrap_component_namespace.cpp
    detail/bootstrap_locality_namespace.cpp
    detail/hosted_component_namespace.cpp
    detail/hosted_locality_namespace.cpp
    gva.cpp
    locality_namespace.cpp
    primary_namespace.cpp
    server/component_namespace_server.cpp
    server/locality_namespace_server.cpp
    server/primary_namespace_server.cpp
    server/symbol_namespace_server.cpp
    symbol_namespace.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full agas_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${agas_base_sources}
  HEADERS ${agas_base_headers}
  ADD_TO_GLOBAL_HEADER
    "hpx/agas_base/detail/bootstrap_component_namespace.hpp"
    "hpx/agas_base/detail/bootstrap_locality_namespace.hpp"
    "hpx/agas_base/detail/hosted_component_namespace.hpp"
    "hpx/agas_base/detail/hosted_locality_namespace.hpp"
  COMPAT_HEADERS ${agas_base_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES
    hpx_actions
    hpx_actions_base
    hpx_async_distributed
    hpx_components_base
    hpx_naming
    hpx_naming_base
    hpx_parcelset_base
  CMAKE_SUBDIRS examples tests
)
