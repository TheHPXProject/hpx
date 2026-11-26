# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# hpx_tune_idle: measures the machine and writes the scheduler's idle
# settings as an ini file (see the tuning module, libs/core/tuning)
add_hpx_executable(
  hpx_tune_idle INTERNAL_FLAGS
  SOURCES hpx_tune_idle.cpp
  NOLIBS
  FOLDER "Tools/Tuners"
)

target_include_directories(hpx_tune_idle PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(hpx_tune_idle PRIVATE hpx_core)

add_hpx_pseudo_dependencies(tools.tuners hpx_tune_idle)
