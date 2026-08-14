# Copyright (c) 2007-2021 Hartmut Kaiser
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# add subdirectories
set(subdirs actions util)

foreach(subdir ${subdirs})
  add_hpx_pseudo_target(tests.unit.modules.async_distributed.${subdir})
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(
    tests.unit.modules.async_distributed
    tests.unit.modules.async_distributed.${subdir}
  )
endforeach()

set(tests
    async_cb_remote
    async_cb_remote_client
    async_continue
    async_continue_cb
    async_remote
    async_remote_client
    async_unwrap_result
    post_remote
    post_remote_client
    remote_dataflow
    sync_remote
)

set(async_continue_PARAMETERS LOCALITIES 2)
set(async_continue_cb_PARAMETERS LOCALITIES 2)
set(async_remote_PARAMETERS LOCALITIES 2)

if(HPX_WITH_CXX26_REFLECTION)
  set(tests ${tests} async_remote_reflect)
  set(async_remote_reflect_PARAMETERS LOCALITIES 2)
endif()
set(async_remote_client_PARAMETERS LOCALITIES 2)
set(async_cb_remote_PARAMETERS LOCALITIES 2)
set(async_cb_remote_client_PARAMETERS LOCALITIES 2)

set(post_remote_PARAMETERS LOCALITIES 2)
set(post_remote_client_PARAMETERS LOCALITIES 2)

set(remote_dataflow_PARAMETERS THREADS_PER_LOCALITY 4)
set(remote_dataflow_PARAMETERS LOCALITIES 2)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Full/AsyncDistributed")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.async_distributed" ${test} ${${test}_PARAMETERS})
endforeach()
