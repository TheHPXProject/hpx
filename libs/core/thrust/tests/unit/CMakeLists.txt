# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT ((HPX_WITH_CUDA OR HPX_WITH_HIP) AND HPX_WITH_THRUST))
  return()
endif()

set(tests thrust_policy_traits)
# stdexec can't be compiled by nvcc with MSVC as the host compiler (the CUDA
# tests include hpx/modules/thrust.hpp, which may include stdexec)
if(NOT (MSVC AND CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA"))
  list(APPEND tests thrust_policy_test thrust_pooled_allocator
       thrust_serialization
  )
endif()
set(thrust_policy_traits_PARAMETERS THREADS_PER_LOCALITY 4)
set(thrust_policy_test_PARAMETERS THREADS_PER_LOCALITY 4)
set(thrust_policy_test_CUDA ON)
set(thrust_pooled_allocator_PARAMETERS THREADS_PER_LOCALITY 4)
set(thrust_pooled_allocator_CUDA ON)
set(thrust_pooled_allocator_SOURCES thrust_pooled_allocator_archives.cpp)
set(thrust_serialization_PARAMETERS THREADS_PER_LOCALITY 4)
set(thrust_serialization_CUDA ON)
set(thrust_serialization_SOURCES thrust_serialization_archives.cpp)

# the tests include thrust headers directly: with modules built as static
# libraries, hpx_core links hpx_thrust privately and doesn't forward the include
# directories of rocThrust
set(thrust_test_dependencies)
if(HPX_WITH_HIP AND TARGET roc::rocthrust)
  set(thrust_test_dependencies roc::rocthrust)
endif()

foreach(test ${tests})
  if(${${test}_CUDA})
    set(sources ${test}.cu)
  else()
    set(sources ${test}.cpp)
  endif()

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_SOURCES} ${${test}_FLAGS}
    DEPENDENCIES ${thrust_test_dependencies}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Thrust"
  )

  add_hpx_unit_test("modules.thrust" ${test} ${${test}_PARAMETERS})
endforeach()

# the same with pinned host memory, whatever the device
if(TARGET thrust_pooled_allocator_test)
  add_hpx_unit_test(
    "modules.thrust" thrust_pooled_allocator_pinned
    EXECUTABLE thrust_pooled_allocator
    PSEUDO_DEPS_NAME thrust_pooled_allocator
    THREADS_PER_LOCALITY 4
    ARGS --hpx:ini=hpx.thrust.memory=pinned
  )
endif()
if(TARGET thrust_serialization_test)
  add_hpx_unit_test(
    "modules.thrust" thrust_serialization_pinned
    EXECUTABLE thrust_serialization
    PSEUDO_DEPS_NAME thrust_serialization
    THREADS_PER_LOCALITY 4
    ARGS --hpx:ini=hpx.thrust.memory=pinned
  )
endif()
