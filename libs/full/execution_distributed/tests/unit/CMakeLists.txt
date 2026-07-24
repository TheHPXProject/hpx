# Copyright (c) 2020-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_NETWORKING)
  return()
endif()

set(tests distributed_scheduler)

set(distributed_scheduler_PARAMETERS LOCALITIES 2)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Full/ExecutionDistributed")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test(
    "modules.execution_distributed" ${test} ${${test}_PARAMETERS}
  )
endforeach()
