# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The smoke tests of hpx_tune, tests.unit.tools.tuners.*: they run the tool and
# check the files it writes. Each area has its tests in a file of its own,
# <area>.cmake in this directory (idle.cmake, ...), so that adding an area
# doesn't change this file.

add_hpx_pseudo_target(tests.unit.tools.tuners)
add_hpx_pseudo_dependencies(tests.unit tests.unit.tools.tuners)

file(GLOB tuner_area_tests CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cmake"
)
list(SORT tuner_area_tests)
foreach(area_tests ${tuner_area_tests})
  include(${area_tests})
endforeach()
