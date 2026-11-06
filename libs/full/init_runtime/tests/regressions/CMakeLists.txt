# Copyright (c) 2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests hpx_init_header_7877 hpx_start_header_7877 init_header_7877)

foreach(test ${tests})
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${test}.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/InitRuntime"
    SCAN_FOR_MODULES ${HPX_WITH_CXX_MODULES}
  )

  # Each facade must provide user_main_config without help from other headers.
  set_target_properties(${test}_test PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

  add_hpx_regression_test("modules.init_runtime" ${test})
endforeach()
