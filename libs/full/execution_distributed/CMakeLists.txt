# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(execution_distributed_headers
    hpx/execution_distributed/detail/distributed_receiver_component.hpp
    hpx/execution_distributed/detail/transfer_action.hpp
    hpx/execution_distributed/distributed_bulk_sender.hpp
    hpx/execution_distributed/distributed_continues_on_sender.hpp
    hpx/execution_distributed/distributed_scheduler.hpp
    hpx/execution_distributed/distributed_then_sender.hpp
)

set(execution_distributed_sources)

include(HPX_AddModule)
add_hpx_module(
  full execution_distributed
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${execution_distributed_sources}
  HEADERS ${execution_distributed_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions_base hpx_components_base hpx_naming_base
  CMAKE_SUBDIRS examples tests
)
