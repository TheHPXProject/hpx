# Copyright (c) 2020-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests ini_escape long_lines)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/Ini")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL NOLIBS
    DEPENDENCIES hpx_core
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.ini" ${test} ${${test}_PARAMETERS})
endforeach()
