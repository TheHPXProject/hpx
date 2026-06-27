# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/checkpoint/include
set(checkpoint_headers hpx/checkpoint/checkpoint.hpp)

# Default location is $HPX_ROOT/libs/checkpoint/include_compatibility
# cmake-format: off
set(checkpoint_compat_headers
    hpx/checkpoint.hpp => hpx/modules/checkpoint.hpp
    hpx/util/checkpoint.hpp => hpx/modules/checkpoint.hpp
)
# cmake-format: on

set(checkpoint_sources checkpoint.cpp)

include(HPX_AddModule)
add_hpx_module(
  full checkpoint
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${checkpoint_sources}
  HEADERS ${checkpoint_headers}
  COMPAT_HEADERS ${checkpoint_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_async_distributed hpx_actions_base hpx_components_base
                      hpx_naming hpx_runtime_components hpx_runtime_distributed
  CMAKE_SUBDIRS examples tests
)
