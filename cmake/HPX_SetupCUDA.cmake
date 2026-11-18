# Copyright (c) 2019-2025 Ste||ar-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_CUDA AND NOT TARGET Cuda::cuda)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(HPX_WITH_CLANG_CUDA ON)
  endif()

  # cuda_std_20 not recognized for previous versions
  cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

  # Check CUDA standard
  if(NOT DEFINED CMAKE_CUDA_STANDARD)
    if(DEFINED CMAKE_CXX_STANDARD)
      set(CMAKE_CUDA_STANDARD ${CMAKE_CXX_STANDARD})
    else()
      set(CMAKE_CUDA_STANDARD 20)
    endif()
  else()
    if(CMAKE_CUDA_STANDARD LESS 20)
      hpx_error(
        "You've set CMAKE_CUDA_STANDARD to ${CMAKE_CUDA_STANDARD}, which is less than 20 (the minimum required by HPX)"
      )
    endif()
  endif()
  set(CMAKE_CUDA_EXTENSIONS OFF)

  enable_language(CUDA)

  if(NOT HPX_FIND_PACKAGE)
    # The cmake variables are supposed to be cached no need to redefine them
    set(HPX_WITH_COMPUTE ON)
    hpx_add_config_define(HPX_HAVE_CUDA)
    hpx_add_config_define(HPX_HAVE_COMPUTE)
  endif()

  # CUDA libraries used
  add_library(Cuda::cuda INTERFACE IMPORTED)
  # Toolkit targets like CUDA::cudart, CUDA::cublas, CUDA::cufft, etc. available
  find_package(CUDAToolkit MODULE REQUIRED)
  if(CUDAToolkit_FOUND)
    target_link_libraries(Cuda::cuda INTERFACE CUDA::cudart)
    if(TARGET CUDA::cublas)
      set(HPX_WITH_GPUBLAS ON)
      hpx_add_config_define(HPX_HAVE_GPUBLAS)
      target_link_libraries(Cuda::cuda INTERFACE CUDA::cublas)
    else()
      set(HPX_WITH_GPUBLAS OFF)
    endif()
  endif()
  # Flag not working for CLANG CUDA
  target_compile_features(Cuda::cuda INTERFACE cuda_std_${CMAKE_CUDA_STANDARD})
  set_target_properties(
    Cuda::cuda PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON
  )
  hpx_info("Using CUDA C++${CMAKE_CUDA_STANDARD}")

  if(NOT HPX_WITH_CLANG_CUDA)
    if(NOT MSVC)
      target_compile_options(
        Cuda::cuda INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-w>
      )
    else()
      # Windows

      # the CUDA headers (libcu++, used by stdexec) break if windows.h defines
      # min and max as macros; the compile definition reaches dependent targets
      # also if they include windows.h before any HPX header
      if(NOT HPX_FIND_PACKAGE)
        hpx_add_config_cond_define(NOMINMAX)
      endif()
      target_compile_definitions(Cuda::cuda INTERFACE NOMINMAX)
      set(CUDA_PROPAGATE_HOST_FLAGS OFF)
      target_compile_options(
        Cuda::cuda
        INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:$<$<CONFIG:Debug>:
                  -D_DEBUG
                  -O0
                  -g
                  -G
                  -Xcompiler=-MDd
                  -Xcompiler=-Od
                  -Xcompiler=-Zi
                  -Xcompiler=-bigobj
                  >>
      )
      target_compile_options(
        Cuda::cuda
        INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:$<$<CONFIG:RelWithDebInfo>:
                  -DNDEBUG
                  -O2
                  -g
                  -Xcompiler=-MD,-O2,-Zi
                  -Xcompiler=-bigobj
                  >>
      )
      target_compile_options(
        Cuda::cuda
        INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:$<$<CONFIG:MinSizeRel>: -DNDEBUG
                  -O1 -Xcompiler=-MD,-O1 -Xcompiler=-bigobj >>
      )
      target_compile_options(
        Cuda::cuda
        INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:$<$<CONFIG:Release>: -DNDEBUG -O2
                  -Xcompiler=-MD,-Ox -Xcompiler=-bigobj >>
      )
      # stdexec needs a conforming __cplusplus and preprocessor from the host
      # compiler too (see the corresponding CXX options in CMakeLists.txt)
      target_compile_options(
        Cuda::cuda
        INTERFACE $<$<COMPILE_LANGUAGE:CUDA>: -Xcompiler=-Zc:__cplusplus
                  -Xcompiler=-Zc:preprocessor >
      )
    endif()
    set(CUDA_SEPARABLE_COMPILATION ON)
    target_compile_options(
      Cuda::cuda
      INTERFACE $<$<COMPILE_LANGUAGE:CUDA>: --extended-lambda --default-stream
                per-thread --expt-relaxed-constexpr >
    )
    if(${CMAKE_CUDA_COMPILER_ID} STREQUAL "NVIDIA")
      target_compile_definitions(
        Cuda::cuda INTERFACE $<$<COMPILE_LANGUAGE:CUDA>: ASIO_DISABLE_CONSTEXPR
                             >
      )
    endif()
  else()
    if(NOT HPX_FIND_PACKAGE)
      hpx_add_target_compile_option(-DBOOST_THREAD_USES_MOVE PUBLIC)
    endif()
  endif()

  if(NOT HPX_FIND_PACKAGE)
    target_link_libraries(hpx_base_libraries INTERFACE Cuda::cuda)
  endif()
endif()
