# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_NETWORKING AND HPX_WITH_DEVICE_CONDUIT_UCX))
  return()
endif()

# the UCX of the MPI library if that is UCX based
include(HPX_SetupUCX)
hpx_setup_ucx()
hpx_info("    UCX ${UCX_VERSION} for the device conduit: ${HPX_UCX_ROOT}")

set(device_conduit_ucx_headers hpx/device_conduit_ucx/ucx_conduit.hpp)
set(device_conduit_ucx_sources ucx_conduit.cpp)

# the CUDA driver API (the CUDA context of the calling OS thread)
set(device_conduit_ucx_dependencies hpx_core PkgConfig::UCX)
if(HPX_WITH_CUDA)
  list(APPEND device_conduit_ucx_dependencies CUDA::cuda_driver)
endif()

include(HPX_AddModule)
add_hpx_module(
  full device_conduit_ucx
  GLOBAL_HEADER_GEN OFF
  SOURCES ${device_conduit_ucx_sources}
  HEADERS ${device_conduit_ucx_headers}
  DEPENDENCIES ${device_conduit_ucx_dependencies}
  MODULE_DEPENDENCIES
    hpx_actions
    hpx_actions_base
    hpx_async_distributed
    hpx_naming_base
    hpx_parcelset
    hpx_parcelset_base
    hpx_plugin_factories
)

set(HPX_STATIC_DEVICE_CONDUIT_PLUGINS
    ${HPX_STATIC_DEVICE_CONDUIT_PLUGINS} ucx
    CACHE INTERNAL "" FORCE
)
