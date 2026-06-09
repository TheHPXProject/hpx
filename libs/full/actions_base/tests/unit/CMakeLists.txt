# Copyright (c) 2026 Priyanshi Sharma
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests)

if(HPX_WITH_CXX26_REFLECTION)
  set(tests ${tests} reflect_action_test reflect_component_action_test)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)
  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL NOLIBS
    DEPENDENCIES hpx_core
    FOLDER "Tests/Unit/Modules/Full/ActionsBase"
  )

  add_hpx_unit_test("modules.actions_base" ${test} ${${test}_PARAMETERS})

  target_compile_definitions(${test}_test PRIVATE -DHPX_MODULE_STATIC_LINKING)
endforeach()
