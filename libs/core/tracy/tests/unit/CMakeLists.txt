# Copyright (c) 2026 The STE||AR Group
# Copyright (c) 2026 Vansh Dobhal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests fiber_name)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Tracy"
  )

  add_hpx_unit_test("modules.tracy" ${test})
endforeach()

if(HPX_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests compile_active_tracy)

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    add_hpx_unit_compile_test(
      "modules.tracy" ${compile_test}
      SOURCES ${sources}
      FOLDER "Tests/Unit/Modules/Core/Tracy/CompileOnly"
    )
  endforeach()
endif()
