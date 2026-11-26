# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The smoke test of the parcel area: runs the tool for TCP on this node (its two
# localities started by hpxrun.py), tests.unit.tools.tuners.hpx_tune_parcel
if(HPX_WITH_DISTRIBUTED_RUNTIME AND HPX_WITH_NETWORKING)
  add_hpx_executable(
    hpx_tune_parcel_test INTERNAL_FLAGS
    SOURCES parcel_area.cpp
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Unit/Tools/Tuners"
  )

  add_dependencies(hpx_tune_parcel_test hpx_tune)
  target_compile_definitions(
    hpx_tune_parcel_test
    PRIVATE "HPX_TUNE_EXECUTABLE=\"$<TARGET_FILE:hpx_tune>\""
            "HPX_TUNE_TEST_DIRECTORY=\"${CMAKE_CURRENT_BINARY_DIR}\""
  )

  # the tool's localities use ports: not in parallel with other tests
  add_hpx_unit_test("tools.tuners" hpx_tune_parcel RUN_SERIAL)
endif()
