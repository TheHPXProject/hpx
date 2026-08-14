# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(components_headers
    hpx/components/basename_registration.hpp
    hpx/components/basename_registration_fwd.hpp
    hpx/components/components_fwd.hpp
    hpx/components/client.hpp
    hpx/components/client_base.hpp
    hpx/components/executor_component.hpp
    hpx/components/get_ptr.hpp
    hpx/components/make_client.hpp
)

# cmake-format: off
set(components_compat_headers
    hpx/components_fwd.hpp => hpx/modules/components.hpp
    hpx/runtime/basename_registration.hpp => hpx/modules/components.hpp
    hpx/runtime/basename_registration_fwd.hpp => hpx/modules/components.hpp
    hpx/runtime/components/client.hpp => hpx/modules/components.hpp
    hpx/runtime/components/client_base.hpp => hpx/modules/components.hpp
    hpx/runtime/components/make_client.hpp => hpx/modules/components.hpp
    hpx/runtime/components/server/executor_component.hpp => hpx/modules/components.hpp
    hpx/runtime/get_ptr.hpp => hpx/modules/components.hpp
)
# cmake-format: on

set(components_sources basename_registration.cpp client_base.cpp)

include(HPX_AddModule)
add_hpx_module(
  full components
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${components_sources}
  HEADERS ${components_headers}
  COMPAT_HEADERS ${components_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions_base hpx_components_base hpx_naming_base
  CMAKE_SUBDIRS examples tests
)
