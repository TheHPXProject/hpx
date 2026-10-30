# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/testing/include
set(testing_headers hpx/testing/api.hpp hpx/testing/macros.hpp
                    hpx/testing/performance.hpp
)

set(testing_macro_headers hpx/testing/macros.hpp)

# Default location is $HPX_ROOT/libs/testing/include_compatibility
# cmake-format: off
set(testing_compat_headers
    hpx/testing.hpp => hpx/modules/testing.hpp
    hpx/util/lightweight_test.hpp => hpx/modules/testing.hpp
)
# cmake-format: on

# Default location is $HPX_ROOT/libs/testing/src
set(testing_sources testing.cpp performance.cpp)

set(module_deps
    hpx_assertion
    hpx_command_line_handling_local
    hpx_config
    hpx_format
    hpx_functional
    hpx_preprocessor
    hpx_thread_support
    hpx_util
)

if(HPX_WITH_NANOBENCH)
  set(module_deps ${module_deps} nanobench)
endif()

include(HPX_AddModule)
add_hpx_module(
  core testing
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${testing_sources}
  HEADERS ${testing_headers}
  MACRO_HEADERS ${testing_macro_headers}
  COMPAT_HEADERS ${testing_compat_headers}
  MODULE_DEPENDENCIES ${module_deps}
  CMAKE_SUBDIRS examples tests
)

# performance.cpp includes nanobench.h after HPX headers that include windows.h,
# whose min and max macros break nanobench.h
if(MSVC AND HPX_WITH_NANOBENCH)
  target_compile_definitions(hpx_testing PRIVATE NOMINMAX)
endif()
