# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/timing/include
set(timing_headers
    hpx/timing/durations.hpp hpx/timing/high_resolution_clock.hpp
    hpx/timing/high_resolution_timer.hpp hpx/timing/scoped_timer.hpp
    hpx/timing/steady_clock.hpp hpx/timing/tick_counter.hpp
)

# Default location is $HPX_ROOT/libs/timing/include_compatibility
# cmake-format: off
set(timing_compat_headers
    hpx/timing.hpp => hpx/chrono.hpp
    hpx/util/high_resolution_clock.hpp => hpx/chrono.hpp
    hpx/util/high_resolution_timer.hpp => hpx/chrono.hpp
    hpx/util/scoped_timer.hpp => hpx/chrono.hpp
    hpx/util/steady_clock.hpp => hpx/chrono.hpp
    hpx/util/tick_counter.hpp => hpx/modules/timing.hpp
)
# cmake-format: on

# cmake will not create a separate VS project without any source files, thus
# this adds a dummy (empty) source file to the target Default location is
# $HPX_ROOT/libs/timing/src
set(timing_sources)

include(HPX_AddModule)
add_hpx_module(
  core timing
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${timing_sources}
  HEADERS ${timing_headers}
  COMPAT_HEADERS ${timing_compat_headers}
  MODULE_DEPENDENCIES hpx_config hpx_hardware hpx_type_support
  CMAKE_SUBDIRS examples tests
)
