# Copyright (c) 2017 Ajai V George
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    partitioned_vector_adjacent_difference1
    partitioned_vector_adjacent_difference2
    partitioned_vector_adjacent_find1
    partitioned_vector_adjacent_find2
    partitioned_vector_all_of1
    partitioned_vector_all_of2
    partitioned_vector_any_of1
    partitioned_vector_any_of2
    partitioned_vector_copy
    partitioned_vector_equal
    partitioned_vector_for_each
    partitioned_vector_for_each_double
    partitioned_vector_for_each_n
    partitioned_vector_generate
    partitioned_vector_handle_values
    partitioned_vector_iter
    partitioned_vector_max_element1
    partitioned_vector_max_element2
    partitioned_vector_min_element1
    partitioned_vector_min_element2
    partitioned_vector_minmax_element1
    partitioned_vector_minmax_element2
    partitioned_vector_mismatch
    partitioned_vector_move
    partitioned_vector_target
    partitioned_vector_transform1
    partitioned_vector_transform2
    partitioned_vector_transform_binary
    partitioned_vector_transform_binary1
    partitioned_vector_transform_binary2
    partitioned_vector_transform_binary3
    partitioned_vector_transform_reduce1
    partitioned_vector_transform_reduce2
    partitioned_vector_transform_reduce_binary1
    partitioned_vector_transform_reduce_binary2
    partitioned_vector_fill
    partitioned_vector_find
    partitioned_vector_find2
    partitioned_vector_inclusive_scan
    partitioned_vector_inclusive_scan2
    partitioned_vector_is_sorted
    partitioned_vector_is_partitioned
    partitioned_vector_exclusive_scan
    partitioned_vector_exclusive_scan2
    partitioned_vector_none1
    partitioned_vector_none2
    partitioned_vector_transform_scan
    partitioned_vector_transform_scan2
    partitioned_vector_reduce
    partitioned_vector_replace
    partitioned_vector_replace_copy
)

set(partitioned_vector_inclusive_scan_PARAMETERS RUN_SERIAL)
set(partitioned_vector_inclusive_scan2_PARAMETERS RUN_SERIAL)
set(partitioned_vector_exclusive_scan_PARAMETERS RUN_SERIAL)
set(partitioned_vector_exclusive_scan2_PARAMETERS RUN_SERIAL)
set(partitioned_vector_target_PARAMETERS RUN_SERIAL)

if(HPX_WITH_PARCELPORT_LCI)
  set(no_lci_tests
      partitioned_vector_find
      partitioned_vector_adjacent_find1
      partitioned_vector_adjacent_find2
      partitioned_vector_inclusive_scan
      partitioned_vector_inclusive_scan2
      partitioned_vector_exclusive_scan
      partitioned_vector_exclusive_scan2
  )
  foreach(no_lci_test ${no_lci_tests})
    set(${no_lci_test}_PARAMETERS ${${no_lci_test}_PARAMETERS}
                                  NO_PARCELPORT_LCI
    )
  endforeach()
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_FLAGS DEPENDENCIES partitioned_vector_component)
  set(${test}_PARAMETERS ${${test}_PARAMETERS} LOCALITIES 2
                         THREADS_PER_LOCALITY 4
  )

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Full/SegmentedAlgorithms")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER ${folder_name}
  )

  add_hpx_unit_test(
    "modules.segmented_algorithms" ${test} ${${test}_PARAMETERS}
  )

endforeach()
