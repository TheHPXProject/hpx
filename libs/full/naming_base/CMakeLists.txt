# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

# Default location is $HPX_ROOT/libs/naming_base/include
set(naming_base_headers
    hpx/naming_base/address.hpp hpx/naming_base/gid_type.hpp
    hpx/naming_base/id_type.hpp hpx/naming_base/naming_base.hpp
    hpx/naming_base/unmanaged.hpp
)

# cmake-format: off
set(naming_base_compat_headers
    hpx/runtime/naming/address.hpp => hpx/modules/naming_base.hpp
    hpx/runtime/naming/id_type.hpp => hpx/modules/naming_base.hpp
    hpx/runtime/naming/unmanaged.hpp => hpx/modules/naming_base.hpp
)
# cmake-format: on

set(naming_base_sources address.cpp gid_type.cpp id_type.cpp unmanaged.cpp)

include(HPX_AddModule)
add_hpx_module(
  full naming_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${naming_base_headers}
  COMPAT_HEADERS ${naming_base_compat_headers}
  SOURCES ${naming_base_sources}
  DEPENDENCIES hpx_core
  CMAKE_SUBDIRS examples tests
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION
                                             VERSION_GREATER_EQUAL 22
)
  # Workaround for Clang 22 ICE (exit code 139) during C++20 modules template
  # instantiation
  target_compile_options(hpx_naming_base PRIVATE -O1)
endif()
