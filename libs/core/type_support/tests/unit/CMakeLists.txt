# Copyright (c) 2023-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests relocate relocate_at uninitialized_relocate_n_primitive)

if(HPX_WITH_CXX20_COROUTINES)
  set(tests ${tests} generator)
endif()

foreach(test ${tests})

  set(test_PARAMETERS THREADS_PER_LOCALITY 4)

  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/TypeSupport"
  )

  add_hpx_unit_test("modules.type_support" ${test} ${${test}_PARAMETERS})

endforeach()

if(HPX_WITH_COMPILE_ONLY_TESTS)
  # add compile time tests
  set(compile_tests is_contiguous_iterator is_relocatable is_replaceable
                    is_trivially_relocatable
  )

  if(HPX_WITH_FAIL_COMPILE_TESTS)
    set(fail_compile_tests fail_relocate_at fail_uninitialized_relocate)
    foreach(fail_compile_test ${fail_compile_tests})
      set(${fail_compile_test}_FLAGS FAILURE_EXPECTED)
    endforeach()

    set(compile_tests ${compile_tests} ${fail_compile_tests})
  endif()

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    add_hpx_unit_compile_test(
      "modules.type_support" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Unit/Modules/Core/TypeSupport/CompileOnly"
    )

  endforeach()

endif()
