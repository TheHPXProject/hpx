# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Special option to enable testing of FreeBSD specific limitations This is
# purely an option allowing to test whether things work properly on systems that
# may not report cores in the topology at all (e.g. FreeBSD). There is no need
# for a user to every enable this.
hpx_option(
  HPX_TOPOLOGY_WITH_ADDITIONAL_HWLOC_TESTING
  BOOL
  "Enable HWLOC filtering that makes it report no cores, this is purely an
  option supporting better testing - do not enable under normal circumstances.
  (default: OFF)"
  OFF
  ADVANCED
  CATEGORY "Modules"
  MODULE TOPOLOGY
)

if(HPX_TOPOLOGY_WITH_ADDITIONAL_HWLOC_TESTING)
  hpx_add_config_define_namespace(
    DEFINE HPX_TOPOLOGY_HAVE_ADDITIONAL_HWLOC_TESTING NAMESPACE TOPOLOGY
  )
endif()

# Default location is $HPX_ROOT/libs/topology/include
set(topology_headers
    hpx/topology/cpu_mask.hpp hpx/topology/detail/mask_fallback.hpp
    hpx/topology/scheduling_properties.hpp hpx/topology/topology.hpp
)

# Default location is $HPX_ROOT/libs/topology/include_compatibility
# cmake-format: off
set(topology_compat_headers
    hpx/topology.hpp => hpx/modules/topology.hpp
    hpx/runtime/threads/cpu_mask.hpp => hpx/modules/topology.hpp
    hpx/runtime/threads/topology.hpp => hpx/modules/topology.hpp
    hpx/util/cache_aligned_data.hpp => hpx/modules/topology.hpp
)
# cmake-format: on

# Default location is $HPX_ROOT/libs/topology/src
set(topology_sources cpu_mask.cpp topology.cpp)

include(HPX_AddModule)
add_hpx_module(
  core topology
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${topology_sources}
  HEADERS ${topology_headers}
  COMPAT_HEADERS ${topology_compat_headers}
  MODULE_DEPENDENCIES
    hpx_assertion
    hpx_async_base
    hpx_config
    hpx_concurrency
    hpx_datastructures
    hpx_errors
    hpx_format
    hpx_logging
    hpx_tag_invoke
    hpx_type_support
    hpx_util
  DEPENDENCIES Hwloc::hwloc
  CMAKE_SUBDIRS examples tests
)
