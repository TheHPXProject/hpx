# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# the parcel layer needs networking
if(NOT HPX_WITH_DISTRIBUTED_RUNTIME OR NOT HPX_WITH_NETWORKING)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(parcel_tuning_headers
    hpx/parcel_tuning/parcel_settings.hpp hpx/parcel_tuning/peer_nodes.hpp
    hpx/parcel_tuning/transfers.hpp
)

# cmake-format: off
set(parcel_tuning_compat_headers)
# cmake-format: on

set(parcel_tuning_sources parcel_settings.cpp peer_nodes.cpp transfers.cpp)

include(HPX_AddModule)
add_hpx_module(
  full parcel_tuning
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${parcel_tuning_sources}
  HEADERS ${parcel_tuning_headers}
  COMPAT_HEADERS ${parcel_tuning_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions_base hpx_async_distributed hpx_naming_base
                      hpx_parcelset
  CMAKE_SUBDIRS examples tests
)
