# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(thread_support_headers
    hpx/thread_support/atomic_count.hpp hpx/thread_support/set_thread_name.hpp
    hpx/thread_support/spinlock.hpp hpx/thread_support/thread_specific_ptr.hpp
    hpx/thread_support/unlock_guard.hpp
)

# cmake-format: off
set(thread_support_compat_headers
    hpx/thread_support.hpp => hpx/modules/thread_support.hpp
    hpx/util/atomic_count.hpp => hpx/modules/thread_support.hpp
    hpx/util/set_thread_name.hpp => hpx/modules/thread_support.hpp
    hpx/util/thread_specific_ptr.hpp => hpx/modules/thread_support.hpp
    hpx/util/unlock_guard.hpp => hpx/modules/thread_support.hpp
)
# cmake-format: on

set(thread_support_sources set_thread_name.cpp spinlock.cpp)

include(HPX_AddModule)
add_hpx_module(
  core thread_support
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${thread_support_sources}
  HEADERS ${thread_support_headers}
  COMPAT_HEADERS ${thread_support_compat_headers}
  MODULE_DEPENDENCIES hpx_assertion hpx_config hpx_concepts hpx_tracing
                      hpx_type_support
  CMAKE_SUBDIRS examples tests
)
