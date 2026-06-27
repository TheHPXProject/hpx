# Copyright (c) 2025 Jiakun Yan
# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCW))
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(parcelport_lcw_headers
    hpx/parcelport_lcw/config.hpp
    hpx/parcelport_lcw/header.hpp
    hpx/parcelport_lcw/helper.hpp
    hpx/parcelport_lcw/locality.hpp
    hpx/parcelport_lcw/receiver_base.hpp
    hpx/parcelport_lcw/sender_base.hpp
    hpx/parcelport_lcw/sender_connection_base.hpp
    hpx/parcelport_lcw/backlog_queue.hpp
    hpx/parcelport_lcw/parcelport_lcw.hpp
    hpx/parcelport_lcw/sendrecv/sender_sendrecv.hpp
    hpx/parcelport_lcw/sendrecv/sender_connection_sendrecv.hpp
    hpx/parcelport_lcw/sendrecv/receiver_sendrecv.hpp
    hpx/parcelport_lcw/sendrecv/receiver_connection_sendrecv.hpp
    hpx/parcelport_lcw/completion_manager_base.hpp
    hpx/parcelport_lcw/completion_manager/completion_manager_queue.hpp
)

# cmake-format: off
set(parcelport_lcw_compat_headers)
# cmake-format: on

set(parcelport_lcw_sources
    config.cpp
    locality.cpp
    parcelport_lcw.cpp
    backlog_queue.cpp
    sender_connection_base.cpp
    sender_base.cpp
    sendrecv/sender_sendrecv.cpp
    sendrecv/sender_connection_sendrecv.cpp
    sendrecv/receiver_sendrecv.cpp
    sendrecv/receiver_connection_sendrecv.cpp
)

include(HPX_SetupLCW)
hpx_setup_lcw()

include(HPX_AddModule)
add_hpx_module(
  full parcelport_lcw
  GLOBAL_HEADER_GEN OFF
  SOURCES ${parcelport_lcw_sources}
  HEADERS ${parcelport_lcw_headers}
  COMPAT_HEADERS ${parcelport_lcw_compat_headers}
  DEPENDENCIES hpx_core hpx_dependencies_boost LCW::LCW LCI::LCT
  MODULE_DEPENDENCIES hpx_actions hpx_command_line_handling hpx_parcelset
                      hpx_parcelset_base hpx_plugin_factories
  CMAKE_SUBDIRS examples tests
)

set(HPX_STATIC_PARCELPORT_PLUGINS
    ${HPX_STATIC_PARCELPORT_PLUGINS} parcelport_lcw
    CACHE INTERNAL "" FORCE
)
