# Copyright (c) 2019-2022 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(compute_headers
    hpx/compute/detail/target_distribution_policy.hpp
    hpx/compute/host/distributed_target.hpp
    hpx/compute/host/get_targets.hpp
    hpx/compute/host.hpp
    hpx/compute/host/target_distribution_policy.hpp
    hpx/compute/traits.hpp
)

# cmake-format: off
set(compute_compat_headers
    hpx/compute/host/target.hpp => hpx/compute.hpp
    hpx/util/functional/new.hpp => hpx/compute.hpp
)
# cmake-format: on

set(compute_sources get_host_targets.cpp distributed_target.cpp
                    target_distribution_policy.cpp
)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  set(compute_optional_module_dependencies
      hpx_actions
      hpx_actions_base
      hpx_naming
      hpx_naming_base
      hpx_performance_counters
      hpx_async_distributed
      hpx_runtime_components
      hpx_runtime_distributed
  )
endif()

include(HPX_AddModule)
add_hpx_module(
  full compute
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${compute_sources}
  HEADERS ${compute_headers}
  COMPAT_HEADERS ${compute_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES ${compute_optional_module_dependencies}
  CMAKE_SUBDIRS examples tests
)
