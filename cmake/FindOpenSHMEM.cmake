# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

macro(find_openshmem)
  # Only Open MPI's OpenSHMEM is supported. The package is located through
  # pkg-config (pkg_check_modules uses PKG_CONFIG_PATH and the standard install
  # locations), i.e. via the oshmem-c.pc, oshmem.pc or oshmem-cxx.pc module
  # files. oshmem-c/oshmem are preferred because they carry the -loshmem link
  # flag.
  find_package(PkgConfig QUIET)
  if(NOT PKG_CONFIG_FOUND)
    hpx_error("pkg-config was not found; OpenSHMEM detection requires it")
  endif()

  set(_oshmem_pkg_names oshmem-c oshmem oshmem-cxx)
  foreach(_pkg ${_oshmem_pkg_names})
    pkg_check_modules(OSHMEM QUIET IMPORTED_TARGET GLOBAL ${_pkg})
    if(OSHMEM_FOUND)
      set(OpenSHMEM_PKG ${_pkg})
      break()
    endif()
  endforeach()

  set(OpenSHMEM_FOUND ${OSHMEM_FOUND})

  if(NOT OpenSHMEM_FOUND)
    hpx_error(
      "Could not find Open MPI OpenSHMEM. Tried: ${_oshmem_pkg_names}. "
      "Please set PKG_CONFIG_PATH to the directory containing the oshmem.pc "
      "module files."
    )
  endif()

  if(NOT OSHMEM_INCLUDE_DIRS OR NOT (OSHMEM_LIBRARY_DIRS OR OSHMEM_LIBRARIES))
    hpx_error(
      "Could not find OSHMEM_INCLUDE_DIRS or OSHMEM_LIBRARIES/OSHMEM_LIBRARY_DIRS"
    )
  endif()

  if(NOT TARGET OpenSHMEM::openshmem)
    add_library(OpenSHMEM::openshmem INTERFACE IMPORTED)
    set_target_properties(
      OpenSHMEM::openshmem
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OSHMEM_INCLUDE_DIRS}"
                 INTERFACE_LINK_LIBRARIES "${OSHMEM_LIBRARIES}"
    )
  endif()

  message(STATUS "Found OpenSHMEM (${OpenSHMEM_PKG}): ${OSHMEM_INCLUDE_DIRS}")
endmacro()
