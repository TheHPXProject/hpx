# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This file is auto generated. Please do not edit manually.

include(HPX_Message)
include(HPX_AddPseudoDependencies)
include(HPX_AddPseudoTarget)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

# Variable used to add the components to the pkgconfig linking step
set(HPX_COMPONENTS
    ""
    CACHE INTERNAL "list of HPX components" FORCE
)

set(component_dirs
    component_storage
    containers
    iostreams
    parcel_plugins
    performance_counters
    process
    supervision_dispatch
)

# add example pseudo targets needed for components
if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.components)
  add_hpx_pseudo_dependencies(examples examples.components)
endif()

# add test pseudo targets needed for components
if(HPX_WITH_TESTS)
  if(HPX_WITH_TESTS_UNIT)
    add_hpx_pseudo_target(tests.unit.components)
    add_hpx_pseudo_dependencies(tests.unit tests.unit.components)
  endif()

  if(HPX_WITH_EXAMPLES AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.components)
    add_hpx_pseudo_dependencies(tests.examples tests.examples.components)
  endif()

  if(HPX_WITH_TESTS_REGRESSIONS)
    add_hpx_pseudo_target(tests.regressions.components)
    add_hpx_pseudo_dependencies(tests.regressions tests.regressions.components)
  endif()

  if(HPX_WITH_TESTS_BENCHMARKS)
    add_hpx_pseudo_target(tests.performance.components)
    add_hpx_pseudo_dependencies(tests.performance tests.performance.components)
  endif()

  if(HPX_WITH_TESTS_HEADERS)
    add_hpx_pseudo_target(tests.headers.components)
    add_hpx_pseudo_dependencies(tests.headers tests.headers.components)
  endif()
endif()

hpx_info("Configuring components:")

add_hpx_pseudo_target(components)

foreach(component ${component_dirs})
  hpx_info("  ${component}")
  add_hpx_pseudo_target(components.${component})
  add_subdirectory(${component})
  add_hpx_pseudo_dependencies(components components.${component})
endforeach()
