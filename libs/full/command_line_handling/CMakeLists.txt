# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(command_line_handling_headers
    hpx/command_line_handling/command_line_handling.hpp
    hpx/command_line_handling/late_command_line_handling.hpp
    hpx/command_line_handling/parse_command_line.hpp
)

# cmake-format: off
set(command_line_handling_compat_headers
    hpx/util/command_line_handling.hpp => hpx/modules/command_line_handling.hpp
    hpx/util/parse_command_line.hpp => hpx/modules/command_line_handling.hpp
)
# cmake-format: on

set(command_line_handling_sources
    command_line_handling.cpp late_command_line_handling.cpp
    parse_command_line.cpp
)

if((HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI) OR HPX_WITH_ASYNC_MPI)
  include(HPX_SetupMPI)
  hpx_setup_mpi()
  set(additional_dependencies ${additional_dependencies} Mpi::mpi)
endif()
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  include(HPX_SetupLCI)
  hpx_setup_lci()
  set(additional_dependencies ${additional_dependencies} LCI::LCI)
endif()
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCW)
  include(HPX_SetupLCW)
  hpx_setup_lcw()
  set(additional_dependencies ${additional_dependencies} LCW::LCW LCI::LCT)
endif()
if(HPX_WITH_PARCELPORT_GASNET)
  include(HPX_SetupGasnet)
  hpx_setup_gasnet()
  if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
    set(additional_dependencies ${additional_dependencies} Mpi::mpi)
  endif()
  set(additional_dependencies ${additional_dependencies} PkgConfig::GASNET)
endif()
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_OPENSHMEM)
  include(HPX_SetupOpenSHMEM)
  hpx_setup_openshmem()
  set(additional_dependencies ${additional_dependencies} OpenSHMEM::openshmem
                             hpx_openshmem_base)
endif()

include(HPX_AddModule)
add_hpx_module(
  full command_line_handling
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${command_line_handling_sources}
  HEADERS ${command_line_handling_headers}
  COMPAT_HEADERS ${command_line_handling_compat_headers}
  DEPENDENCIES hpx_core ${additional_dependencies}
  CMAKE_SUBDIRS tests
)
