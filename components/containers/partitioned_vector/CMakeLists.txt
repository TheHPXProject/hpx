# Copyright (c) 2015-2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

set(HPX_COMPONENTS
    ${HPX_COMPONENTS} partitioned_vector
    CACHE INTERNAL "list of HPX components"
)

set(partitioned_vector_headers
    hpx/components/containers/coarray/coarray.hpp
    hpx/components/containers/partitioned_vector/detail/view_element.hpp
    hpx/components/containers/partitioned_vector/export_definitions.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_component.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_component_decl.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_component_impl.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_decl.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_fwd.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_impl.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_local_view.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_local_view_iterator.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_predef.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_segmented_iterator.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_view.hpp
    hpx/components/containers/partitioned_vector/partitioned_vector_view_iterator.hpp
    hpx/components/containers/partitioned_vector/serialization/partitioned_vector.hpp
    hpx/include/partitioned_vector.hpp
    hpx/include/partitioned_vector_predef.hpp
    hpx/include/partitioned_vector_view.hpp
)

set(partitioned_vector_sources
    partitioned_vector_component.cpp partitioned_vector_component_double.cpp
    partitioned_vector_component_int.cpp
    partitioned_vector_component_std_string.cpp
)

add_hpx_component(
  partitioned_vector INTERNAL_FLAGS
  FOLDER "Core/Components/Containers"
  PREPEND_HEADER_ROOT INSTALL_HEADERS
  INSTALL_COMPONENT runtime
  HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include"
  HEADERS ${partitioned_vector_headers}
  PREPEND_SOURCE_ROOT ${HPX_WITH_UNITY_BUILD_OPTION}
  ${HPX_WITH_CXX_MODULES_OPTION}
  SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src"
  SOURCES ${partitioned_vector_sources}
)

target_compile_definitions(
  partitioned_vector_component PRIVATE HPX_PARTITIONED_VECTOR_MODULE_EXPORTS
)

add_hpx_pseudo_dependencies(
  components.containers.partitioned_vector partitioned_vector_component
)

# MSVC V1951 and V1952 exposes ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 1952))
)
  target_compile_definitions(
    partitioned_vector_component PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()

add_subdirectory(tests)
add_subdirectory(examples)
