# Copyright (c) 2019-2023 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/config/include
set(config_macro_headers
    hpx/config.hpp
    hpx/config/asio.hpp
    hpx/config/attributes.hpp
    hpx/config/autolink.hpp
    hpx/config/auto_vectorization.hpp
    hpx/config/branch_hints.hpp
    hpx/config/compiler_fence.hpp
    hpx/config/compiler_specific.hpp
    hpx/config/constexpr.hpp
    hpx/config/debug.hpp
    hpx/config/deprecation.hpp
    hpx/config/detail/compat_error_code.hpp
    hpx/config/emulate_deleted.hpp
    hpx/config/endian.hpp
    hpx/config/export_definitions.hpp
    hpx/config/forceinline.hpp
    hpx/config/forward.hpp
    hpx/config/manual_profiling.hpp
    hpx/config/move.hpp
    hpx/config/static_linker_check.hpp
    hpx/config/threads_stack.hpp
    hpx/config/warnings_prefix.hpp
    hpx/config/warnings_suffix.hpp
    hpx/config/weak_symbol.hpp
)

set(config_headers ${config_macro_headers})

# Default location is $HPX_ROOT/libs/config/src
set(config_sources version.cpp)

include(HPX_AddModule)
add_hpx_module(
  core config CONFIG_FILES
  GLOBAL_HEADER_GEN OFF
  GLOBAL_HEADER_MODULE_GEN OFF
  SOURCES ${config_sources}
  HEADERS ${config_headers}
  EXCLUDE_FROM_GLOBAL_HEADER hpx/modules/config.hpp
  MODULE_DEPENDENCIES hpx_preprocessor
  DEPENDENCIES hpx_dependencies_boost
  CMAKE_SUBDIRS examples tests
)
