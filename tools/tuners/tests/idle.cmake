# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The smoke test of the idle area, and of the parts all areas share (the driver,
# child.hpp, the report): tests.unit.tools.tuners.hpx_tune

add_hpx_executable(
  hpx_tune_test INTERNAL_FLAGS
  SOURCES hpx_tune.cpp ../child.cpp
  EXCLUDE_FROM_ALL
  FOLDER "Tests/Unit/Tools/Tuners"
)

# the tool it runs, and where it writes its files
add_dependencies(hpx_tune_test hpx_tune)
target_compile_definitions(
  hpx_tune_test
  PRIVATE "HPX_TUNE_EXECUTABLE=\"$<TARGET_FILE:hpx_tune>\""
          "HPX_TUNE_TEST_DIRECTORY=\"${CMAKE_CURRENT_BINARY_DIR}\""
          "HPX_TUNE_CMAKE_COMMAND=\"${CMAKE_COMMAND}\""
)

add_hpx_unit_test("tools.tuners" hpx_tune)

# The idle area end to end (tuner chapter 13, level 1): hpx_tune --check of
# files the tool wrote and of files written by hand, the pools' idle settings
# read back by its children: tests.unit.tools.tuners.hpx_tune_idle_end_to_end

add_hpx_executable(
  hpx_tune_idle_end_to_end_test INTERNAL_FLAGS
  SOURCES idle_end_to_end.cpp
  EXCLUDE_FROM_ALL
  FOLDER "Tests/Unit/Tools/Tuners"
)

add_dependencies(hpx_tune_idle_end_to_end_test hpx_tune)
target_compile_definitions(
  hpx_tune_idle_end_to_end_test
  PRIVATE "HPX_TUNE_EXECUTABLE=\"$<TARGET_FILE:hpx_tune>\""
          "HPX_TUNE_TEST_DIRECTORY=\"${CMAKE_CURRENT_BINARY_DIR}\""
)

add_hpx_unit_test("tools.tuners" hpx_tune_idle_end_to_end)
