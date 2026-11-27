# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(version_headers hpx/version.hpp hpx/version/api.hpp)

set(version_sources version.cpp)

# the libraries the modules use register their versions themselves (see
# hpx::config_registry::add_library_version)

include(HPX_AddModule)
add_hpx_module(
  core version
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${version_sources}
  HEADERS ${version_headers}
  EXCLUDE_FROM_GLOBAL_HEADER hpx/version.hpp
  MODULE_DEPENDENCIES hpx_config hpx_config_registry hpx_format hpx_prefix
                      hpx_preprocessor
  DEPENDENCIES Hwloc::hwloc
  CMAKE_SUBDIRS tests
)

target_include_directories(
  hpx_version PRIVATE $<BUILD_INTERFACE:${HPX_BINARY_DIR}>
)
