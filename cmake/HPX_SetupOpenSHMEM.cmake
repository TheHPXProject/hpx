# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)
include(${CMAKE_CURRENT_LIST_DIR}/FindOpenSHMEM.cmake)

macro(hpx_setup_openshmem)
  if(NOT TARGET OpenSHMEM::openshmem)
    find_openshmem()

    if(NOT OpenSHMEM_FOUND)
      hpx_error("OpenSHMEM was not found")
    endif()

    hpx_add_config_define(HPX_HAVE_PARCELPORT_OPENSHMEM)

    set(HPX_OPENSHMEM_LIBRARIES
        "${OSHMEM_LIBRARIES}"
        CACHE INTERNAL "OpenSHMEM libraries" FORCE
    )
    set(HPX_OPENSHMEM_INCLUDE_DIRS
        "${OSHMEM_INCLUDE_DIRS}"
        CACHE INTERNAL "OpenSHMEM include directories" FORCE
    )

    hpx_info("OpenSHMEM libraries: ${OSHMEM_LIBRARIES}")
    hpx_info("OpenSHMEM include dirs: ${OSHMEM_INCLUDE_DIRS}")
  endif()
endmacro()
