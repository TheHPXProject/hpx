# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The smoke test of the chunk area: tests.unit.tools.tuners.hpx_tune_chunk

add_hpx_executable(
  hpx_tune_chunk_test INTERNAL_FLAGS
  SOURCES hpx_tune_chunk.cpp
  EXCLUDE_FROM_ALL
  FOLDER "Tests/Unit/Tools/Tuners"
)

# the tool it runs, and where it writes its files
add_dependencies(hpx_tune_chunk_test hpx_tune)
target_compile_definitions(
  hpx_tune_chunk_test
  PRIVATE "HPX_TUNE_EXECUTABLE=\"$<TARGET_FILE:hpx_tune>\""
          "HPX_TUNE_TEST_DIRECTORY=\"${CMAKE_CURRENT_BINARY_DIR}\""
)

add_hpx_unit_test("tools.tuners" hpx_tune_chunk THREADS_PER_LOCALITY 2)
