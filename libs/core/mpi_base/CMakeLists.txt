# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if((HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI) OR HPX_WITH_ASYNC_MPI)
  # setup MPI, if necessary
  include(HPX_SetupMPI)
  hpx_setup_mpi()
  set(additional_dependencies Mpi::mpi)
else()
  return()
endif()

# Default location is $HPX_ROOT/libs/mpi_base/include
set(mpi_base_headers hpx/mpi_base/mpi.hpp hpx/mpi_base/mpi_environment.hpp)

# Default location is $HPX_ROOT/libs/mpi_base/include_compatibility
# cmake-format: off
set(mpi_base_compat_headers
    hpx/plugins/parcelport/mpi/mpi.hpp => hpx/modules/mpi_base.hpp
    hpx/plugins/parcelport/mpi/mpi_environment.hpp => hpx/modules/mpi_base.hpp
)
# cmake-format: on

set(mpi_base_sources mpi_environment.cpp)

include(HPX_AddModule)
add_hpx_module(
  core mpi_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${mpi_base_sources}
  HEADERS ${mpi_base_headers}
  COMPAT_HEADERS ${mpi_base_compat_headers}
  MODULE_DEPENDENCIES
    hpx_concepts
    hpx_errors
    hpx_format
    hpx_logging
    hpx_runtime_configuration
    hpx_string_util
    hpx_synchronization
    hpx_util
  DEPENDENCIES ${additional_dependencies}
  CMAKE_SUBDIRS examples tests
)

# report in the MPI version whether the MPI library supports the GPU backend
# (checked in hpx_check_mpi_gpu_aware for builds with CUDA or HIP)
if(DEFINED HPX_MPI_CUDA_AWARE)
  target_compile_definitions(
    hpx_mpi_base
    PRIVATE HPX_VERSION_MPI_CUDA_AWARE=$<BOOL:${HPX_MPI_CUDA_AWARE}>
  )
endif()
if(DEFINED HPX_MPI_HIP_AWARE)
  target_compile_definitions(
    hpx_mpi_base PRIVATE HPX_VERSION_MPI_HIP_AWARE=$<BOOL:${HPX_MPI_HIP_AWARE}>
  )
endif()
