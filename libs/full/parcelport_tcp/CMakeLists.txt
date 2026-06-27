# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP))
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(parcelport_tcp_headers
    hpx/parcelport_tcp/connection_handler.hpp hpx/parcelport_tcp/locality.hpp
    hpx/parcelport_tcp/receiver.hpp hpx/parcelport_tcp/sender.hpp
)

# cmake-format: off
set(parcelport_tcp_compat_headers)
# cmake-format: on

set(parcelport_tcp_sources connection_handler_tcp.cpp locality.cpp
                           parcelport_tcp.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full parcelport_tcp
  GLOBAL_HEADER_GEN OFF
  SOURCES ${parcelport_tcp_sources}
  HEADERS ${parcelport_tcp_headers}
  COMPAT_HEADERS ${parcelport_tcp_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions hpx_command_line_handling hpx_parcelset
                      hpx_parcelset_base hpx_plugin_factories
  CMAKE_SUBDIRS examples tests
)

set(HPX_STATIC_PARCELPORT_PLUGINS
    ${HPX_STATIC_PARCELPORT_PLUGINS} parcelport_tcp
    CACHE INTERNAL "" FORCE
)
