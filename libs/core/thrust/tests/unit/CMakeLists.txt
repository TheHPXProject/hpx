# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_CUDA AND HPX_WITH_THRUST))
  return()
endif()

set(tests thrust_policy_test policy_traits)
set(policy_traits_PARAMETERS THREADS_PER_LOCALITY 4)
set(thrust_policy_test_PARAMETERS THREADS_PER_LOCALITY 4)
set(thrust_policy_test_CUDA ON)

foreach(test ${tests})
  if(${${test}_CUDA})
    set(sources ${test}.cu)
  else()
    set(sources ${test}.cpp)
  endif()

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    DEPENDENCIES ${${test_program}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Thrust"
  )

  add_hpx_unit_test("modules.thrust" ${test} ${${test}_PARAMETERS})
endforeach()
