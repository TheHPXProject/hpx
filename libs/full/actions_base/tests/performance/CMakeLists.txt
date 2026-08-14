# Copyright (c) 2026 Priyanshi Sharma
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_CXX26_REFLECTION AND Python3_EXECUTABLE)
  set(benchmark_script
      "${PROJECT_SOURCE_DIR}/libs/full/actions_base/tests/benchmarks/compile_time_benchmark.py"
  )
  add_hpx_performance_test(
    "modules.actions_base"
    compile_time_reflect_action
    EXECUTABLE
    ${Python3_EXECUTABLE}
    ARGS
    ${benchmark_script}
    10
    50
    100
    TIMEOUT
    600
  )
endif()
