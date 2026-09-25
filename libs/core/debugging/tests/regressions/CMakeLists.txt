# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests hostname_print_helper_7520 dbghelp_refresh_module_list_7608)

set(hostname_print_helper_7520_PARAMETERS THREADS_PER_LOCALITY 4)

# Create test cases
foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/Debugging"
  )

  target_link_libraries(${test}_test PRIVATE ${${test}_LIBRARIES})
  add_hpx_regression_test("modules.debugging" ${test} ${${test}_PARAMETERS})

endforeach()

if(WIN32)
  # Companion DLL for dbghelp_refresh_module_list_7608_test. It is deliberately
  # not linked into that executable: the whole point of #7608 is a module that
  # DbgHelp does not know about until after SymInitialize has already run, so
  # this must only enter the process via LoadLibraryW, at test runtime, exactly
  # like a plugin would. A plain add_library is used rather than add_hpx_library
  # because this is test fixture data, not an HPX module.
  add_library(
    dbghelp_refresh_module_list_7608_helper SHARED
    dbghelp_refresh_module_list_7608_helper.cpp
  )
  set_target_properties(
    dbghelp_refresh_module_list_7608_helper
    PROPERTIES FOLDER "Tests/Regressions/Modules/Core/Debugging"
               RUNTIME_OUTPUT_DIRECTORY
               "$<TARGET_FILE_DIR:dbghelp_refresh_module_list_7608_test>"
  )
  # The test process loads this by relative file name and relies on the default
  # DLL search order finding it next to the executable, so the two must land in
  # the same output directory and the helper must exist before the test runs.
  add_dependencies(
    dbghelp_refresh_module_list_7608_test
    dbghelp_refresh_module_list_7608_helper
  )
endif()
