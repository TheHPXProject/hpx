# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests thread_id_type)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL NOLIBS
    DEPENDENCIES hpx_core
    FOLDER "Tests/Unit/Modules/Core/Coroutines"
  )

  add_hpx_unit_test("modules.coroutines" ${test} ${${test}_PARAMETERS})
  target_compile_definitions(${test}_test PRIVATE -DHPX_MODULE_STATIC_LINKING)
endforeach()
