# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests static_component_registry static_plugin_registry)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL NOLIBS
    DEPENDENCIES hpx_runtime_configuration hpx_testing
    FOLDER "Tests/Unit/Modules/Core/RuntimeConfiguration"
  )

  add_hpx_unit_test(
    "modules.runtime_configuration" ${test} ${${test}_PARAMETERS}
  )
endforeach()
