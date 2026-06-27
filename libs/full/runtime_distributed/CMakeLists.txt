# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(runtime_distributed_headers
    hpx/runtime_distributed/applier.hpp
    hpx/runtime_distributed/applier_fwd.hpp
    hpx/runtime_distributed/big_boot_barrier.hpp
    hpx/runtime_distributed/copy_component.hpp
    hpx/runtime_distributed.hpp
    hpx/runtime_distributed/find_all_localities.hpp
    hpx/runtime_distributed/find_here.hpp
    hpx/runtime_distributed/find_localities.hpp
    hpx/runtime_distributed/get_locality_name.hpp
    hpx/runtime_distributed/get_num_localities.hpp
    hpx/runtime_distributed/migrate_component.hpp
    hpx/runtime_distributed/runtime_fwd.hpp
    hpx/runtime_distributed/runtime_support.hpp
    hpx/runtime_distributed/server/copy_component.hpp
    hpx/runtime_distributed/server/migrate_component.hpp
    hpx/runtime_distributed/server/runtime_support.hpp
    hpx/runtime_distributed/stubs/runtime_support.hpp
)

# cmake-format: off
set(runtime_distributed_compat_headers
    hpx/runtime/agas/big_boot_barrier.hpp => hpx/runtime_distributed.hpp
    hpx/runtime/applier/applier.hpp => hpx/runtime_distributed.hpp
    hpx/runtime/applier_fwd.hpp => hpx/runtime_distributed.hpp
    hpx/runtime/components/copy_component.hpp => hpx/modules/runtime_distributed.hpp
    hpx/runtime/components/migrate_component.hpp => hpx/modules/runtime_distributed.hpp
    hpx/runtime/find_here.hpp => hpx/modules/runtime_distributed.hpp
    hpx/runtime/find_all_localities.hpp => hpx/modules/runtime_distributed.hpp
    hpx/runtime/find_localities.hpp => hpx/modules/runtime_distributed.hpp
    hpx/runtime/runtime_fwd.hpp => hpx/modules/runtime_distributed.hpp
)
# cmake-format: on

set(runtime_distributed_sources
    applier.cpp
    big_boot_barrier.cpp
    get_locality_name.cpp
    locality_interface.cpp
    runtime_support.cpp
    runtime_distributed.cpp
    server/runtime_support_server.cpp
    stubs/runtime_support_stubs.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full runtime_distributed
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${runtime_distributed_sources}
  HEADERS ${runtime_distributed_headers}
  COMPAT_HEADERS ${runtime_distributed_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES
    hpx_actions
    hpx_actions_base
    hpx_agas
    hpx_agas_base
    hpx_async_colocated
    hpx_async_distributed
    hpx_components
    hpx_components_base
    hpx_command_line_handling
    hpx_distribution_policies
    hpx_naming_base
    hpx_parcelset
    hpx_parcelset_base
    hpx_performance_counters
    hpx_plugin_factories
    hpx_runtime_components
  CMAKE_SUBDIRS examples tests
)
