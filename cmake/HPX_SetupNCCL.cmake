# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)

# Finds the library of the NCCL device conduit: NCCL with CUDA, RCCL (the same
# API) with HIP, and defines HPX::nccl for it. Non-blocking communicators and
# ncclGetLastError need NCCL 2.14 or newer (RCCL of ROCm 5.4 or newer).
macro(hpx_setup_nccl)
  if(NOT TARGET HPX::nccl)
    if(HPX_WITH_HIP)
      # RCCL_ROOT (the module of the CI stack sets it) or the ROCm directory
      find_package(
        rccl
        REQUIRED
        CONFIG
        HINTS
        ${RCCL_ROOT}
        $ENV{RCCL_ROOT}
        ${ROCM_PATH}
        $ENV{ROCM_PATH}
        /opt/rocm
      )
      add_library(HPX::nccl INTERFACE IMPORTED GLOBAL)
      target_link_libraries(HPX::nccl INTERFACE roc::rccl)
      hpx_info("    RCCL ${rccl_VERSION} for the device conduit")
    elseif(HPX_WITH_CUDA)
      find_package(CUDAToolkit REQUIRED)
      find_package(NCCL 2.14 REQUIRED)
      add_library(HPX::nccl INTERFACE IMPORTED GLOBAL)
      target_link_libraries(HPX::nccl INTERFACE NCCL::nccl CUDA::cudart)
      hpx_info(
        "    NCCL ${NCCL_VERSION} for the device conduit: ${NCCL_LIBRARY}"
      )
    else()
      hpx_error(
        "The NCCL device conduit requires CUDA or HIP (HPX_WITH_CUDA=ON or HPX_WITH_HIP=ON)"
      )
    endif()
  endif()
endmacro()
