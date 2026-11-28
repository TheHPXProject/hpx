# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# hpx_tune: measures the machine and writes HPX settings for it as an ini file,
# by area (see the tuning module, libs/core/tuning)
add_hpx_executable(
  hpx_tune INTERNAL_FLAGS
  SOURCES area.cpp child.cpp chunk_area.cpp hpx_tune.cpp idle_area.cpp
  HEADERS area.hpp child.hpp chunk_area.hpp idle_area.hpp
  NOLIBS
  FOLDER "Tools/Tuners"
)

target_include_directories(hpx_tune PRIVATE ${PROJECT_BINARY_DIR})
# the full library (the areas start the runtime with hpx/init.hpp), without
# wrapping main (NOLIBS): the tool's own process doesn't start the runtime
target_link_libraries(hpx_tune PRIVATE HPX::hpx)

add_hpx_pseudo_dependencies(tools.tuners hpx_tune)

# the smoke tests start the tool and HPX applications: only on request
if(HPX_WITH_TESTS
   AND HPX_WITH_TESTS_UNIT
   AND HPX_WITH_TUNING_TESTS
)
  add_subdirectory(tests)
endif()
