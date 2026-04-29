# Copyright (c) 2021 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    bulk_then_execute_3182
    bulk_sync_wait
    fork_join_with_result
    future_then_async_executor
    parallel_executor_1781
    pu_count_6184
    wrapping_executor
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  list(REMOVE_ITEM tests bulk_sync_wait)
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
    FOLDER "Tests/Regressions/Modules/Core/Executors"
  )

  add_hpx_regression_test("modules.executors" ${test} ${${test}_PARAMETERS})
endforeach()

if(HPX_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests)
  if(HPX_WITH_CUDA OR HPX_WITH_HIP)
    list(APPEND compile_tests service_executor_cuda)
    set(service_executor_cuda_CUDA ON)
  endif()

  foreach(compile_test ${compile_tests})
    if(${${compile_test}_CUDA})
      set(sources ${compile_test}.cu)
    else()
      set(sources ${compile_test}.cpp)
    endif()

    source_group("Source Files" FILES ${sources})

    add_hpx_regression_compile_test(
      "modules.executors" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Regressions/Modules/Core/Executors/CompileOnly"
    )
  endforeach()
endif()
