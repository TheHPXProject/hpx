# Copyright (c)      2020 ETH Zurich
# Copyright (c) 2007-2017 Hartmut Kaiser
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests apply_local apply_local_executor async_function_pointer async_local
          async_local_executor async_local_executor_additional_arguments
)

set(apply_local_PARAMETERS THREADS_PER_LOCALITY 4)
set(apply_local_executor_PARAMETERS THREADS_PER_LOCALITY 4)
set(async_local_PARAMETERS THREADS_PER_LOCALITY 4)
set(async_local_executor_PARAMETERS THREADS_PER_LOCALITY 4)
set(async_local_executor_additional_arguments_PARAMETERS THREADS_PER_LOCALITY 4)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/AsyncLocal")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.async_local" ${test} ${${test}_PARAMETERS})
endforeach()
