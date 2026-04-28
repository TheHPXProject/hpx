# Copyright (c) 2014-2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    adjacentdifference_range
    adjacentdifference_range_sender
    adjacentfind_bad_alloc_range
    adjacentfind_binary_bad_alloc_range
    adjacentfind_binary_range
    adjacentfind_binary_projection_range
    adjacentfind_binary_exception_range
    adjacentfind_range
    adjacentfind_exception_range
    all_of_range
    any_of_range
    contains_range
    contains_subrange_range
    copy_range
    copyn_range
    copyif_range
    count_range
    countif_range
    destroy_range
    destroyn_range
    distance_range
    ends_with_range
    equal_range
    equal_binary_range
    exclusive_scan_range
    fill_range
    filln_range
    find_range
    find_if_range
    find_if_not_range
    find_if_not_exception_range
    find_end_range
    find_end_range2
    find_first_of_range
    find_first_of_range2
    iota_range
    fold_range
    for_loop_exception_range
    for_loop_induction_range
    for_loop_induction_async_range
    for_loop_range
    for_loop_reduction_range
    for_loop_reduction_async_range
    for_loop_strided_range
    foreach_adapt
    foreach_range
    foreach_range_projection
    foreach_range_sender
    generate_range
    includes_range
    inclusive_scan_range
    inplace_merge_range
    is_heap_range
    is_heap_until_range
    is_partitioned_async_range
    is_partitioned_range
    is_partitioned_projection_range
    is_sorted_projection
    is_sorted_range
    is_sorted_range2
    is_sorted_range3
    is_sorted_range_exception
    is_sorted_until_range
    is_sorted_until_range_exception
    lexicographical_compare_range
    make_heap_range
    max_element_range
    merge_range
    min_element_range
    minmax_element_range
    minmax_element_parallel_projection
    minmax_element_projection
    mismatch_binary_range
    mismatch_range
    move_range
    nth_element_range
    none_of_range
    partial_sort_range
    partial_sort_copy_range
    partition_range
    partition_copy_range
    reduce_range
    remove_range
    remove_if_range
    remove_copy_range
    remove_copy_if_range
    replace_range
    replace_copy_range
    replace_copy_if_range
    replace_if_range
    reverse_range
    reverse_copy_range
    rotate_range
    rotate_copy_range
    search_range
    search_n_range
    set_difference_range
    set_intersection_range
    set_symmetric_difference_range
    set_union_range
    shift_left_range
    shift_right_range
    sort_range
    stable_partition_range
    stable_sort_range
    starts_with_range
    swap_ranges_range
    transform_range
    transform_range_binary
    transform_range2
    transform_range_binary2
    transform_exclusive_scan_range
    transform_inclusive_scan_range
    transform_reduce_binary_bad_alloc_range
    transform_reduce_binary_exception_range
    transform_reduce_binary_range
    transform_reduce_range
    uninitialized_copy_range
    uninitialized_copy_n_range
    uninitialized_default_construct_range
    uninitialized_default_constructn_range
    uninitialized_fill_range
    uninitialized_filln_range
    uninitialized_move_range
    uninitialized_move_n_range
    uninitialized_value_construct_range
    uninitialized_value_constructn_range
    unique_range
    unique_copy_range
)

if(HPX_WITH_CXX20_COROUTINES)
  set(tests ${tests} for_loop_range_generator)
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
    FOLDER "Tests/Unit/Modules/Core/Algorithms/Container"
  )

  target_link_libraries(
    ${test}_test PRIVATE hpx_iterator_support_test_utilities
  )

  add_hpx_unit_test(
    "modules.algorithms.container_algorithms" ${test} ${${test}_PARAMETERS}
  )
endforeach()
