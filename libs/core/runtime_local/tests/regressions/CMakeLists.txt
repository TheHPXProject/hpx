# Copyright (c) 2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(UNIX)
  add_hpx_executable(
    abort_signal_test INTERNAL_FLAGS
    SOURCES abort_signal.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/RuntimeLocal"
  )

  add_hpx_regression_test(
    "modules.runtime_local" abort_signal
    THREADS_PER_LOCALITY 2
    TIMEOUT 30
  )
endif()
