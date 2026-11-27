# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(tuning_headers
    hpx/tuning/chunk_costs.hpp
    hpx/tuning/host_fingerprint.hpp
    hpx/tuning/idle_settings.hpp
    hpx/tuning/ini_file.hpp
    hpx/tuning/machine_description.hpp
    hpx/tuning/microbenchmarks.hpp
    hpx/tuning/statistics.hpp
    hpx/tuning/tuned_setting.hpp
    hpx/tuning/tuning_table.hpp
)

# cmake-format: off
set(tuning_compat_headers)
# cmake-format: on

set(tuning_sources
    chunk_costs.cpp
    host_fingerprint.cpp
    idle_settings.cpp
    ini_file.cpp
    machine_description.cpp
    microbenchmarks.cpp
    statistics.cpp
    tuning_table.cpp
)

include(HPX_AddModule)
add_hpx_module(
  core tuning
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${tuning_sources}
  HEADERS ${tuning_headers}
  COMPAT_HEADERS ${tuning_compat_headers}
  MODULE_DEPENDENCIES
    hpx_config
    hpx_execution
    hpx_executors
    hpx_format
    hpx_threading_base
    hpx_topology
    hpx_version
  CMAKE_SUBDIRS examples tests
)
