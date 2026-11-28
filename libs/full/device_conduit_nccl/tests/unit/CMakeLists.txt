# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests device_memory stall)

set(device_memory_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 4 RUN_SERIAL)
set(stall_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 4 RUN_SERIAL)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/DeviceConduitNCCL"
  )

  # the CUDA (or HIP) runtime for the device memory of the test
  target_link_libraries(${test}_test PRIVATE HPX::nccl)

  add_hpx_unit_test("modules.device_conduit_nccl" ${test} ${${test}_PARAMETERS})
endforeach()
