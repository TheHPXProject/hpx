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

# Sets <result> if the MPI library was built against the CUDA runtime (cudart).
# MPICH's compiler wrapper lists the libraries MPICH was built against when
# asked for the link line of a static link (-static-mpi -link-info), whether the
# MPI library itself is shared or static and without a GPU. The wrapper is
# MPI_CXX_COMPILER, or CMAKE_CXX_COMPILER if that is the wrapper.
function(hpx_mpi_needs_cudart result)
  set(${result}
      OFF
      PARENT_SCOPE
  )
  set(wrapper "${MPI_CXX_COMPILER}")
  if(NOT wrapper)
    set(wrapper "${CMAKE_CXX_COMPILER}")
  endif()
  execute_process(
    COMMAND "${wrapper}" -static-mpi -link-info
    RESULT_VARIABLE failed
    OUTPUT_VARIABLE link_line
    ERROR_QUIET
  )
  if(failed)
    hpx_warn("Could not ask the MPI compiler wrapper ${wrapper} for its"
             "libraries (${failed}), set MPI_CXX_COMPILER to MPICH's mpicxx"
    )
  elseif(link_line MATCHES "(^|[ \t])-lcudart(_static)?([ \t\n]|$)"
         OR link_line MATCHES "/libcudart(_static)?\\.(so|a)"
  )
    set(${result}
        ON
        PARENT_SCOPE
    )
  endif()
endfunction()

# Sets HPX_MPI_CUDA_AWARE (cached) if the MPI library was built with CUDA
# support:
#
# * Open MPI: mpi-ext.h provides MPIX_CUDA_AWARE_SUPPORT and
#   MPIX_Query_cuda_support.
# * MPICH (4.1 and newer): mpi.h declares MPIX_Query_cuda_support whether or not
#   the library was built with CUDA, and the query answers 1 only after MPI_Init
#   with a GPU visible. A library built with CUDA is built against the CUDA
#   runtime (cudart), which MPICH's compiler wrapper lists (see
#   hpx_mpi_needs_cudart).
#
# Whether the transport in use handles device memory is decided at runtime by
# MPIX_Query_cuda_support().
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
    HPX_MPI_CUDA_AWARE_OPENMPI
  )

  if(NOT HPX_MPI_CUDA_AWARE_OPENMPI)
    check_cxx_source_compiles(
      "#include <mpi.h>
#if !defined(MPICH_NUMVERSION) || !defined(MPIX_GPU_SUPPORT_CUDA)
#error not an MPICH with GPU support queries
#endif
int main() { return MPIX_Query_cuda_support(); }"
      HPX_MPI_HAVE_MPICH_CUDA_QUERY
    )
    if(HPX_MPI_HAVE_MPICH_CUDA_QUERY)
      hpx_mpi_needs_cudart(HPX_MPI_CUDA_AWARE_MPICH)
    endif()
  endif()
  cmake_pop_check_state()

  if(HPX_MPI_CUDA_AWARE_OPENMPI OR HPX_MPI_CUDA_AWARE_MPICH)
    set(HPX_MPI_CUDA_AWARE
        ON
        CACHE INTERNAL ""
    )
    hpx_info("MPI library is CUDA-aware")
  else()
    set(HPX_MPI_CUDA_AWARE
        OFF
        CACHE INTERNAL ""
    )
    hpx_info("MPI library is not CUDA-aware")
  endif()
endfunction()
