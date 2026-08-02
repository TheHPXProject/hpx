# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if((NOT HPX_WITH_DISTRIBUTED_RUNTIME) OR (NOT HPX_WITH_SUPERVISION))
  return()
endif()

set(HPX_COMPONENTS
    ${HPX_COMPONENTS} supervision_dispatch
    CACHE INTERNAL "list of HPX components"
)

set(supervision_dispatch_headers
    hpx/supervision_dispatch/server/registry.hpp
    hpx/supervision_dispatch/server/sentinel.hpp
    hpx/supervision_dispatch/discovery.hpp
    hpx/supervision_dispatch/dispatch_api.hpp
    hpx/supervision_dispatch/dispatch_work.hpp
    hpx/supervision_dispatch/registry.hpp
    hpx/supervision_dispatch/sentinel.hpp
    hpx/supervision_dispatch/export_definitions.hpp
    hpx/supervision_dispatch.hpp
)

set(supervision_dispatch_sources
    server/registry_server.cpp
    server/sentinel_server.cpp
    discovery.cpp
    dispatch_api.cpp
    registry.cpp
    sentinel.cpp
    component_module.cpp
)

add_hpx_component(
  supervision_dispatch INTERNAL_FLAGS
  FOLDER "Core/Components/Supervision"
  INSTALL_HEADERS
  INSTALL_COMPONENT runtime ${HPX_WITH_UNITY_BUILD_OPTION}
                            ${HPX_WITH_CXX_MODULES_OPTION}
  PREPEND_SOURCE_ROOT PREPEND_HEADER_ROOT
  HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include"
  HEADERS ${supervision_dispatch_headers}
  SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src"
  SOURCES ${supervision_dispatch_sources}
)

target_compile_definitions(
  supervision_dispatch_component
  PRIVATE HPX_SUPERVISION_DISPATCH_MODULE_EXPORTS
)

add_hpx_pseudo_dependencies(
  components.supervision_dispatch supervision_dispatch_component
)

# MSVC V1951 and V1952 exposes ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND (MSVC_VERSION LESS_EQUAL 1952)
)
  target_compile_definitions(
    supervision_dispatch_component PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()

add_subdirectory(tests)
add_subdirectory(examples)
