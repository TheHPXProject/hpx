#  Copyright (c) 2022 Gregor Daiss
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests sycl_vector_add_get_future sycl_vector_add_executor
          sycl_vector_add_concurrent_executors sycl_stream
)

foreach(test ${tests})
  hpx_info("Building SYCL test ${test}_test")
  set(sources "${test}.cpp" "${additional_sources}")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    DEPENDENCIES ${${test_program}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/AsyncSYCL"
  )

  # make it compile with sycl
  if(HPX_WITH_HIPSYCL)
    add_sycl_to_target(TARGET ${test}_test SOURCES ${test}.cpp)
  else()
    separate_arguments(
      HPX_WITH_SYCL_FLAGS UNIX_COMMAND "${HPX_WITH_SYCL_FLAGS}"
    )
    target_compile_options(${test}_test PUBLIC -fsycl ${HPX_WITH_SYCL_FLAGS})
    target_link_options(${test}_test PUBLIC -fsycl ${HPX_WITH_SYCL_FLAGS})
  endif()
  # Test with more than one thread (important for conccurent sycl executor test,
  # does not hurt for other tests)
  set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)
  # Add to tests
  add_hpx_unit_test("modules.async_sycl" ${test} ${${test}_PARAMETERS})
endforeach()
