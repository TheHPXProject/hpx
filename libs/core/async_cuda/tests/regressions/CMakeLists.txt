# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests first_device_use_small_stack)

set(first_device_use_small_stack_PARAMETERS THREADS_PER_LOCALITY 2)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/AsyncCuda"
  )

  add_hpx_regression_test(
    "modules.async_cuda" ${test} ${${test}_PARAMETERS} RUN_SERIAL
  )
endforeach()

if(HPX_WITH_HIP)
  # all devices warmed up
  add_hpx_regression_test(
    "modules.async_cuda" first_device_use_small_stack_all
    EXECUTABLE first_device_use_small_stack
    PSEUDO_DEPS_NAME first_device_use_small_stack
                     ${first_device_use_small_stack_PARAMETERS}
    RUN_SERIAL
    ARGS --hpx:ini=hpx.hip.warmup=all
  )
endif()
