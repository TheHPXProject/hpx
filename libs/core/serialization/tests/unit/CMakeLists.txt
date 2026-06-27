# Copyright (c) 2014 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    not_bitwise_serializable
    serialization_array
    serialization_valarray
    serialization_builtins
    serialization_complex
    serialization_custom_constructor
    serialization_deque
    serialization_list
    serialization_map
    serialization_multimap
    serialization_set
    serialization_multiset
    serialization_simple
    serialization_smart_ptr
    serialization_std_tuple
    serialization_unordered_map
    serialization_unordered_multimap
    serialization_unordered_set
    serialization_unordered_multiset
    serialization_vector
    serialize_with_incompatible_signature
    serialization_std_variant
)

set(full_tests serialization_raw_pointer)

if(HPX_SERIALIZATION_WITH_BOOST_TYPES)
  set(tests ${tests} serialization_boost_variant)
endif()

if(NOT HPX_SERIALIZATION_WITH_ALLOW_AUTO_GENERATE)
  set(tests ${tests} serialization_brace_initializable)
endif()

add_subdirectory(polymorphic)
add_subdirectory(refl)

# tests that can run without HPX
foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/Serialization")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL NOLIBS
    DEPENDENCIES hpx_core
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.serialization" ${test} ${${test}_PARAMETERS})
  target_compile_definitions(${test}_test PRIVATE -DHPX_MODULE_STATIC_LINKING)

endforeach()

# tests that depend on HPX
foreach(test ${full_tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/Serialization")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.serialization" ${test} ${${test}_PARAMETERS})

endforeach()

if(HPX_WITH_COMPILE_ONLY_TESTS)
  # add compile time tests
  set(compile_tests succeed_compile_empty_class)

  if(HPX_WITH_FAIL_COMPILE_TESTS)
    set(fail_compile_tests)
    foreach(fail_compile_test ${fail_compile_tests})
      set(${fail_compile_test}_FLAGS FAILURE_EXPECTED)
    endforeach()

    set(compile_tests ${compile_tests} ${fail_compile_tests})
  endif()

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    add_hpx_unit_compile_test(
      "modules.serialization" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Unit/Modules/Core/Serialization/CompileOnly"
    )

  endforeach()

endif()
