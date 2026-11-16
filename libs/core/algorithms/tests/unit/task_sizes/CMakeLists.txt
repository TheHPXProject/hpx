# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The task sizes the algorithms give their worker tasks, see
# task_size_checks.hpp
set(tests task_sizes_multi_call task_sizes_predicates task_sizes_reduce_merge)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Algorithms/TaskSizes"
  )

  add_hpx_unit_test(
    "modules.algorithms.task_sizes" ${test} ${${test}_PARAMETERS}
  )
endforeach()
