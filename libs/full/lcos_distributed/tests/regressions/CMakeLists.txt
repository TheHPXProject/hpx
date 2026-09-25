# Copyright (c) 2007-2017 Hartmut Kaiser
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests channel_2916 channel_not_empty_2890 channel_register_as_2722
          channel_register_2arg_macro_7613
)

set(channel_2916_FLAGS DEPENDENCIES iostreams_component)
set(channel_2916_PARAMETERS THREADS_PER_LOCALITY 4)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/LCOsDistributed"
  )

  target_link_libraries(${test}_test PRIVATE ${${test}_LIBRARIES})
  add_hpx_regression_test(
    "modules.lcos_distributed" ${test} ${${test}_PARAMETERS}
  )
endforeach()
