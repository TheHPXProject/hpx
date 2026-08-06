# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    symbol_name_registration
    discover_and_join
    discover_peers
    dispatch_api
    failure_detection
    failure_detection_join
    fan_out_join
    fenced_dispatch
    registry_join
    registry_snapshot
    sentinel_start
)

set(discover_and_join_PARAMETERS LOCALITIES 2)
set(discover_and_join_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(discover_peers_PARAMETERS LOCALITIES 2)
set(discover_peers_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(failure_detection_PARAMETERS LOCALITIES 2)
set(failure_detection_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(failure_detection_join_PARAMETERS LOCALITIES 2)
set(failure_detection_join_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(fan_out_join_PARAMETERS LOCALITIES 2)
set(fan_out_join_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(fenced_dispatch_PARAMETERS LOCALITIES 2)

foreach(test ${tests})
  set(sources ${test}.cpp)

  # add executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    COMPONENT_DEPENDENCIES supervision_dispatch
    FOLDER "Tests/Unit/Components/Supervision"
  )

  add_hpx_unit_test(
    "components.supervision_dispatch" ${test} ${${test}_PARAMETERS}
  )

  # MSVC V1951 and V1952 expose ICE compiling this in C++20 module mode
  if(HPX_WITH_CXX_MODULES
     AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     AND (MSVC_VERSION LESS_EQUAL 1952)
  )
    target_compile_definitions(
      ${test}_test PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
    )
  endif()
endforeach()
