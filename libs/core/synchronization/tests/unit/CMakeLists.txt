# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    async_rw_mutex
    barrier_cpp20
    binary_semaphore_cpp20
    channel_mpmc_fib
    channel_mpmc_shift
    channel_mpsc_fib
    channel_mpsc_shift
    channel_spsc_fib
    channel_spsc_shift
    condition_variable
    counting_semaphore
    counting_semaphore_cpp20
    in_place_stop_token
    in_place_stop_token_cb2
    latch_cpp20
    local_latch
    local_barrier
    local_barrier_count_up
    local_barrier_reset
    local_event
    local_mutex
    sliding_semaphore
    stop_token
    stop_token_cb2
)

set(async_rw_mutex_PARAMETERS THREADS_PER_LOCALITY 4)
set(barrier_cpp20_PARAMETERS THREADS_PER_LOCALITY 4)
set(binary_semaphore_cpp20_PARAMETERS THREADS_PER_LOCALITY 4)
set(channel_mpmc_fib_PARAMETERS THREADS_PER_LOCALITY 4)
set(channel_mpmc_shift_PARAMETERS THREADS_PER_LOCALITY 4)
set(channel_mpsc_fib_PARAMETERS THREADS_PER_LOCALITY 4)
set(channel_mpsc_shift_PARAMETERS THREADS_PER_LOCALITY 4)
set(channel_spsc_fib_PARAMETERS THREADS_PER_LOCALITY 4)
set(channel_spsc_shift_PARAMETERS THREADS_PER_LOCALITY 4)

set(counting_semaphore_PARAMETERS THREADS_PER_LOCALITY 4)
set(counting_semaphore_cpp20_PARAMETERS THREADS_PER_LOCALITY 4)

set(latch_cpp20_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_barrier_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_latch_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_event_PARAMETERS THREADS_PER_LOCALITY 4)
set(local_mutex_PARAMETERS THREADS_PER_LOCALITY 4)

set(sliding_semaphore_PARAMETERS THREADS_PER_LOCALITY 4)

set(stop_token_cb2_PARAMETERS THREADS_PER_LOCALITY 4)
set(stop_token_PARAMETERS THREADS_PER_LOCALITY 4)

set(in_place_stop_token_cb2_PARAMETERS THREADS_PER_LOCALITY 4)
set(in_place_stop_token_PARAMETERS THREADS_PER_LOCALITY 4)

foreach(test ${tests})

  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Synchronization"
  )

  add_hpx_unit_test("modules.synchronization" ${test} ${${test}_PARAMETERS})

endforeach()

if(HPX_WITH_COMPILE_ONLY_TESTS)
  # add compile time tests
  set(compile_tests)

  if(HPX_WITH_FAIL_COMPILE_TESTS)
    set(fail_compile_tests fail_compile_mutex_move fail_compile_spinlock_move
                           fail_compile_spinlock_no_backoff_move
    )
    foreach(fail_compile_test ${fail_compile_tests})
      set(${fail_compile_test}_FLAGS FAILURE_EXPECTED)
    endforeach()

    set(compile_tests ${compile_tests} ${fail_compile_tests})
  endif()

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    add_hpx_unit_compile_test(
      "modules.synchronization" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Unit/Modules/Core/Synchronization/CompileOnly"
    )

  endforeach()

endif()

# ##############################################################################
set(subdirs shared_mutex range_mutex)

# add sub directories
foreach(subdir ${subdirs})
  add_hpx_pseudo_target(tests.unit.modules.synchronization.${subdir})
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(
    tests.unit.modules.synchronization
    tests.unit.modules.synchronization.${subdir}
  )
endforeach()
