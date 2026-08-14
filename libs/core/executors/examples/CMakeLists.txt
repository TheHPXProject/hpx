# Copyright (c) 2022-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.modules.executors)
  add_hpx_pseudo_dependencies(examples.modules examples.modules.executors)
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.modules.executors)
    add_hpx_pseudo_dependencies(
      tests.examples.modules tests.examples.modules.executors
    )
  endif()
else()
  return()
endif()

set(example_programs disable_thread_stealing_executor
                     executor_with_thread_hooks
)

if(HPX_LIKWID_WITH_LIKWID)
  set(example_programs ${example_programs} executor_using_likwid)
endif()

foreach(example_program ${example_programs})
  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources} ${${example_program}_FLAGS}
    FOLDER "Examples/Modules/Core/Executors"
  )

  add_hpx_example_target_dependencies("modules.executors" ${example_program})

  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_example_test(
      "modules.executors" ${example_program} ${${example_program}_PARAMETERS}
    )
  endif()
endforeach()

if(HPX_WITH_CXX_MODULES AND MSVC)
  foreach(example disable_thread_stealing_executor)
    target_compile_definitions(${example} PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES)
  endforeach()
endif()
