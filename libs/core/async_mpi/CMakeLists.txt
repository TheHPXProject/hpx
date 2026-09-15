# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Note: HPX_WITH_ASYNC_MPI is handled in the main CMakeLists.txt

# if the user does not want mpi async futures, quit - the module will not be
# enabled
if(NOT ${HPX_WITH_ASYNC_MPI})
  return()
endif()

hpx_add_config_define(HPX_HAVE_ASYNC_MPI)

# setup MPI, if necessary
if(NOT TARGET Mpi::mpi)
  include(HPX_SetupMPI)
  hpx_setup_mpi()
endif()

# Default location is $HPX_ROOT/libs/mpi/include
set(async_mpi_headers
    hpx/async_mpi/mpi_exception.hpp hpx/async_mpi/mpi_executor.hpp
    hpx/async_mpi/mpi_future.hpp hpx/async_mpi/transform_mpi.hpp
)

# Default location is $HPX_ROOT/libs/mpi/src
set(mpi_sources mpi_future.cpp)

include(HPX_AddModule)
add_hpx_module(
  core async_mpi
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${mpi_sources}
  HEADERS ${async_mpi_headers}
  DEPENDENCIES Mpi::mpi
  MODULE_DEPENDENCIES
    hpx_concepts
    hpx_concurrency
    hpx_config
    hpx_datastructures
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_futures
    hpx_memory
    hpx_mpi_base
    hpx_threading_base
    hpx_runtime_local
    hpx_synchronization
  CMAKE_SUBDIRS examples tests
)
