# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(debugging_headers
    hpx/debugging/attach_debugger.hpp
    hpx/debugging/backtrace.hpp
    hpx/debugging/backtrace/backtrace.hpp
    hpx/debugging/demangle_helper.hpp
    hpx/debugging/detail/dbghelp_lock.hpp
    hpx/debugging/detail/dbghelp_symbol_cache.hpp
    hpx/debugging/environ.hpp
    hpx/debugging/macros.hpp
    hpx/debugging/print.hpp
)
set(debugging_macro_headers hpx/debugging/macros.hpp)

# cmake-format: off
set(debugging_compat_headers
    hpx/debugging.hpp => hpx/modules/debugging.hpp
    hpx/util/backtrace.hpp => hpx/modules/debugging.hpp
    hpx/util/backtrace/backtrace.hpp => hpx/modules/debugging.hpp
    hpx/util/debug/demangle_helper.hpp => hpx/modules/debugging.hpp
)
# cmake-format: on

set(debugging_sources attach_debugger.cpp backtrace.cpp detail/dbghelp_lock.cpp
                      detail/dbghelp_symbol_cache.cpp print.cpp
)

include(HPX_AddModule)
add_hpx_module(
  core debugging
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${debugging_sources}
  HEADERS ${debugging_headers}
  MACRO_HEADERS ${debugging_macro_headers}
  COMPAT_HEADERS ${debugging_compat_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/debugging/backtrace/backtrace.hpp"
  DEPENDENCIES Asio::asio
  MODULE_DEPENDENCIES hpx_cache hpx_config
  CMAKE_SUBDIRS examples tests
)
