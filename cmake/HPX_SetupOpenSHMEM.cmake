# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)
include(${CMAKE_CURRENT_LIST_DIR}/FindOpenSHMEM.cmake)

macro(hpx_setup_openshmem)
  if(NOT TARGET OpenSHMEM::openshmem)

    # When this macro runs at HPX build time the OpenSHMEM location is
    # detected through pkg-config (oshmem-c/oshmem/oshmem-cxx) and the
    # resulting values are cached.  When it runs on a consumer
    # (find_package(HPX)) the cached values are reused as-is, so the consumer
    # needs neither pkg-config nor the oshmem module files themselves.
    if(NOT HPX_OPENSHMEM_LIBRARIES)

      find_openshmem()

      if(NOT OpenSHMEM_FOUND)
        hpx_error("OpenSHMEM was not found")
      endif()

      if(COMMAND hpx_add_config_define)
        hpx_add_config_define(HPX_HAVE_PARCELPORT_OPENSHMEM)
      endif()

      set(HPX_OPENSHMEM_LIBRARIES
          "${OSHMEM_LIBRARIES}"
          CACHE INTERNAL "OpenSHMEM libraries" FORCE
      )
      set(HPX_OPENSHMEM_INCLUDE_DIRS
          "${OSHMEM_INCLUDE_DIRS}"
          CACHE INTERNAL "OpenSHMEM include directories" FORCE
      )
    endif()

    add_library(OpenSHMEM::openshmem INTERFACE IMPORTED GLOBAL)
    set_target_properties(
      OpenSHMEM::openshmem
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                 "${HPX_OPENSHMEM_INCLUDE_DIRS}"
                 INTERFACE_LINK_LIBRARIES
                 "${HPX_OPENSHMEM_LIBRARIES}"
    )

    hpx_info("OpenSHMEM libraries: ${HPX_OPENSHMEM_LIBRARIES}")
    hpx_info("OpenSHMEM include dirs: ${HPX_OPENSHMEM_INCLUDE_DIRS}")
  endif()
endmacro()
