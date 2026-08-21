# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests)

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP)

  set(tests ${tests} launch_connecting_locality)
  set(folder_name "Tests/Unit/Components/Process")

  # add executable needed for the launch_connecting_locality test
  add_hpx_executable(
    worker_connecting_locality INTERNAL_FLAGS
    SOURCES worker_connecting_locality.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER ${folder_name}
  )

  set(launch_connecting_locality_FLAGS DEPENDENCIES process_component)
  set(launch_connecting_locality_PARAMETERS
      RUN_SERIAL --launch=$<TARGET_FILE:worker_connecting_locality>
      # Force use of the TCP parcelport
      --hpx:ini=hpx.parcel.tcp.priority=1000 --hpx:ini=hpx.parcel.bootstrap=tcp
  )

  foreach(test ${tests})
    set(sources ${test}.cpp)
    source_group("Source Files" FILES ${sources})
    set(folder_name "Tests/Unit/Components/Process")

    add_hpx_executable(
      ${test}_test INTERNAL_FLAGS
      SOURCES ${sources} ${${test}_FLAGS}
      EXCLUDE_FROM_ALL
      HPX_PREFIX ${HPX_BUILD_PREFIX}
      FOLDER ${folder_name}
    )
    add_hpx_unit_test("components.process" ${test} ${${test}_PARAMETERS})
  endforeach()

  add_hpx_pseudo_dependencies(
    tests.unit.components.process.launch_connecting_locality
    worker_connecting_locality
  )
endif()
