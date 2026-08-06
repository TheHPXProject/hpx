# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.components.supervision_dispatch)
  add_hpx_pseudo_dependencies(
    examples.components examples.components.supervision_dispatch
  )
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.components.supervision_dispatch)
    add_hpx_pseudo_dependencies(
      tests.examples.components tests.examples.components.supervision_dispatch
    )
  endif()
else()
  return()
endif()

set(example_programs component_worker plain_worker)

# late_component_worker/late_component_failing_worker are connect-mode-only
# binaries (see late_component_worker.cpp): they never run standalone, only when
# launched on demand by late_component_launcher via process::execute(). They are
# therefore built but not wired into add_hpx_example_test() below, the same way
# examples/heartbeat/heartbeat_console is built without being ctest-driven.
#
# late_component_launcher itself is the root of this family (it spawns the two
# binaries above on demand and never expects to be co-launched alongside them),
# but wiring it into ctest would additionally require the same kind of
# --launch=$<TARGET_FILE:...> plumbing used by launch_process/
# departed_locality_7384/os_assigned_parcelport_port_7406 (see
# libs/full/runtime_components/tests/unit/components/CMakeLists.txt and
# libs/full/agas/tests/regressions/CMakeLists.txt); left out of
# add_hpx_example_test() below for now, alongside its two workers.
set(example_programs_build_only
    late_component_worker late_component_failing_worker late_component_launcher
)

# cmake-format: off
set(plain_worker_PARAMETERS LOCALITIES 2)
set(plain_worker_FLAGS
    COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE
    DEPENDENCIES supervision_dispatch_component
)

set(component_worker_PARAMETERS LOCALITIES 2)
set(component_worker_FLAGS
    COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE
    DEPENDENCIES supervision_dispatch_component
)

set(late_component_worker_FLAGS
    COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE
    HEADERS late_component_worker.hpp
    DEPENDENCIES supervision_dispatch_component
)
set(late_component_failing_worker_FLAGS
    HEADERS late_component_worker.hpp
    DEPENDENCIES supervision_dispatch_component
)
set(late_component_launcher_FLAGS
    HEADERS late_component_worker.hpp
    DEPENDENCIES supervision_dispatch_component process_component
)
# cmake-format: on

foreach(example_program ${example_programs} ${example_programs_build_only})
  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources} ${${example_program}_FLAGS}
    FOLDER "Examples/Components/Supervision"
  )

  add_hpx_example_target_dependencies(
    "components.supervision_dispatch" ${example_program}
  )
endforeach()

add_dependencies(
  late_component_launcher late_component_worker late_component_failing_worker
)

if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
  foreach(example_program ${example_programs})
    add_hpx_example_test(
      "components.supervision_dispatch" ${example_program}
      ${${example_program}_PARAMETERS}
    )
  endforeach()
endif()
