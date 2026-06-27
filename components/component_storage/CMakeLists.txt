# Copyright (c) 2015-2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

set(HPX_COMPONENTS
    ${HPX_COMPONENTS} component_storage
    CACHE INTERNAL "list of HPX components"
)

set(component_storage_headers
    hpx/components/component_storage/server/component_storage.hpp
    hpx/components/component_storage/server/migrate_from_storage.hpp
    hpx/components/component_storage/server/migrate_to_storage.hpp
    hpx/components/component_storage/component_storage.hpp
    hpx/components/component_storage/export_definitions.hpp
    hpx/components/component_storage/migrate_from_storage.hpp
    hpx/components/component_storage/migrate_to_storage.hpp
    hpx/include/component_storage.hpp
)

set(component_storage_sources server/component_storage_server.cpp
                              component_module.cpp component_storage.cpp
)

add_hpx_component(
  component_storage INTERNAL_FLAGS
  FOLDER "Core/Components/IO"
  INSTALL_HEADERS PREPEND_HEADER_ROOT
  INSTALL_COMPONENT runtime
  HEADER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/include
  HEADERS ${component_storage_headers}
  PREPEND_SOURCE_ROOT ${HPX_WITH_UNITY_BUILD_OPTION}
  ${HPX_WITH_CXX_MODULES_OPTION}
  SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES ${component_storage_sources}
  DEPENDENCIES unordered_component
)

target_compile_definitions(
  component_storage_component PRIVATE HPX_MIGRATE_TO_STORAGE_MODULE_EXPORTS
)

add_hpx_pseudo_dependencies(
  components.component_storage component_storage_component
)

# MSVC V1951 and V1952 exposes ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 1952))
)
  target_compile_definitions(
    component_storage_component PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()

add_subdirectory(tests)
add_subdirectory(examples)
