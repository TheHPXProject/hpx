# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(agas_headers hpx/agas/addressing_service.hpp hpx/agas/agas_fwd.hpp
                 hpx/agas/state.hpp
)

# cmake-format: off
set(agas_compat_headers
    hpx/runtime/agas_fwd.hpp => hpx/modules/agas.hpp
    hpx/runtime/agas/addressing_service.hpp => hpx/modules/agas.hpp
)
# cmake-format: on

set(agas_sources addressing_service.cpp detail/interface.cpp route.cpp
                 state.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full agas
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${agas_sources}
  HEADERS ${agas_headers}
  COMPAT_HEADERS ${agas_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES
    hpx_actions_base
    hpx_agas_base
    hpx_async_distributed
    hpx_components_base
    hpx_naming
    hpx_naming_base
    hpx_parcelset
    hpx_parcelset_base
  CMAKE_SUBDIRS examples tests
)
