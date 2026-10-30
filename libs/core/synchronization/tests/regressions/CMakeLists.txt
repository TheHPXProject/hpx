# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests ignore_while_locked_1485 shared_mutex_1702 shared_mutex_6854
          shared_mutex_dual_ownership
)

set(shared_mutex_1702_PARAMETERS THREADS_PER_LOCALITY 4)
set(shared_mutex_1702_LIBRARIES hpx_dependencies_boost)
set(ignore_while_locked_1485_PARAMETERS THREADS_PER_LOCALITY 2)
set(shared_mutex_6854_PARAMETERS THREADS_PER_LOCALITY 4)
set(shared_mutex_dual_ownership_PARAMETERS THREADS_PER_LOCALITY 4)

# Create test cases
foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/Synchronization"
  )

  target_link_libraries(${test}_test PRIVATE ${${test}_LIBRARIES})
  add_hpx_regression_test(
    "modules.synchronization" ${test} ${${test}_PARAMETERS}
  )

endforeach()
