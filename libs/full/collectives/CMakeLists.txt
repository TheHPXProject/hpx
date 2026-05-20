# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/collectives/include
set(collectives_headers
    hpx/collectives/all_gather.hpp
    hpx/collectives/all_reduce.hpp
    hpx/collectives/all_to_all.hpp
    hpx/collectives/argument_types.hpp
    hpx/collectives/barrier.hpp
    hpx/collectives/broadcast.hpp
    hpx/collectives/broadcast_direct.hpp
    hpx/collectives/channel_communicator.hpp
    hpx/collectives/create_communicator.hpp
    hpx/collectives/detail/channel_communicator.hpp
    hpx/collectives/detail/communicator.hpp
    hpx/collectives/detail/latch.hpp
    hpx/collectives/exclusive_scan.hpp
    hpx/collectives/fold.hpp
    hpx/collectives/gather.hpp
    hpx/collectives/inclusive_scan.hpp
    hpx/collectives/latch.hpp
    hpx/collectives/reduce.hpp
    hpx/collectives/reduce_direct.hpp
    hpx/collectives/scatter.hpp
    hpx/collectives/spmd_block.hpp
    hpx/collectives/detail/latch.hpp
)

# Default location is $HPX_ROOT/libs/collectives/include_compatibility
# cmake-format: off
set(collectives_compat_headers
  hpx/lcos/barrier.hpp => hpx/barrier.hpp
  hpx/lcos/broadcast.hpp => hpx/include/lcos.hpp
  hpx/lcos/fold.hpp => hpx/include/lcos.hpp
  hpx/lcos/gather.hpp => hpx/include/lcos.hpp
  hpx/lcos/latch.hpp => hpx/latch.hpp
  hpx/lcos/reduce.hpp => hpx/include/lcos.hpp
  hpx/lcos/spmd_block.hpp => hpx/include/lcos.hpp
)

# cmake-format: on

# Default location is $HPX_ROOT/libs/collectives/src
set(collectives_sources
    all_gather.cpp
    all_reduce.cpp
    all_to_all.cpp
    barrier.cpp
    broadcast.cpp
    channel_communicator.cpp
    create_communicator.cpp
    detail/channel_communicator_server.cpp
    exclusive_scan.cpp
    gather.cpp
    inclusive_scan.cpp
    latch.cpp
    reduce.cpp
    scatter.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full collectives
  SOURCES ${collectives_sources}
  HEADERS ${collectives_headers}
  COMPAT_HEADERS ${collectives_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES
    hpx_actions
    hpx_actions_base
    hpx_async_colocated
    hpx_async_distributed
    hpx_components
    hpx_components_base
    hpx_naming_base
    hpx_runtime_components
    hpx_runtime_distributed
  CMAKE_SUBDIRS examples tests
)
