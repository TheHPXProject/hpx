# Copyright (c) 2007-2016 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests lost_output_2236 no_output_1173 output_after_shutdown_8068)

set(lost_output_2236_FLAGS COMPONENT_DEPENDENCIES iostreams)

set(no_output_1173_PARAMETERS LOCALITIES 2)
set(no_output_1173_FLAGS COMPONENT_DEPENDENCIES iostreams)

set(output_after_shutdown_8068_FLAGS COMPONENT_DEPENDENCIES iostreams)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL ${HPX_WITH_CXX_MODULES_OPTION}
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Components/IO"
  )

  add_hpx_regression_test("components.iostreams" ${test} ${${test}_PARAMETERS})
endforeach()
