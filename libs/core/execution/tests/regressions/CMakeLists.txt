# Copyright (c) 2014-2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests annotated_minmax_2593 chunk_size_4118 executor_parameters_num_chunks
          is_executor_1691 fibonacci_sr
)

if(HPX_WITH_DATAPAR)
  set(tests ${tests} lambda_arguments_2403 lambda_return_type_2402
            rebind_par_simd
  )
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/Execution"
  )

  add_hpx_regression_test("modules.execution" ${test} ${${test}_PARAMETERS})

endforeach()
