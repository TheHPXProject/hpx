# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    and_gate
    channel_local
    local_dataflow
    local_dataflow_small_vector
    local_dataflow_executor
    local_dataflow_external_future
    local_dataflow_executor_additional_arguments
    local_dataflow_std_array
    run_guarded
    split_future
)

set(and_gate_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_dataflow_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_dataflow_external_future_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_dataflow_executor_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_dataflow_executor_additional_arguments_PARAMETERS THREADS_PER_LOCALITY
                                                            4
)
set(run_guarded_PARAMETERS THREADS_PER_LOCALITY 4)

foreach(test ${tests})

  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/LocalLCOs"
  )

  add_hpx_unit_test("modules.lcos_local" ${test} ${${test}_PARAMETERS})

endforeach()
