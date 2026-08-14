# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(properties_headers hpx/property.hpp hpx/properties/property.hpp)

include(HPX_AddModule)
add_hpx_module(
  core properties
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${properties_headers}
  EXCLUDE_FROM_GLOBAL_HEADER hpx/property.hpp
  MODULE_DEPENDENCIES hpx_config
  CMAKE_SUBDIRS examples tests
)
