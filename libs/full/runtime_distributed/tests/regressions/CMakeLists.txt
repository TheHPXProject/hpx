# Copyright (c) 2020-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests multiple_init multiple_init_2918 shutdown_hang_6699
          unhandled_exception_582
)

set(unhandled_exception_582_PARAMETERS THREADS_PER_LOCALITY 1)

if(HPX_WITH_FORCE_DISCONNECT)
  set(tests ${tests} dijkstra_termination_disconnected_locality_7474
            dijkstra_termination_disconnected_localities_7474
  )

  set(dijkstra_termination_disconnected_locality_7474_FLAGS DEPENDENCIES
                                                            process_component
  )
  add_hpx_executable(
    dijkstra_termination_disconnected_locality_7474_worker INTERNAL_FLAGS
    SOURCES dijkstra_termination_disconnected_locality_7474_worker.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/RuntimeDistributed"
  )
  set(dijkstra_termination_disconnected_locality_7474_PARAMETERS
      RUN_SERIAL
      --launch=$<TARGET_FILE:dijkstra_termination_disconnected_locality_7474_worker>
  )
  set(dijkstra_termination_disconnected_localities_7474_FLAGS DEPENDENCIES
                                                              process_component
  )
  set(dijkstra_termination_disconnected_localities_7474_PARAMETERS
      RUN_SERIAL
      --launch=$<TARGET_FILE:dijkstra_termination_disconnected_locality_7474_worker>
  )
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/RuntimeDistributed"
  )

  add_hpx_regression_test(
    "modules.runtime_distributed" ${test} ${${test}_PARAMETERS}
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

if(HPX_WITH_FORCE_DISCONNECT)
  add_dependencies(
    dijkstra_termination_disconnected_locality_7474_test
    dijkstra_termination_disconnected_locality_7474_worker
  )

  add_dependencies(
    dijkstra_termination_disconnected_localities_7474_test
    dijkstra_termination_disconnected_locality_7474_worker
  )

  # MSVC V1951 and V1952 expose ICE compiling this in C++20 module mode
  if(HPX_WITH_CXX_MODULES
     AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     AND (MSVC_VERSION LESS_EQUAL 1952)
  )
    target_compile_definitions(
      dijkstra_termination_disconnected_locality_7474_worker
      PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
    )
  endif()
endif()
