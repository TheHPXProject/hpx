# Copyright (c) 2007-2017 Hartmut Kaiser
# Copyright (c) 2011      Bryce Lelbach
# Copyright (c) 2011      Dylan Stark
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(subdirs
    1d_stencil
    accumulators
    async_io
    balancing
    cancelable_action
    future_reduce
    heartbeat
    jacobi
    jacobi_smp
    nqueen
    performance_counters
    pipeline
    quickstart
    qt
    random_mem_access
    spell_check
    startup_shutdown
    task_bench
    throttle
    tuplespace
    transpose
)

if(HPX_WITH_EXAMPLES_HDF5)
  set(subdirs ${subdirs} interpolate1d sheneos)
endif()

if(HPX_WITH_APEX)
  set(subdirs ${subdirs} apex)
endif()

foreach(subdir ${subdirs})
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.${subdir})
    add_hpx_pseudo_dependencies(tests.examples tests.examples.${subdir})
  endif()

  add_hpx_pseudo_target(examples.${subdir})
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(examples examples.${subdir})
endforeach()
