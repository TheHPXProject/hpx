# Copyright (c) 2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests ignore_aliases_local queuing_options_7743)

set(ignore_aliases_local_PARAMETERS "-wobble=1")

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS ${${test}_PARAMETERS} THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/CommandLineHandlingLocal"
  )

  add_hpx_regression_test(
    "modules.command_line_handling_local" ${test} ${${test}_PARAMETERS}
  )
endforeach()
