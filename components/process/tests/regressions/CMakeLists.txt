# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT WIN32)
  add_hpx_executable(
    posix_initializers_7646_test INTERNAL_FLAGS
    SOURCES posix_initializers_7646.cpp
    DEPENDENCIES process_component
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Components/Process"
  )
  add_hpx_regression_test("components.process" posix_initializers_7646)
endif()
