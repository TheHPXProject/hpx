# Copyright (c)      2018 Mikael Simberg
# Copyright (c) 2007-2025 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests spmd_block task_block task_block_executor task_block_par task_group)

# MacOS doesn't support binding threads to cores, so run_on_all would fail
if(NOT APPLE)
  set(tests ${tests} run_on_all)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Algorithms/Block"
  )

  add_hpx_unit_test("modules.algorithms.block" ${test} ${${test}_PARAMETERS})
endforeach()

add_hpx_unit_test(
  "modules.algorithms.block" spmd_block_one_worker
  EXECUTABLE spmd_block
  PSEUDO_DEPS_NAME spmd_block
  THREADS_PER_LOCALITY 1
)
