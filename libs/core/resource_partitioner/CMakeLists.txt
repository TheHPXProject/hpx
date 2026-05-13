# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(resource_partitioner_headers
    hpx/modules/resource_partitioner_mode.hpp
    hpx/resource_partitioner/detail/create_partitioner.hpp
    hpx/resource_partitioner/detail/partitioner.hpp
    hpx/resource_partitioner/partitioner.hpp
    hpx/resource_partitioner/partitioner_fwd.hpp
    hpx/resource_partitioner/partitioner_mode.hpp
)

# cmake-format: off
set(resource_partitioner_compat_headers
    hpx/resource_partitioner.hpp => hpx/include/resource_partitioner.hpp
    hpx/runtime/resource/partitioner.hpp => hpx/include/resource_partitioner.hpp
    hpx/runtime/resource/partitioner_fwd.hpp => hpx/include/resource_partitioner.hpp
)
# cmake-format: on

set(resource_partitioner_sources detail_partitioner.cpp partitioner.cpp)

include(HPX_AddModule)
add_hpx_module(
  core resource_partitioner
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${resource_partitioner_sources}
  HEADERS ${resource_partitioner_headers}
  COMPAT_HEADERS ${resource_partitioner_compat_headers}
  MODULE_DEPENDENCIES
    hpx_affinity
    hpx_config
    hpx_datastructures
    hpx_errors
    hpx_format
    hpx_functional
    hpx_ini
    hpx_prefix
    hpx_runtime_configuration
    hpx_synchronization
    hpx_threading_base
    hpx_thread_pools
    hpx_topology
    hpx_type_support
    hpx_util
  CMAKE_SUBDIRS examples tests
)
