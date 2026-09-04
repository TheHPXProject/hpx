# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_OPENSHMEM)
  include(HPX_SetupOpenSHMEM)
  hpx_setup_openshmem()
  set(additional_dependencies OpenSHMEM::openshmem)
else()
  return()
endif()

set(openshmem_base_headers hpx/openshmem_base/openshmem_environment.hpp)

set(openshmem_base_sources openshmem_environment.cpp)

include(HPX_AddModule)
add_hpx_module(
  core openshmem_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${openshmem_base_sources}
  HEADERS ${openshmem_base_headers}
  MODULE_DEPENDENCIES
    hpx_concepts
    hpx_errors
    hpx_format
    hpx_logging
    hpx_runtime_configuration
    hpx_string_util
    hpx_synchronization
    hpx_util
  DEPENDENCIES ${additional_dependencies}
)