# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests supervision_api)

set(supervision_api_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 4)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/Supervision"
  )

  add_hpx_unit_test("modules.supervision" ${test} ${${test}_PARAMETERS})

endforeach()

# MSVC V1951 and V1952 expose ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 1952))
)
  target_compile_definitions(
    supervision_api_test PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()
