# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/itt_notify/include
set(itt_notify_headers
    hpx/itt_notify/api.hpp hpx/itt_notify/macros.hpp
    hpx/itt_notify/detail/use_ittnotify_api.hpp hpx/itt_notify/thread_name.hpp
)
set(itt_notify_macro_headers hpx/itt_notify/macros.hpp)

# Default location is $HPX_ROOT/libs/itt_notify/include_compatibility
# cmake-format: off
set(itt_notify_compat_headers
    hpx/util/itt_notify.hpp => hpx/modules/itt_notify.hpp
)
# cmake-format: on

set(itt_notify_sources itt_notify.cpp)

if(TARGET Amplifier::amplifier)
  set(itt_notify_optional_dependencies Amplifier::amplifier)
endif()

include(HPX_AddModule)
add_hpx_module(
  core itt_notify
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${itt_notify_sources}
  HEADERS ${itt_notify_headers}
  MACRO_HEADERS ${itt_notify_macro_headers}
  COMPAT_HEADERS ${itt_notify_compat_headers}
  DEPENDENCIES ${itt_notify_optional_dependencies}
  MODULE_DEPENDENCIES hpx_config
  CMAKE_SUBDIRS examples tests
)
