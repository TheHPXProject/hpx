# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_NETWORKING AND HPX_WITH_DEVICE_CONDUIT_NCCL))
  return()
endif()

# NCCL with CUDA, RCCL with HIP (HPX::nccl)
include(HPX_SetupNCCL)
hpx_setup_nccl()

set(device_conduit_nccl_headers hpx/device_conduit_nccl/nccl_conduit.hpp)
set(device_conduit_nccl_sources nccl_conduit.cpp)

include(HPX_AddModule)
add_hpx_module(
  full device_conduit_nccl
  GLOBAL_HEADER_GEN OFF
  SOURCES ${device_conduit_nccl_sources}
  HEADERS ${device_conduit_nccl_headers}
  DEPENDENCIES hpx_core HPX::nccl
  MODULE_DEPENDENCIES
    hpx_actions
    hpx_actions_base
    hpx_async_distributed
    hpx_naming_base
    hpx_parcelset
    hpx_parcelset_base
    hpx_plugin_factories
  CMAKE_SUBDIRS tests
)

set(HPX_STATIC_DEVICE_CONDUIT_PLUGINS
    ${HPX_STATIC_DEVICE_CONDUIT_PLUGINS} nccl
    CACHE INTERNAL "" FORCE
)
