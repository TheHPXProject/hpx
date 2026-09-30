# Copyright (c) 2019-2022 Ste||ar Group
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
