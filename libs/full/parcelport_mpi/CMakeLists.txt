# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI))
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(parcelport_mpi_headers
    hpx/parcelport_mpi/header.hpp
    hpx/parcelport_mpi/locality.hpp
    hpx/parcelport_mpi/receiver.hpp
    hpx/parcelport_mpi/receiver_connection.hpp
    hpx/parcelport_mpi/sender.hpp
    hpx/parcelport_mpi/sender_connection.hpp
    hpx/parcelport_mpi/tag_provider.hpp
)

# cmake-format: off
set(parcelport_mpi_compat_headers)
# cmake-format: on

set(parcelport_mpi_sources locality.cpp parcelport_mpi.cpp)

include(HPX_SetupMPI)
hpx_setup_mpi()

include(HPX_AddModule)
add_hpx_module(
  full parcelport_mpi
  GLOBAL_HEADER_GEN OFF
  SOURCES ${parcelport_mpi_sources}
  HEADERS ${parcelport_mpi_headers}
  COMPAT_HEADERS ${parcelport_mpi_compat_headers}
  DEPENDENCIES hpx_core hpx_dependencies_boost Mpi::mpi
  MODULE_DEPENDENCIES hpx_actions hpx_command_line_handling hpx_parcelset
                      hpx_parcelset_base hpx_plugin_factories
  CMAKE_SUBDIRS examples tests
)

set(HPX_STATIC_PARCELPORT_PLUGINS
    ${HPX_STATIC_PARCELPORT_PLUGINS} parcelport_mpi
    CACHE INTERNAL "" FORCE
)
