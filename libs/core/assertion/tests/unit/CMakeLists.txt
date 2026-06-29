# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests assert_fail assert_succeed)

set(assert_fail_PARAMETERS FAILURE_EXPECTED)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Unit/Modules/Core/Assertion/"
  )

  add_hpx_unit_test("modules.assertion" ${test} ${${test}_PARAMETERS})
endforeach()
