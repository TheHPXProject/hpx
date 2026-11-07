# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests schedule_timer)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    DEPENDENCIES hpx_io_service
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Unit/Modules/Core/IOService"
  )

  add_hpx_unit_test("modules.io_service" ${test})
endforeach()
