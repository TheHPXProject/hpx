# Copyright (c) 2007-2026 Hartmut Kaiser
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    split_shared_future
    wait_all
    wait_all_for
    wait_all_std_array
    wait_any
    wait_any_std_array
    wait_each
    wait_some
    wait_some_std_array
    when_all
    when_all_std_array
    when_any
    when_any_std_array
    when_each
    when_some
    when_some_std_array
)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/AsyncCombinators")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.async_combinators" ${test} ${${test}_PARAMETERS})
endforeach()
