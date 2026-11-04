# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    supervision_api
    supervision_await_terminal
    supervision_await_terminal_shutdown
    supervision_check_activity
    supervision_check_admission
    supervision_observer_disconnected
    supervision_remove_target
    supervision_tidy_remote_shutdown
    supervision_tidy_teardown
)

set(supervision_api_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 4)
set(supervision_await_terminal_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 4)
set(supervision_await_terminal_shutdown_PARAMETERS THREADS_PER_LOCALITY 4)
set(supervision_check_activity_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 4)
set(supervision_check_admission_PARAMETERS THREADS_PER_LOCALITY 4)
set(supervision_observer_disconnected_PARAMETERS LOCALITIES 2
                                                 THREADS_PER_LOCALITY 4
)
set(supervision_remove_target_PARAMETERS THREADS_PER_LOCALITY 4)
set(supervision_tidy_remote_shutdown_PARAMETERS
    LOCALITIES 2 THREADS_PER_LOCALITY 4 TIMEOUT 120
)
set(supervision_tidy_teardown_PARAMETERS THREADS_PER_LOCALITY 4)

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP)
  set(tests ${tests} supervision_observer_force_disconnect)

  # add executable needed for the supervision_observer_force_disconnect test
  add_hpx_executable(
    supervision_observer_force_disconnect_worker INTERNAL_FLAGS
    SOURCES supervision_observer_force_disconnect_worker.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/Supervision"
  )

  # cmake-format: off
  set(supervision_observer_force_disconnect_PARAMETERS
      RUN_SERIAL
      THREADS_PER_LOCALITY 4
      --launch=$<TARGET_FILE:supervision_observer_force_disconnect_worker>
      # Force use of the TCP parcelport
      --hpx:ini=hpx.parcel.tcp.priority=1000
      --hpx:ini=hpx.parcel.bootstrap=tcp
  )
  set(supervision_observer_force_disconnect_FLAGS
      DEPENDENCIES process_component
  )
  # cmake-format: on
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    HEADERS "supervision_test_helpers.hpp"
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/Supervision"
  )

  add_hpx_unit_test("modules.supervision" ${test} ${${test}_PARAMETERS})

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

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP)
  # MSVC V1951 and V1952 expose ICE compiling this in C++20 module mode
  if(HPX_WITH_CXX_MODULES
     AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     AND (MSVC_VERSION LESS_EQUAL 1952)
  )
    target_compile_definitions(
      supervision_observer_force_disconnect_worker
      PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
    )
  endif()

  add_hpx_pseudo_dependencies(
    tests.unit.modules.supervision.supervision_observer_force_disconnect
    supervision_observer_force_disconnect_worker
  )
endif()
