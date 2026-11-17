# Copyright (c) 2019-2022 Ste||ar Group
# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)

macro(hpx_setup_mpi)

  if(NOT TARGET Mpi::mpi)

    find_package(MPI REQUIRED QUIET COMPONENTS CXX)
    add_library(Mpi::mpi INTERFACE IMPORTED)
    target_link_libraries(Mpi::mpi INTERFACE MPI::MPI_CXX)

    # Ensure compatibility with older versions
    if(MPI_LIBRARY)
      target_link_libraries(Mpi::mpi INTERFACE ${MPI_LIBRARY})
    endif()
    if(MPI_EXTRA_LIBRARY)
      target_link_libraries(Mpi::mpi INTERFACE ${MPI_EXTRA_LIBRARY})
    endif()

    hpx_info("MPI version: " ${MPI_CXX_VERSION})

  endif()

endmacro()

# Sets HPX_MPI_CUDA_AWARE (cached) if the MPI library was built with CUDA
# support, i.e. Open MPI's mpi-ext.h provides MPIX_CUDA_AWARE_SUPPORT and
# MPIX_Query_cuda_support. Whether the transport in use handles device memory is
# decided at runtime by MPIX_Query_cuda_support().
function(hpx_check_mpi_cuda_aware)
  include(CheckCXXSourceCompiles)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_LIBRARIES Mpi::mpi)
  set(CMAKE_REQUIRED_QUIET ON)
  check_cxx_source_compiles(
    "#include <mpi.h>
#include <mpi-ext.h>
#if !defined(MPIX_CUDA_AWARE_SUPPORT) || !MPIX_CUDA_AWARE_SUPPORT
#error MPI library not built with CUDA support
#endif
int main() { return MPIX_Query_cuda_support(); }"
    HPX_MPI_CUDA_AWARE
  )
  cmake_pop_check_state()

  if(HPX_MPI_CUDA_AWARE)
    hpx_info("MPI library is CUDA-aware")
  else()
    hpx_info("MPI library is not CUDA-aware (no MPIX_Query_cuda_support)")
  endif()
endfunction()
