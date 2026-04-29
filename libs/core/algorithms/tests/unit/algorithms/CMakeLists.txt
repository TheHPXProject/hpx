# Copyright (c) 2014-2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# add subdirectories
set(subdirs detail util)

foreach(subdir ${subdirs})
  add_hpx_pseudo_target(tests.unit.modules.algorithms.${subdir})
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(
    tests.unit.modules.algorithms tests.unit.modules.algorithms.${subdir}
  )
endforeach()

# add tests
set(tests
    adjacentdifference
    adjacentfind
    adjacentfind_binary
    all_of
    any_of
    copy
    copyif_random
    copyif_forward
    copyif_exception
    copyif_bad_alloc
    copyn
    count
    countif
    destroy
    destroyn
    distance
    ends_with
    equal
    equal_binary
    exclusive_scan
    exclusive_scan2
    exclusive_scan_exception
    exclusive_scan_bad_alloc
    exclusive_scan_validate
    fill
    filln
    find
    find_last
    find_last_if
    find_last_if_not
    findend
    findfirstof
    findfirstof_binary
    findif
    findifnot
    foreach
    foreach_executors
    foreach_prefetching
    foreach_scheduler
    foreachn
    foreachn_exception
    foreachn_bad_alloc
    for_each_index
    for_loop
    for_loop_exception
    for_loop_induction
    for_loop_induction_async
    for_loop_n
    for_loop_n_strided
    for_loop_reduction
    for_loop_reduction_async
    for_loop_strided
    generate
    generaten
    is_heap
    is_heap_until
    iota
    includes
    inclusive_scan
    inclusive_scan_exception
    inplace_merge
    is_partitioned
    is_sorted
    is_sorted_until
    lexicographical_compare
    make_heap
    max_element
    merge
    min_element
    minmax_element
    mismatch
    mismatch_binary
    move
    nth_element
    none_of
    parallel_sort
    partial_sort
    partial_sort_copy
    partition
    partition_copy
    reduce_
    reduce_by_key
    reduce_deterministic
    remove
    remove1
    remove2
    remove_if
    remove_if1
    remove_copy
    remove_copy_if
    replace
    replace_if
    replace_copy
    replace_copy_if
    reverse
    reverse_copy
    rotate
    rotate_copy
    search
    search_n
    set_difference
    set_intersection
    set_symmetric_difference
    set_union
    shift_left
    shift_right
    sort
    sort_by_key
    sort_exceptions
    stable_partition
    stable_sort
    stable_sort_exceptions
    starts_with
    swapranges
    transform
    transform_binary
    transform_binary2
    transform_exclusive_scan
    transform_inclusive_scan
    transform_reduce
    transform_reduce_binary
    transform_reduce_binary_exception
    transform_reduce_binary_bad_alloc
    uninitialized_copy
    uninitialized_copyn
    uninitialized_default_construct
    uninitialized_default_constructn
    uninitialized_fill
    uninitialized_filln
    uninitialized_move
    uninitialized_moven
    uninitialized_relocate_backward
    uninitialized_relocate
    uninitialized_relocaten
    uninitialized_value_construct
    uninitialized_value_constructn
    unique
    unique_copy
)

if(HPX_WITH_CXX17_STD_EXECUTION_POLICES)
  set(tests ${tests} foreach_std_policies)
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  set(tests
      ${tests}
      adjacentdifference_sender
      adjacentfind_sender
      all_of_sender
      any_of_sender
      copy_sender
      copyn_sender
      count_sender
      countif_sender
      destroy_sender
      destroyn_sender
      ends_with_sender
      equal_sender
      equal_binary_sender
      fill_sender
      filln_sender
      find_sender
      findend_sender
      findfirstof_sender
      findif_sender
      findifnot_sender
      foreach_sender
      foreachn_sender
      generate_sender
      generaten_sender
      is_heap_sender
      is_heap_until_sender
      includes_sender
      is_partitioned_sender
      is_sorted_sender
      is_sorted_until_sender
      lexicographical_compare_sender
      max_element_sender
      min_element_sender
      minmax_element_sender
      mismatch_sender
      mismatch_binary_sender
      move_sender
      none_of_sender
      partial_sort_sender
      reduce_sender
      remove_sender
      remove_if_sender
      replace_sender
      replace_if_sender
      replace_copy_sender
      replace_copy_if_sender
      reverse_sender
      reverse_copy_sender
      rotate_sender
      rotate_copy_sender
      search_sender
      starts_with_sender
      swapranges_sender
      transform_sender
      transform_binary_sender
      transform_binary2_sender
      transform_reduce_sender
      transform_reduce_binary_sender
      uninitialized_copy_sender
      uninitialized_copyn_sender
      uninitialized_default_construct_sender
      uninitialized_default_constructn_sender
      uninitialized_fill_sender
      uninitialized_filln_sender
      uninitialized_move_sender
      uninitialized_moven_sender
      uninitialized_relocate_sender
      uninitialized_relocate_backward_sender
      uninitialized_relocaten_sender
      uninitialized_value_construct_sender
      uninitialized_value_constructn_sender
      unique_sender
  )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  list(REMOVE_ITEM tests foreach_scheduler)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/Algorithms")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER ${folder_name}
  )

  target_link_libraries(
    ${test}_test PRIVATE hpx_iterator_support_test_utilities
  )

  add_hpx_unit_test(
    "modules.algorithms.algorithms" ${test} ${${test}_PARAMETERS}
  )
endforeach()
