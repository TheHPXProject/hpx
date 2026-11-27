# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)

# the module's unit, regression and performance tests only with the tuner's
# tests (HPX_WITH_TUNING_TESTS), the header tests always
if(HPX_WITH_TESTS)
  if(HPX_WITH_TESTS_UNIT AND HPX_WITH_TUNING_TESTS)
    add_hpx_pseudo_target(tests.unit.modules.parcel_tuning)
    add_hpx_pseudo_dependencies(
      tests.unit.modules tests.unit.modules.parcel_tuning
    )
    add_subdirectory(unit)
  endif()

  if(HPX_WITH_TESTS_REGRESSIONS AND HPX_WITH_TUNING_TESTS)
    add_hpx_pseudo_target(tests.regressions.modules.parcel_tuning)
    add_hpx_pseudo_dependencies(
      tests.regressions.modules tests.regressions.modules.parcel_tuning
    )
    add_subdirectory(regressions)
  endif()

  if(HPX_WITH_TESTS_BENCHMARKS AND HPX_WITH_TUNING_TESTS)
    add_hpx_pseudo_target(tests.performance.modules.parcel_tuning)
    add_hpx_pseudo_dependencies(
      tests.performance.modules tests.performance.modules.parcel_tuning
    )
    add_subdirectory(performance)
  endif()

  if(HPX_WITH_TESTS_HEADERS)
    add_hpx_header_tests(
      modules.parcel_tuning
      HEADERS ${parcel_tuning_headers}
      HEADER_ROOT ${PROJECT_SOURCE_DIR}/include
      DEPENDENCIES hpx_parcel_tuning
    )
  endif()
endif()
