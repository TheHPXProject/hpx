# Copyright (c) 2024 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.components.process)
  add_hpx_pseudo_dependencies(examples.components examples.components.process)
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.components.process)
    add_hpx_pseudo_dependencies(
      tests.examples.components tests.examples.components.process
    )
  endif()
else()
  return()
endif()

set(example_programs launch_command)
set(launch_command_FLAGS DEPENDENCIES process_component)

foreach(example_program ${example_programs})
  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources} ${${example_program}_FLAGS}
    FOLDER "Examples/Components/Process"
  )

  add_hpx_example_target_dependencies("components.process" ${example_program})

  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_example_test(
      "components.process" ${example_program} ${${example_program}_PARAMETERS}
    )
  endif()
endforeach()
