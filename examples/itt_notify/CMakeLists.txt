# Copyright (c) 2026 The STE||AR Group
# Copyright (c) 2026 Vansh Dobhal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_ITTNOTIFY)
  return()
endif()

set(example_programs itt_notify_smoke)

foreach(example_program ${example_programs})
  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources}
    FOLDER "Examples/ITTNotify" ${HPX_WITH_CXX_MODULES_OPTION}
  )

  add_hpx_example_target_dependencies("itt_notify" ${example_program})

  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_example_test(
      "itt_notify" ${example_program} ${${example_program}_PARAMETERS}
    )
  endif()
endforeach()
