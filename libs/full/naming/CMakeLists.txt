# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(naming_headers
    hpx/naming/credit_handling.hpp hpx/naming/naming.hpp
    hpx/naming/split_gid.hpp hpx/naming/detail/preprocess_gid_types.hpp
)

# cmake-format: off
set(naming_compat_headers
    hpx/runtime/naming_fwd.hpp => hpx/modules/naming.hpp
    hpx/runtime/naming/split_gid.hpp => hpx/modules/naming.hpp
)
# cmake-format: on

set(naming_sources credit_handling.cpp detail/preprocess_gid_types.cpp)

include(HPX_AddModule)
add_hpx_module(
  full naming
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${naming_sources}
  HEADERS ${naming_headers}
  ADD_TO_GLOBAL_HEADER "hpx/naming/detail/preprocess_gid_types.hpp"
  COMPAT_HEADERS ${naming_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_components_base hpx_naming_base
  CMAKE_SUBDIRS examples tests
)
