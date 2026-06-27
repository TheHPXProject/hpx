# Copyright (c) 2020-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(executors_distributed_headers
    hpx/executors_distributed/distribution_policy_executor.hpp
)

# cmake-format: off
set(executors_distributed_compat_headers
    hpx/parallel/executors/distribution_policy_executor.hpp => hpx/modules/executors_distributed.hpp
)
# cmake-format: on

include(HPX_AddModule)
add_hpx_module(
  full executors_distributed
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${executors_distributed_headers}
  COMPAT_HEADERS ${executors_distributed_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions_base hpx_async_distributed
  CMAKE_SUBDIRS examples tests
)
