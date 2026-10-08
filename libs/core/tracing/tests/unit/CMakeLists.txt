# Copyright (c) 2026 Vansh Dobhal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The ITT test loads a recording collector through the ittnotify static part,
# which uses dlopen, so it is only built for the ITT backend on non-Windows, and
# only when one of the event classes it checks is compiled in.
if(NOT HPX_WITH_ITTNOTIFY
   OR WIN32
   OR NOT (HPX_WITH_TRACING_LIFECYCLE_EVENTS
           OR HPX_WITH_TRACING_WORK_STEALING_EVENTS)
)
  return()
endif()

add_library(
  hpx_itt_test_collector MODULE EXCLUDE_FROM_ALL itt_test_collector.cpp
)
target_include_directories(
  hpx_itt_test_collector PRIVATE ${Amplifier_INCLUDE_DIR}
)
set_target_properties(
  hpx_itt_test_collector PROPERTIES FOLDER "Tests/Unit/Modules/Core/Tracing"
)

add_hpx_executable(
  itt_task_lifecycle_test INTERNAL_FLAGS
  SOURCES itt_task_lifecycle.cpp
  EXCLUDE_FROM_ALL
  HPX_PREFIX ${HPX_BUILD_PREFIX}
  FOLDER "Tests/Unit/Modules/Core/Tracing"
)
target_link_libraries(itt_task_lifecycle_test PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(itt_task_lifecycle_test hpx_itt_test_collector)

set(collector_arg --collector=$<TARGET_FILE:hpx_itt_test_collector>)

# One worker makes inline execution deterministic; two exercise stealing.
if(HPX_WITH_TRACING_LIFECYCLE_EVENTS)
  add_hpx_unit_test(
    "modules.tracing" itt_task_lifecycle ${collector_arg}
    THREADS_PER_LOCALITY 1
  )
endif()
if(HPX_WITH_TRACING_WORK_STEALING_EVENTS)
  add_hpx_unit_test(
    "modules.tracing" itt_work_stealing ${collector_arg}
    EXECUTABLE itt_task_lifecycle
    PSEUDO_DEPS_NAME itt_task_lifecycle
    THREADS_PER_LOCALITY 2
  )
endif()
