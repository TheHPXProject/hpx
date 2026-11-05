# Copyright (c) 2007-2022 Hartmut Kaiser
# Copyright (c) 2011 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests unwrapped_1528 unwrapping_nullary_6045 unwrap_nested_containers_7945)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Regressions/Modules/Core/PackTraversal"
  )

  add_hpx_regression_test(
    "modules.pack_traversal" ${test} ${${test}_PARAMETERS}
  )
endforeach()

# compile only tests
if(HPX_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests unwrapping_noncopyable)

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    add_hpx_regression_compile_test(
      "modules.pack_traversal" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Regressions/Modules/Core/PackTraversal/CompileOnly"
    )
  endforeach()
endif()
