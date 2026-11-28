# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Find NVIDIA's NCCL library. Hints: NCCL_ROOT, NCCL_DIR, the environment
# variables NCCL_ROOT and NCCL_HOME, and the CUDA toolkit directory. Sets
# NCCL_FOUND, NCCL_VERSION (from nccl.h), NCCL_INCLUDE_DIR, NCCL_LIBRARY and
# defines the imported target NCCL::nccl.

if(NOT TARGET NCCL::nccl)

  find_path(
    NCCL_INCLUDE_DIR nccl.h
    HINTS ${NCCL_ROOT}
          ${NCCL_DIR}
          ENV
          NCCL_ROOT
          ENV
          NCCL_HOME
          ${CUDAToolkit_INCLUDE_DIRS}
    PATH_SUFFIXES include
  )

  find_library(
    NCCL_LIBRARY
    NAMES nccl
    HINTS ${NCCL_ROOT}
          ${NCCL_DIR}
          ENV
          NCCL_ROOT
          ENV
          NCCL_HOME
          ${CUDAToolkit_LIBRARY_DIR}
    PATH_SUFFIXES lib lib64
  )

  if(NCCL_INCLUDE_DIR AND EXISTS "${NCCL_INCLUDE_DIR}/nccl.h")
    file(STRINGS "${NCCL_INCLUDE_DIR}/nccl.h" _nccl_version_lines
         REGEX "#define NCCL_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+"
    )
    foreach(_part MAJOR MINOR PATCH)
      string(REGEX MATCH "NCCL_${_part}[ \t]+([0-9]+)" _match
                   "${_nccl_version_lines}"
      )
      set(NCCL_VERSION_${_part} "${CMAKE_MATCH_1}")
    endforeach()
    set(NCCL_VERSION
        "${NCCL_VERSION_MAJOR}.${NCCL_VERSION_MINOR}.${NCCL_VERSION_PATCH}"
    )
    unset(_nccl_version_lines)
    unset(_match)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    NCCL
    REQUIRED_VARS NCCL_LIBRARY NCCL_INCLUDE_DIR
    VERSION_VAR NCCL_VERSION
  )

  if(NCCL_FOUND)
    add_library(NCCL::nccl INTERFACE IMPORTED GLOBAL)
    target_include_directories(NCCL::nccl SYSTEM INTERFACE ${NCCL_INCLUDE_DIR})
    target_link_libraries(NCCL::nccl INTERFACE ${NCCL_LIBRARY})
  endif()

  mark_as_advanced(NCCL_INCLUDE_DIR NCCL_LIBRARY)

endif()
