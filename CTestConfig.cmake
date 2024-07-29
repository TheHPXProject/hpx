# Copyright (c) 2014 Thomas Heller
# Copyright (c) 2024 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# This file should be placed in the root directory of your project. Then modify
# the CMakeLists.txt file in the root directory of your project to incorporate
# the testing dashboard.
# The following are required to uses Dart and the dashboard
#    ENABLE_TESTING()
#    INCLUDE(CTest)
#
set(CTEST_PROJECT_NAME "HPX")
set(CTEST_NIGHTLY_START_TIME "00:00:00 GMT")

if(CMAKE_VERSION VERSION_GREATER 3.14)
  set(CTEST_SUBMIT_URL
      "https://cdash.rostam.cct.lsu.edu//submit.php?project=${CTEST_PROJECT_NAME}"
  )
else()
  set(CTEST_DROP_METHOD "https")
  set(CTEST_DROP_SITE "cdash.rostam.cct.lsu.edu")
  set(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
endif()

set(CTEST_DROP_SITE_CDASH TRUE)
