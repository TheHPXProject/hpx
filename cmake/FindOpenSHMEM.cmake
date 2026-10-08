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

  # Open MPI's oshmem moves data through its `osc/ucp` component, so a
  # UCX-enabled Open MPI is required for the transport to work at all.  The UCX
  # libraries only appear in pkg-config's `Libs.private` field; the
  # pkg_check_modules call above exposes those through the generated
  # <prefix>_STATIC_* results, which are used here for inspection only.  The
  # link line derived from the static query is intentionally not used: it drags
  # in Open MPI's own private dependencies (libevent, hwloc, pmix) that would
  # clash with HPX's copies.
  if(NOT DEFINED HPX_OPENSHMEM_REQUIRE_UCX)
    set(HPX_OPENSHMEM_REQUIRE_UCX
        FALSE
        CACHE BOOL
              "Fail the OpenSHMEM detection if UCX was not enabled in Open MPI"
    )
  endif()

  set(OpenSHMEM_UCX_FOUND FALSE)
  set(OpenSHMEM_UCX_LIBRARIES "")
  set(OpenSHMEM_UCX_LIBRARY_DIRS "")

  # An oshmem module built with `--with-ucx=...` records -lucp -luct -lucs -lucm
  # in `Libs.private`.  Match the library names rather than directory names so
  # that a UCX installed in a non-standard location still matches.
  set(_oshmem_ucx_libs "")
  foreach(_lib IN LISTS OSHMEM_STATIC_LIBRARIES)
    if(_lib MATCHES "^(ucp|uct|ucs|ucm)$")
      list(APPEND _oshmem_ucx_libs "${_lib}")
    endif()
  endforeach()

  if(_oshmem_ucx_libs)
    set(OpenSHMEM_UCX_FOUND TRUE)

    # Resolve each flag to the same directory Open MPI recorded, so that the
    # linker cannot silently satisfy -lucp from a different UCX that happened to
    # sit on the default search path.
    foreach(_lib IN LISTS _oshmem_ucx_libs)
      set(_ucx_lib "")
      foreach(_dir IN LISTS OSHMEM_STATIC_LIBRARY_DIRS)
        if(EXISTS "${_dir}/lib${_lib}${CMAKE_SHARED_LIBRARY_SUFFIX}")
          set(_ucx_lib "${_dir}/lib${_lib}${CMAKE_SHARED_LIBRARY_SUFFIX}")
          list(APPEND OpenSHMEM_UCX_LIBRARY_DIRS "${_dir}")
          break()
        endif()
      endforeach()

      if(_ucx_lib)
        list(APPEND OpenSHMEM_UCX_LIBRARIES "${_ucx_lib}")
      else()
        # No matching file next to the other Open MPI libraries; fall back to
        # the plain -l flag and let the linker resolve it.
        list(APPEND OpenSHMEM_UCX_LIBRARIES "-l${_lib}")
      endif()
    endforeach()

    if(OpenSHMEM_UCX_LIBRARY_DIRS)
      list(REMOVE_DUPLICATES OpenSHMEM_UCX_LIBRARY_DIRS)
    endif()

    hpx_info("OpenSHMEM: UCX detected in Open MPI's private libraries: "
             "${OpenSHMEM_UCX_LIBRARIES}"
    )
  else()
    set(_oshmem_ucx_hint
        "OpenSHMEM was found, but UCX does not appear in the private "
        "libraries of '${OpenSHMEM_PKG}.pc'.  Open MPI was most likely built "
        "without '--with-ucx', in which case it has no 'osc/ucp' component "
        "and Open SHMEM data movement will not work as expected."
    )

    if(HPX_OPENSHMEM_REQUIRE_UCX)
      hpx_error("${_oshmem_ucx_hint}")
    else()
      hpx_warn("${_oshmem_ucx_hint}")
      hpx_warn(
        "Set '-DHPX_OPENSHMEM_REQUIRE_UCX=ON' to turn this into a hard error."
      )
    endif()
  endif()

  message(STATUS "Found OpenSHMEM (${OpenSHMEM_PKG}): ${OSHMEM_INCLUDE_DIRS}")
endmacro()
