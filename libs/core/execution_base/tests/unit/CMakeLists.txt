# Copyright (c) 2019 Thomas Heller
# Copyright (c) 2020-2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    any_sender
    basic_operation_state
    basic_receiver
    basic_sender
    basic_schedule
    completion_signatures
    execution_context
    get_env
    execute_may_block_caller
    stdexec
    test_tag_invoke_only_completion_signatures
)

if(HPX_WITH_CXX20_COROUTINES)
  set(tests ${tests} coroutine_traits coroutine_utils)
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  foreach(test ${tests})
    set(sources ${test}.cpp)

    source_group("Source Files" FILES ${sources})

    # add example executable
    add_hpx_executable(
      ${test}_test INTERNAL_FLAGS
      SOURCES ${sources}
      NOLIBS
      DEPENDENCIES hpx_core ${BOOST_UNDERLYING_THREAD_LIBRARY}
      EXCLUDE_FROM_ALL
      FOLDER "Tests/Unit/Modules/Core/ExecutionBase"
    )

    target_link_libraries(${test}_test PRIVATE hpx_execution_test_utilities)

    add_hpx_unit_test("modules.execution_base" ${test} ${${test}_PARAMETERS})
    target_compile_definitions(${test}_test PRIVATE -DHPX_MODULE_STATIC_LINKING)

  endforeach()
endif()
