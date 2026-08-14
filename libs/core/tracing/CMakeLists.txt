# Copyright (c) 2026 Hartmut Kaiser
# Copyright (c) 2026 Vansh Dobhal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tracing_headers
    hpx/tracing/backends/apex.hpp hpx/tracing/backends/empty.hpp
    hpx/tracing/backends/ittnotify.hpp hpx/tracing/backends/tracy.hpp
    hpx/tracing/macros.hpp hpx/tracing/tracing.hpp
)
set(tracing_sources backends/ittnotify.cpp backends/tracy.cpp)

set(tracing_module_dependencies hpx_config)
if(HPX_WITH_TRACY)
  list(APPEND tracing_module_dependencies hpx_tracy)
endif()
if(HPX_WITH_ITTNOTIFY)
  list(APPEND tracing_module_dependencies hpx_itt_notify)
endif()

include(HPX_AddModule)
add_hpx_module(
  core tracing
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${tracing_sources}
  HEADERS ${tracing_headers}
  MACRO_HEADERS hpx/tracing/macros.hpp
  EXCLUDE_FROM_GLOBAL_HEADER
    hpx/tracing/backends/apex.hpp hpx/tracing/backends/empty.hpp
    hpx/tracing/backends/ittnotify.hpp hpx/tracing/backends/tracy.hpp
  MODULE_DEPENDENCIES ${tracing_module_dependencies}
)
