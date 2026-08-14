# Copyright (c) 2020-2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests pool_timer thread_mapper termination_detection)

set(pool_timer_PARAMETERS THREADS_PER_LOCALITY 4)
set(thread_mapper_PARAMETERS THREADS_PER_LOCALITY 4)
set(termination_detection_PARAMETERS THREADS_PER_LOCALITY 4)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/RuntimeLocal"
  )

  add_hpx_unit_test("modules.runtime_local" ${test} ${${test}_PARAMETERS})

endforeach()
