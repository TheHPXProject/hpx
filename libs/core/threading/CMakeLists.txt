# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(threading_headers hpx/threading/jthread.hpp hpx/threading/thread.hpp)

# cmake-format: off
set(threading_compat_headers
    hpx/runtime/threads/thread.hpp => hpx/thread.hpp
)
# cmake-format: on

set(threading_sources thread.cpp)

include(HPX_AddModule)
add_hpx_module(
  core threading
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${threading_sources}
  HEADERS ${threading_headers}
  COMPAT_HEADERS ${threading_compat_headers}
  MODULE_DEPENDENCIES
    hpx_errors
    hpx_functional
    hpx_futures
    hpx_lock_registration
    hpx_memory
    hpx_synchronization
    hpx_threading_base
    hpx_thread_support
    hpx_timing
  CMAKE_SUBDIRS examples tests
)
