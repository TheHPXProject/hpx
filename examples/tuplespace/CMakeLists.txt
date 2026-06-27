# Copyright (c) 2007-2009 Hartmut Kaiser
# Copyright (c) 2011      Bryce Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

add_subdirectory(central_tuplespace)

set(example_programs simple_central_tuplespace)

set(simple_central_tuplespace_PARAMETERS THREADS_PER_LOCALITY 4)

# for all targets specified above
foreach(example_program ${example_programs})
  set(sources ${example_program}_client.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program}_client INTERNAL_FLAGS
    SOURCES ${sources}
    COMPONENT_DEPENDENCIES ${example_program} iostreams
    FOLDER "Examples/TupleSpace/${example_program}"
           ${HPX_WITH_CXX_MODULES_OPTION}
  )

  add_hpx_example_target_dependencies("tuplespace" ${example_program}_client)

  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_example_test(
      "tuplespace" ${example_program}_client ${${example_program}_PARAMETERS}
    )
  endif()
endforeach()

if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 1952))
)
  target_compile_definitions(
    simple_central_tuplespace_client PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()
