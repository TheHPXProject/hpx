# Copyright (c) 2020-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests ignore_aliases late_commandline_options_5257 node_specific_options
          options_as_config_3339 use_all_cores_2262
)

set(tests ${tests} configuration_1572)

set(ignore_aliases_PARAMETERS "-wobble=1")

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS ${${test}_PARAMETERS} THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL ${${test}_FLAGS}
    FOLDER "Tests/Regressions/Modules/Full/CommandLineHandling/"
  )

  add_hpx_regression_test(
    "modules.command_line_handling" ${test} ${${test}_PARAMETERS}
  )

endforeach()
