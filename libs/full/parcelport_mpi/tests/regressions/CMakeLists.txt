# Copyright (c) 2020-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests)

if(HPX_WITH_ASYNC_MPI)
  list(APPEND tests user_polling_finalize_5919)
endif()

set(user_polling_finalize_5919_PARAMETERS LOCALITIES 2 PARCELPORTS mpi)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL
    DEPENDENCIES Mpi::mpi
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/ParcelportMPI"
  )

  add_hpx_regression_test(
    "modules.parcelport_mpi" ${test} ${${test}_PARAMETERS}
  )
endforeach()
