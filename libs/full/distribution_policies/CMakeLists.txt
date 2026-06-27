# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(distribution_policies_headers
    hpx/distribution_policies/binpacking_distribution_policy.hpp
    hpx/distribution_policies/colocating_distribution_policy.hpp
    hpx/distribution_policies/container_distribution_policy.hpp
    hpx/distribution_policies/default_distribution_policy.hpp
    hpx/distribution_policies/explicit_container_distribution_policy.hpp
    hpx/distribution_policies/target_distribution_policy.hpp
    hpx/distribution_policies/unwrapping_result_policy.hpp
)

# cmake-format: off
set(distribution_policies_compat_headers
    hpx/components/containers/container_distribution_policy.hpp => hpx/modules/distribution_policies.hpp
    hpx/runtime/components/binpacking_distribution_policy.hpp => hpx/modules/distribution_policies.hpp
    hpx/runtime/components/colocating_distribution_policy.hpp => hpx/modules/distribution_policies.hpp
    hpx/runtime/components/target_distribution_policy.hpp => hpx/modules/distribution_policies.hpp
    hpx/runtime/components/unwrapping_result_policy.hpp => hpx/modules/distribution_policies.hpp
    hpx/runtime/components/default_distribution_policy.hpp => hpx/modules/runtime_components.hpp
)
# cmake-format: on

set(distribution_policies_sources binpacking_distribution_policy.cpp
                                  distribution_policies.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full distribution_policies
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${distribution_policies_sources}
  HEADERS ${distribution_policies_headers}
  COMPAT_HEADERS ${distribution_policies_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES
    hpx_actions_base
    hpx_async_colocated
    hpx_async_distributed
    hpx_components
    hpx_components_base
    hpx_naming_base
    hpx_performance_counters
    hpx_runtime_components
  CMAKE_SUBDIRS examples tests
)
