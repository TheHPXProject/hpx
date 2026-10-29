# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(io_service_headers
    hpx/io_service/io_service_pool.hpp hpx/io_service/io_service_pool_fwd.hpp
    hpx/io_service/schedule_timer.hpp hpx/io_service/io_service_thread_pool.hpp
)

# cmake-format: off
set(io_service_compat_headers
    hpx/util/io_service_pool_fwd.hpp => hpx/modules/io_service.hpp
)
# cmake-format: on

set(io_service_sources io_service_pool.cpp io_service_thread_pool.cpp
                       schedule_timer.cpp
)

include(HPX_AddModule)
add_hpx_module(
  core io_service
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${io_service_sources}
  HEADERS ${io_service_headers}
  COMPAT_HEADERS ${io_service_compat_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/io_service/io_service_pool_fwd.hpp"
  DEPENDENCIES Asio::asio
  MODULE_DEPENDENCIES
    hpx_affinity
    hpx_assertion
    hpx_concurrency
    hpx_config
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_logging
    hpx_threading_base
    hpx_timing
  CMAKE_SUBDIRS examples tests
)
