# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(config_registry_headers hpx/modules/config_registry.hpp)

set(config_registry_sources config_entries.cpp config_registry.cpp)

include(HPX_AddModule)
add_hpx_module(
  core config_registry
  GLOBAL_HEADER_GEN OFF
  GLOBAL_HEADER_MODULE_GEN OFF
  SOURCES ${config_registry_sources}
  HEADERS ${config_registry_headers}
  CMAKE_SUBDIRS examples tests
)
