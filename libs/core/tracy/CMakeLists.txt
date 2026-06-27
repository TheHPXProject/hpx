# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Enable Tracy module only if HPX_WITH_TRACY is set
if(NOT ${HPX_WITH_TRACY})
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Setup Tracy
include(HPX_SetupTracy)

set(tracy_headers hpx/tracy/tracy.hpp hpx/tracy/tracy_lock.hpp
                  hpx/tracy/tracy_tls.hpp
)

set(tracy_sources tracy.cpp tracy_init.cpp tracy_lock.cpp tracy_tls.cpp)

include(HPX_AddModule)
add_hpx_module(
  core tracy
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${tracy_sources}
  HEADERS ${tracy_headers}
  MODULE_DEPENDENCIES hpx_config
  PRIVATE_DEPENDENCIES tracy::tracy
  CMAKE_SUBDIRS tests
)
