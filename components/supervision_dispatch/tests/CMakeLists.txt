# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Option)

if(HPX_WITH_TESTS_UNIT)
  add_hpx_pseudo_target(tests.unit.components.supervision_dispatch)
  add_hpx_pseudo_dependencies(
    tests.unit.components tests.unit.components.supervision_dispatch
  )
  add_subdirectory(unit)
endif()

if(HPX_WITH_TESTS_REGRESSIONS)
  add_hpx_pseudo_target(tests.regressions.components.supervision_dispatch)
  add_hpx_pseudo_dependencies(
    tests.regressions.components
    tests.regressions.components.supervision_dispatch
  )
  add_subdirectory(regressions)
endif()

if(HPX_WITH_TESTS_BENCHMARKS)
  add_hpx_pseudo_target(tests.performance.components.supervision_dispatch)
  add_hpx_pseudo_dependencies(
    tests.performance.components
    tests.performance.components.supervision_dispatch
  )
  add_subdirectory(performance)
endif()

if(HPX_WITH_TESTS_HEADERS)
  add_hpx_header_tests(
    "components.supervision_dispatch"
    HEADERS ${supervision_dispatch_headers}
    HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    COMPONENT_DEPENDENCIES supervision_dispatch
  )
endif()
