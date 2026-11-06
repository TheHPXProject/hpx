# Copyright (c) 2021-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.modules.collectives)
  add_hpx_pseudo_dependencies(examples.modules examples.modules.collectives)
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.modules.collectives)
    add_hpx_pseudo_dependencies(
      tests.examples.modules tests.examples.modules.collectives
    )
  endif()
else()
  return()
endif()

set(example_programs channel_communicator channel_pingpong distributed_pi
                     hierarchical_collectives
)

set(channel_communicator_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 2)
set(channel_communicator_FLAGS DEPENDENCIES iostreams_component)

set(distributed_pi_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 2)
set(distributed_pi_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(channel_pingpong_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 2)
set(channel_pingpong_FLAGS COMPILE_FLAGS -DHPX_HAVE_RUN_MAIN_EVERYWHERE)

set(hierarchical_collectives_PARAMETERS LOCALITIES 4 THREADS_PER_LOCALITY 1)

foreach(example_program ${example_programs})

  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources} ${${example_program}_FLAGS}
    FOLDER "Examples/Modules/Full/Collectives"
  )

  add_hpx_example_target_dependencies("modules.collectives" ${example_program})

  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_example_test(
      "modules.collectives" ${example_program} ${${example_program}_PARAMETERS}
    )
  endif()

endforeach()
