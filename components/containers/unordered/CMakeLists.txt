# Copyright (c) 2015-2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

set(HPX_COMPONENTS
    ${HPX_COMPONENTS} unordered
    CACHE INTERNAL "list of HPX components"
)

set(unordered_headers
    hpx/components/containers/unordered/partition_unordered_map_component.hpp
    hpx/components/containers/unordered/unordered_map.hpp
    hpx/components/containers/unordered/unordered_map_segmented_iterator.hpp
    hpx/include/unordered_map.hpp
)

set(unordered_sources partition_unordered_map_component.cpp)

add_hpx_component(
  unordered INTERNAL_FLAGS
  FOLDER "Core/Components/Containers"
  PREPEND_HEADER_ROOT INSTALL_HEADERS
  INSTALL_COMPONENT runtime
  HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include"
  HEADERS ${unordered_headers}
  PREPEND_SOURCE_ROOT ${HPX_WITH_UNITY_BUILD_OPTION}
  ${HPX_WITH_CXX_MODULES_OPTION}
  SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src"
  SOURCES ${unordered_sources}
)

add_hpx_pseudo_dependencies(components.containers.unordered unordered_component)

# MSVC V1951 and V1952 exposes ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 1952))
)
  target_compile_definitions(
    unordered_component PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()

add_subdirectory(tests)
add_subdirectory(examples)
