# Copyright (c) 2023 Chistopher Taylor
# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT
   (HPX_WITH_NETWORKING
    AND HPX_WITH_PARCELPORT_GASNET
    AND HPX_WITH_PARCELPORT_GASNET_CONDUIT)
)
  return()
endif()

include(HPX_SetupGasnet)
hpx_setup_gasnet()

set(parcelport_gasnet_headers
    hpx/parcelport_gasnet/header.hpp
    hpx/parcelport_gasnet/locality.hpp
    hpx/parcelport_gasnet/receiver.hpp
    hpx/parcelport_gasnet/receiver_connection.hpp
    hpx/parcelport_gasnet/sender.hpp
    hpx/parcelport_gasnet/sender_connection.hpp
    hpx/parcelport_gasnet/tag_provider.hpp
)

set(parcelport_gasnet_sources locality.cpp parcelport_gasnet.cpp)

if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
  set(gasnet_additional_dependencies Mpi::mpi)
endif()

include(HPX_AddModule)
add_hpx_module(
  full parcelport_gasnet
  GLOBAL_HEADER_GEN OFF
  SOURCES ${parcelport_gasnet_sources}
  HEADERS ${parcelport_gasnet_headers}
  DEPENDENCIES hpx_core hpx_gasnet_base PkgConfig::GASNET
               ${gasnet_additional_dependencies}
  MODULE_DEPENDENCIES hpx_actions hpx_command_line_handling hpx_parcelset_base
                      hpx_parcelset hpx_plugin_factories
  CMAKE_SUBDIRS examples tests
)

set(HPX_STATIC_PARCELPORT_PLUGINS
    ${HPX_STATIC_PARCELPORT_PLUGINS} parcelport_gasnet
    CACHE INTERNAL "" FORCE
)
