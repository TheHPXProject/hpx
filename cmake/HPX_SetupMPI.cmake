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
    add_library(Mpi::mpi INTERFACE IMPORTED GLOBAL)
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

# Sets <result> if the MPI library was built against the library <library> (e.g.
# cudart). MPICH's compiler wrapper lists the libraries MPICH was built against
# when asked for the link line of a static link (-static-mpi -link-info),
# whether the MPI library itself is shared or static and without a GPU. The
# wrapper is MPI_CXX_COMPILER, or CMAKE_CXX_COMPILER if that is the wrapper.
function(hpx_mpi_needs_library result library)
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
    hpx_info(
      "Warning: could not ask the MPI compiler wrapper ${wrapper} for its"
      "libraries (${failed}), MPICH is assumed not to be GPU-aware; set"
      "MPI_CXX_COMPILER to MPICH's mpicxx"
    )
  elseif(link_line MATCHES "(^|[ \t])-l${library}(_static)?([ \t\n]|$)"
         OR link_line MATCHES "/lib${library}(_static)?\\.(so|a)"
  )
    set(${result}
        ON
        PARENT_SCOPE
    )
  endif()
endfunction()

# Checks whether the MPI library was built with support for the GPU backend
# <gpu> (CUDA or HIP) and sets (cached):
#
# * HPX_MPI_<gpu>_AWARE: ON if it was, else OFF,
# * HPX_MPI_<gpu>_QUERY: the function that tells at runtime whether the
#   transport in use handles device memory.
#
# Open MPI: mpi-ext.h provides MPIX_CUDA_AWARE_SUPPORT and
# MPIX_Query_cuda_support, or MPIX_ROCM_AWARE_SUPPORT and
# MPIX_Query_rocm_support.
#
# MPICH (4.1 and newer): mpi.h declares MPIX_Query_cuda_support and
# MPIX_Query_hip_support whether or not the library was built with GPU support,
# and the query answers 1 only after MPI_Init with a GPU visible. A library
# built with CUDA or HIP is built against its runtime (cudart, amdhip64), which
# MPICH's compiler wrapper lists (see hpx_mpi_needs_library).
function(hpx_check_mpi_gpu_aware gpu)
  if(gpu STREQUAL "CUDA")
    set(openmpi_macro MPIX_CUDA_AWARE_SUPPORT)
    set(openmpi_query MPIX_Query_cuda_support)
    set(mpich_macro MPIX_GPU_SUPPORT_CUDA)
    set(mpich_query MPIX_Query_cuda_support)
    set(runtime cudart)
    set(name CUDA)
  elseif(gpu STREQUAL "HIP")
    set(openmpi_macro MPIX_ROCM_AWARE_SUPPORT)
    set(openmpi_query MPIX_Query_rocm_support)
    set(mpich_macro MPIX_GPU_SUPPORT_HIP)
    set(mpich_query MPIX_Query_hip_support)
    set(runtime amdhip64)
    set(name ROCm)
  else()
    hpx_error("hpx_check_mpi_gpu_aware: unknown GPU backend ${gpu}")
  endif()

  include(CheckCXXSourceCompiles)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_LIBRARIES Mpi::mpi)
  set(CMAKE_REQUIRED_QUIET ON)

  # checked again in every configuration: the MPI library may have changed
  unset(HPX_MPI_${gpu}_AWARE_OPENMPI CACHE)
  unset(HPX_MPI_HAVE_MPICH_${gpu}_QUERY CACHE)

  set(query)
  check_cxx_source_compiles(
    "#include <mpi.h>
#include <mpi-ext.h>
#if !defined(${openmpi_macro}) || !${openmpi_macro}
#error MPI library not built with ${name} support
#endif
int main() { return ${openmpi_query}(); }"
    HPX_MPI_${gpu}_AWARE_OPENMPI
  )
  if(HPX_MPI_${gpu}_AWARE_OPENMPI)
    set(query ${openmpi_query})
  else()
    check_cxx_source_compiles(
      "#include <mpi.h>
#if !defined(MPICH_NUMVERSION) || !defined(${mpich_macro})
#error not an MPICH with GPU support queries
#endif
int main() { return ${mpich_query}(); }"
      HPX_MPI_HAVE_MPICH_${gpu}_QUERY
    )
    if(HPX_MPI_HAVE_MPICH_${gpu}_QUERY)
      hpx_mpi_needs_library(mpich_aware ${runtime})
      if(mpich_aware)
        set(query ${mpich_query})
      endif()
    endif()
  endif()
  cmake_pop_check_state()

  if(query)
    set(HPX_MPI_${gpu}_AWARE
        ON
        CACHE INTERNAL ""
    )
    hpx_info("MPI library is ${name}-aware")
  else()
    set(HPX_MPI_${gpu}_AWARE
        OFF
        CACHE INTERNAL ""
    )
    hpx_info("MPI library is not ${name}-aware")
  endif()
  set(HPX_MPI_${gpu}_QUERY
      "${query}"
      CACHE INTERNAL ""
  )
endfunction()
