# Copyright (c) 2019-2022 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    any
    any_serialization
    boost_any
    dynamic_bitset1
    dynamic_bitset2
    dynamic_bitset3
    dynamic_bitset4
    dynamic_bitset5
    flat_map
    flat_set
    forward_list
    intrusive_list
    is_tuple_like
    serializable_any
    serializable_boost_any
    serialization_optional
    serialization_raw_pointer_tuple
    serialization_tuple
    tuple
    unique_any
)

if(NOT HPX_WITH_HIP)
  set(tests ${tests} small_vector)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/Datastructures")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.datastructures" ${test} ${${test}_PARAMETERS})

  # gcc issues false positives for the dynamic_bitset tests
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${test}_test PRIVATE "-Wno-alloc-size-larger-than")
  endif()

endforeach()
