# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests mpi_ring_async_executor algorithm_transform_mpi)

set(mpi_ring_async_executor_PARAMETERS THREADS_PER_LOCALITY 4 LOCALITIES 2
                                       RUNWRAPPER mpi
)

set(algorithm_transform_mpi_PARAMETERS LOCALITIES 2 RUNWRAPPER mpi)
set(algorithm_transform_mpi_DEPENDENCIES hpx_execution_test_utilities)

foreach(test ${tests})

  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    DEPENDENCIES Mpi::mpi ${${test}_DEPENDENCIES}
    FOLDER "Tests/Unit/Modules/Core/AsyncMPI"
  )

  add_hpx_unit_test("modules.async_mpi" ${test} ${${test}_PARAMETERS})

endforeach()

if(HPX_WITH_COMPILE_ONLY_TESTS)
  # Negative compile-only tests that exercise the static_asserts at the top of
  # detail::set_value_request_callback_{void,non_void}. Each test passes a
  # deliberately-wrong receiver shape and is expected to fail compilation with
  # the assert's diagnostic message.
  set(compile_tests)

  if(HPX_WITH_FAIL_COMPILE_TESTS)
    set(fail_compile_tests fail_compile_set_value_request_callback_void
                           fail_compile_set_value_request_callback_non_void
    )
    foreach(fail_compile_test ${fail_compile_tests})
      set(${fail_compile_test}_FLAGS FAILURE_EXPECTED)
    endforeach()

    set(compile_tests ${compile_tests} ${fail_compile_tests})
  endif()

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    add_hpx_unit_compile_test(
      "modules.async_mpi" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      DEPENDENCIES Mpi::mpi
      FOLDER "Tests/Unit/Modules/Core/AsyncMPI/CompileOnly"
    )

  endforeach()

endif()
