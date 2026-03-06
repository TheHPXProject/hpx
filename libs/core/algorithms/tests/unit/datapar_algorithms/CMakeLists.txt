# Copyright (c) 2014-2019 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests)

if(HPX_WITH_DATAPAR)
  set(tests
      ${tests}
      adjacentfind_datapar
      adjacentdifference_datapar
      all_of_datapar
      any_of_datapar
      copy_datapar
      copyn_datapar
      count_datapar
      countif_datapar
      equal_binary_datapar
      equal_datapar
      fill_datapar
      filln_datapar
      find_datapar
      findend_datapar
      findfirstof_datapar
      findif_datapar
      findifnot_datapar
      foreach_datapar
      foreach_datapar_zipiter
      foreachn_datapar
      for_loop_datapar
      generate_datapar
      generaten_datapar
      mismatch_binary_datapar
      mismatch_datapar
      none_of_datapar
      reduce_datapar
      remove_datapar
      remove_if_datapar
      replace_copy_if_datapar
      replace_copy_datapar
      replace_datapar
      replace_if_datapar
      search_datapar
      search_n_datapar
      transform_binary_datapar
      transform_binary2_datapar
      transform_datapar
      transform_reduce_datapar
      transform_reduce_binary_datapar
  )
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Algorithms/Datapar"
  )

  add_hpx_unit_test(
    "modules.algorithms.datapar_algorithms" ${test} ${${test}_PARAMETERS}
  )
endforeach()
