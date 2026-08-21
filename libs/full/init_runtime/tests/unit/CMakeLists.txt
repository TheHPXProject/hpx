# Copyright (c) 2020-2026 The STE||AR-Group
#               2011 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests runtime_type parcel_pool shutdown_suspended_thread
          start_stop_callbacks
)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  set(tests ${tests} handled_exception unhandled_exception)
endif()

if(HPX_WITH_SUPERVISION)
  if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP)
    set(tests ${tests} force_disconnect)

    # add executable needed for the force_disconnect test
    add_hpx_executable(
      force_disconnect_worker INTERNAL_FLAGS
      SOURCES force_disconnect_worker.cpp
      EXCLUDE_FROM_ALL
      HPX_PREFIX ${HPX_BUILD_PREFIX}
      FOLDER "Tests/Unit/Modules/Full/InitRuntime"
    )

    # cmake-format: off
    set(force_disconnect_PARAMETERS
        RUN_SERIAL
        --launch=$<TARGET_FILE:force_disconnect_worker>
        # Force use of the TCP parcelport
        --hpx:ini=hpx.parcel.tcp.priority=1000
        --hpx:ini=hpx.parcel.bootstrap=tcp
    )
    set(force_disconnect_FLAGS
        DEPENDENCIES process_component supervision_dispatch_component
    )
    # cmake-format: on
  endif()
endif()

set(unhandled_exception_PARAMETERS FAILURE_EXPECTED)

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS ${${test}_PARAMETERS} THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Full/InitRuntime")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.init_runtime" ${test} ${${test}_PARAMETERS})
endforeach()

if(HPX_WITH_SUPERVISION
   AND HPX_WITH_NETWORKING
   AND HPX_WITH_PARCELPORT_TCP
)
  # MSVC V1951 and V1952 expose ICE compiling this in C++20 module mode
  if(HPX_WITH_CXX_MODULES
     AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     AND (MSVC_VERSION LESS_EQUAL 1952)
  )
    target_compile_definitions(
      force_disconnect_worker PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
    )
    target_compile_definitions(
      force_disconnect_test PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
    )
  endif()

  add_hpx_pseudo_dependencies(
    tests.unit.modules.init_runtime.force_disconnect force_disconnect_worker
  )
endif()
