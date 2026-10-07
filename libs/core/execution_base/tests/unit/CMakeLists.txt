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
)

if(HPX_WITH_CXX20_COROUTINES)
  set(tests ${tests} coroutine_traits coroutine_utils)
endif()

if(HPX_WITH_CUDA)
  set(tests ${tests} stdexec_cuda)
endif()

foreach(test ${tests})
  if(test STREQUAL "stdexec_cuda")
    set(sources ${test}.cu)
  else()
    set(sources ${test}.cpp)
  endif()

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

# Built without HPX_MODULE_STATIC_LINKING so that any_sender.hpp is compiled the
# way a normal consumer of the shared libraries sees it
add_hpx_executable(
  any_sender_consumer_test INTERNAL_FLAGS
  SOURCES any_sender_consumer.cpp
  NOLIBS
  DEPENDENCIES hpx_core ${BOOST_UNDERLYING_THREAD_LIBRARY}
  EXCLUDE_FROM_ALL
  FOLDER "Tests/Unit/Modules/Core/ExecutionBase"
)

add_hpx_unit_test("modules.execution_base" any_sender_consumer)
