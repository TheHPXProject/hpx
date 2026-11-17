# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(version_headers hpx/version.hpp hpx/version/api.hpp)

set(version_sources version.cpp)

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
  set(additional_dependencies ${additional_dependencies} LCW::LCW)
endif()
if(HPX_WITH_NETWORKING
   AND HPX_WITH_PARCELPORT_GASNET
   AND HPX_WITH_PARCELPORT_GASNET_CONDUIT
)
  include(HPX_SetupGasnet)
  hpx_setup_gasnet()
  set(additional_dependencies ${additional_dependencies} PkgConfig::GASNET)
endif()

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
  DEPENDENCIES Hwloc::hwloc ${additional_dependencies}
  CMAKE_SUBDIRS tests
)

target_include_directories(
  hpx_version PRIVATE $<BUILD_INTERFACE:${HPX_BINARY_DIR}>
)

# report in the MPI version whether the MPI library is CUDA-aware (checked in
# hpx_check_mpi_cuda_aware for builds with CUDA)
if(DEFINED HPX_MPI_CUDA_AWARE)
  target_compile_definitions(
    hpx_version
    PRIVATE HPX_VERSION_MPI_CUDA_AWARE=$<BOOL:${HPX_MPI_CUDA_AWARE}>
  )
endif()
