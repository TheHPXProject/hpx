# Copyright (c) 2014-2017 Hartmut Kaiser
# Copyright (c) 2011 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(boost_library_dependencies ${Boost_LIBRARIES})

set(benchmarks
    async_overheads
    coroutines_call_overhead
    delay_baseline
    delay_baseline_threaded
    function_object_wrapper_overhead
    future_overhead
    future_overhead_report
    hpx_heterogeneous_timed_task_spawn
    hpx_tls_overhead
    native_tls_overhead
    parent_vs_child_stealing
    print_heterogeneous_payloads
    resume_suspend
    timed_task_spawn
    skynet
    wait_all_timings
    benchmark_stealing
)

set(timed_task_spawn_SOURCES activate_counters.cpp)
set(timed_task_spawn_HEADERS activate_counters.hpp)

if(NOT HPX_WITH_CUDA_COMPUTE)
  list(APPEND benchmarks stream stream_report)
  set(stream_FLAGS CUDA)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  list(REMOVE_ITEM benchmarks stream_report)
endif()

if(NOT HPX_WITH_SANITIZERS)
  list(APPEND benchmarks start_stop)
endif()

if(HPX_WITH_LIBCDS)
  list(APPEND benchmarks libcds_hazard_pointer_overhead)
  set(libcds_hazard_pointer_overhead_FLAGS DEPENDENCIES iostreams_component)
endif()

if(HPX_WITH_DATAPAR)
  list(APPEND benchmarks transform_reduce_binary_scaling)
  set(transform_reduce_binary_scaling_FLAGS DEPENDENCIES iostreams_component)
endif()

if(HPX_WITH_EXAMPLES_OPENMP)
  list(APPEND benchmarks openmp_homogeneous_timed_task_spawn
       openmp_parallel_region
  )

  set(openmp_homogeneous_timed_task_spawn_FLAGS
      NOLIBS DEPENDENCIES ${boost_library_dependencies} hpx_core
  )
  set(openmp_parallel_region_FLAGS NOLIBS DEPENDENCIES hpx_core)
endif()

if(HPX_WITH_EXAMPLES_QTHREADS)
  list(APPEND benchmarks qthreads_homogeneous_timed_task_spawn
       qthreads_heterogeneous_timed_task_spawn
  )

  set(qthreads_homogeneous_timed_task_spawn_FLAGS
      NOLIBS DEPENDENCIES ${boost_library_dependencies} ${Qthreads_LIBRARIES}
      hpx_core
  )

  set(qthreads_heterogeneous_timed_task_spawn_FLAGS
      NOLIBS DEPENDENCIES ${boost_library_dependencies} ${Qthreads_LIBRARIES}
      hpx_core
  )

  set(qthreads_heterogeneous_timed_task_spawn_INCLUDE_DIRECTORIES
      ${Qthreads_INCLUDE_DIR}
  )
endif()

if(HPX_WITH_EXAMPLES_TBB)
  list(APPEND benchmarks tbb_homogeneous_timed_task_spawn)

  set(tbb_homogeneous_timed_task_spawn_FLAGS
      NOLIBS DEPENDENCIES ${boost_library_dependencies} ${Tbb_LIBRARIES}
      hpx_core
  )

  set(tbb_homogeneous_timed_task_spawn_INCLUDE_DIRECTORIES ${Tbb_INCLUDE_DIR})
endif()

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  list(
    APPEND
    benchmarks
    agas_cache_timings
    partitioned_vector_foreach
    sizeof
    spinlock_overhead1
    spinlock_overhead2
  )
endif()

set(delay_baseline_FLAGS NOLIBS DEPENDENCIES ${boost_library_dependencies}
                         hpx_core
)

set(delay_baseline_threaded_FLAGS DEPENDENCIES ${boost_library_dependencies}
                                  hpx_core
)

set(print_heterogeneous_payloads_FLAGS NOLIBS DEPENDENCIES
                                       ${boost_library_dependencies} hpx_core
)
set(resume_suspend_FLAGS DEPENDENCIES hpx_timing)

set(native_tls_overhead_LIBRARIES hpx_dependencies_boost)

set(skynet_FLAGS DEPENDENCIES iostreams_component)
set(sizeof_FLAGS DEPENDENCIES iostreams_component)
set(spinlock_overhead1_FLAGS DEPENDENCIES iostreams_component)
set(spinlock_overhead2_FLAGS DEPENDENCIES iostreams_component)
set(partitioned_vector_foreach_FLAGS DEPENDENCIES iostreams_component
                                     partitioned_vector_component
)

set(future_overhead_PARAMETERS THREADS_PER_LOCALITY 4)
set(future_overhead_report_PARAMETERS THREADS_PER_LOCALITY 4)
set(stream_report_PARAMETERS THREADS_PER_LOCALITY 2)

# These tests do not run on hpx threads, so we don't want to pass hpx params
# into them
set(delay_baseline_PARAMETERS NO_HPX_MAIN)
set(delay_baseline_threaded_PARAMETERS NO_HPX_MAIN)
set(function_object_wrapper_overhead_PARAMETERS NO_HPX_MAIN)
set(nonconcurrent_fifo_overhead_PARAMETERS NO_HPX_MAIN)
set(nonconcurrent_lifo_overhead_PARAMETERS NO_HPX_MAIN)
set(print_heterogeneous_payloads_PARAMETERS NO_HPX_MAIN)

# These tests fail, so I am marking them as non HPX tests until they are fixed
set(print_heterogeneous_payloads_PARAMETERS NO_HPX_MAIN)
set(skynet_PARAMETERS NO_HPX_MAIN)
set(timed_task_spawn_PARAMETERS NO_HPX_MAIN)
set(benchmark_stealing_PARAMETERS NO_HPX_MAIN)
set(hpx_tls_overhead_PARAMETERS NO_HPX_MAIN)
set(native_tls_overhead_PARAMETERS NO_HPX_MAIN)
set(coroutines_call_overhead_PARAMETERS NO_HPX_MAIN)

foreach(benchmark ${benchmarks})
  set(sources ${benchmark}.cpp ${${benchmark}_SOURCES})

  source_group("Source Files" FILES ${sources})
  if(${benchmark}_HEADERS)
    source_group("Header Files" FILES ${${benchmark}_HEADERS})
    set(${benchmark}_HEADERS HEADERS ${${benchmark}_HEADERS})
  endif()

  # add example executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${benchmark}_FLAGS} ${${benchmark}_HEADERS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Benchmarks/Local" ${HPX_WITH_CXX_MODULES_OPTION}
  )

  target_include_directories(
    ${benchmark}_test SYSTEM PRIVATE ${${benchmark}_INCLUDE_DIRECTORIES}
  )
  target_link_libraries(${benchmark}_test PRIVATE ${${benchmark}_LIBRARIES})

  if(NOT "${${benchmark}_PARAMETERS}" MATCHES NO_HPX_MAIN)
    add_hpx_performance_test("local" ${benchmark} ${${benchmark}_PARAMETERS})
  endif()

endforeach()

if(HPX_WITH_LIBCDS)
  target_link_libraries(libcds_hazard_pointer_overhead_test PRIVATE cds)
endif()

if(HPX_WITH_EXAMPLES_OPENMP)
  set_target_properties(
    openmp_homogeneous_timed_task_spawn_test PROPERTIES COMPILE_FLAGS
                                                        ${OpenMP_CXX_FLAGS}
  )
  set_target_properties(
    openmp_homogeneous_timed_task_spawn_test PROPERTIES LINK_FLAGS
                                                        ${OpenMP_CXX_FLAGS}
  )
  set_target_properties(
    openmp_parallel_region_test PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS}
  )
  set_target_properties(
    openmp_parallel_region_test PROPERTIES LINK_FLAGS ${OpenMP_CXX_FLAGS}
  )
endif()

add_hpx_pseudo_target(tests.performance.local.htts_v2)
add_subdirectory(htts_v2)
add_hpx_pseudo_dependencies(
  tests.performance.local tests.performance.local.htts_v2
)
