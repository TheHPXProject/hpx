# Copyright (c) 2007-2023 Hartmut Kaiser
# Copyright (c)      2011 Bryce Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

set(HPX_COMPONENTS
    ${HPX_COMPONENTS} iostreams
    CACHE INTERNAL "list of HPX components"
)

set(iostreams_headers
    hpx/components/iostreams/server/data_buffer.hpp
    hpx/components/iostreams/server/order_output.hpp
    hpx/components/iostreams/server/output_stream.hpp
    hpx/components/iostreams/export_definitions.hpp
    hpx/components/iostreams/manipulators.hpp
    hpx/components/iostreams/ostream.hpp
    hpx/components/iostreams/standard_streams.hpp
    hpx/components/iostreams/write_functions.hpp
    hpx/iostream.hpp
    hpx/include/iostreams.hpp
)

set(iostreams_sources server/output_stream.cpp component_module.cpp
                      manipulators.cpp standard_streams.cpp
)

add_hpx_component(
  iostreams INTERNAL_FLAGS
  FOLDER "Core/Components/IO"
  INSTALL_HEADERS PREPEND_HEADER_ROOT
  INSTALL_COMPONENT runtime
  HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include"
  HEADERS ${iostreams_headers}
  PREPEND_SOURCE_ROOT ${HPX_WITH_UNITY_BUILD_OPTION}
  ${HPX_WITH_CXX_MODULES_OPTION}
  SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src"
  SOURCES ${iostreams_sources}
)

target_compile_definitions(
  iostreams_component PRIVATE HPX_IOSTREAMS_MODULE_EXPORTS
)

add_hpx_pseudo_dependencies(components.iostreams iostreams_component)

# MSVC V1951 and V1952 exposes ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND ((MSVC_VERSION EQUAL 1951) OR (MSVC_VERSION EQUAL 1952))
)
  target_compile_definitions(
    iostreams_component PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()

add_subdirectory(tests)
add_subdirectory(examples)
