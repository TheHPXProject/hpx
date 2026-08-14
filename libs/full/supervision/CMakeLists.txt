# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(supervision_headers hpx/supervision.hpp)

set(supervision_sources)

if(HPX_WITH_SUPERVISION)
  hpx_add_config_define_namespace(
    DEFINE HPX_SUPERVISION_HAVE_SUPERVISION NAMESPACE SUPERVISION
  )

  set(supervision_headers
      ${supervision_headers}
      hpx/supervision/supervision_manager.hpp
      hpx/supervision/supervision_api.hpp
      hpx/supervision/supervision_fwd.hpp
      hpx/supervision/testing.hpp
      hpx/supervision/server/activity_agent.hpp
      hpx/supervision/server/agent.hpp
      hpx/supervision/server/supervision_manager.hpp
  )

  set(supervision_sources
      supervision.cpp server/supervision_manager_server.cpp
      server/activity_agent_server.cpp server/agent_server.cpp
      supervision_manager.cpp
  )

  set(supervision_dependencies
      hpx_actions
      hpx_actions_base
      hpx_async_distributed
      hpx_components
      hpx_components_base
      hpx_naming
      hpx_naming_base
      hpx_runtime_components
  )

  set(supervision_subdirectories examples tests)
endif()

include(HPX_AddModule)
add_hpx_module(
  full supervision
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${supervision_sources}
  HEADERS ${supervision_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/supervision.hpp"
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES ${supervision_dependencies}
  CMAKE_SUBDIRS ${supervision_subdirectories}
)
