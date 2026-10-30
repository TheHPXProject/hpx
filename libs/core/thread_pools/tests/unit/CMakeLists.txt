# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests bound_thread_yield create_background_thread stop_background_thread)

set(bound_thread_yield_PARAMETERS THREADS_PER_LOCALITY 4)

# needs a second worker to create a background thread for
set(create_background_thread_PARAMETERS THREADS_PER_LOCALITY 2)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/ThreadPools"
  )

  add_hpx_unit_test("modules.thread_pools" ${test} ${${test}_PARAMETERS})
endforeach()
