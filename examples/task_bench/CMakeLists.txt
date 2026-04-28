# Copyright (c) 2026 Pratyksh Gupta
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This example builds the HPX driver from the upstream task-bench repository
# (https://github.com/StanfordLegion/task-bench) as an external dependency. No
# task-bench source files are vendored here.

cmake_minimum_required(VERSION 3.17)

# task-bench core uses POSIX APIs (clock_gettime, etc.) that are not available
# on Windows. Skip the entire example on MSVC builds.
if(MSVC)
  return()
endif()

include(FetchContent)

# Fetch task-bench from upstream. Pin to the commit that includes the HPX driver
# so the build is reproducible.
fetchcontent_declare(
  task_bench
  GIT_REPOSITORY https://github.com/StanfordLegion/task-bench.git
  GIT_TAG master
  SOURCE_SUBDIR hpx
)

# Build the core library first (required by the HPX driver).
fetchcontent_getproperties(task_bench)
if(NOT task_bench_POPULATED)
  fetchcontent_populate(task_bench)
endif()

set(TASK_BENCH_CORE_DIR "${task_bench_SOURCE_DIR}/core")

# core_random.c and siphash.c are C files; enable C so cmake can compile them.
enable_language(C)

# Build the core static library.
add_library(
  task_bench_core STATIC
  "${TASK_BENCH_CORE_DIR}/core.cc"
  "${TASK_BENCH_CORE_DIR}/core_c.cc"
  "${TASK_BENCH_CORE_DIR}/core_kernel.cc"
  "${TASK_BENCH_CORE_DIR}/core_random.c"
  "${TASK_BENCH_CORE_DIR}/siphash.c"
  "${TASK_BENCH_CORE_DIR}/timer.cc"
)
target_include_directories(task_bench_core PUBLIC "${TASK_BENCH_CORE_DIR}")
target_compile_features(task_bench_core PUBLIC cxx_std_17)
set_target_properties(task_bench_core PROPERTIES OUTPUT_NAME "core_s")

# The HPX drivers include hpx/hpx.hpp and hpx/hpx_init.hpp, which are only
# available when the distributed runtime is enabled. Skip them otherwise.
if(COMMAND add_hpx_executable)
  if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
    return()
  endif()
endif()

# Build the HPX fork-join driver (intra-node parallelism, no MPI required).
if(COMMAND add_hpx_executable)
  add_hpx_executable(
    hpx_fork_join
    SOURCES "${task_bench_SOURCE_DIR}/hpx/hpx_fork_join.cc"
    FOLDER "Examples/TaskBench"
  )
  # hpx_include carries the libs/full/include/include path (and all its
  # transitive module deps such as hpx_segmented_algorithms) that hpx/hpx.hpp
  # requires. It is only defined when HPX_WITH_DISTRIBUTED_RUNTIME=ON, which is
  # already guaranteed by the guard above.
  target_link_libraries(hpx_fork_join PRIVATE hpx_include)
else()
  find_package(HPX REQUIRED)
  add_executable(hpx_fork_join "${task_bench_SOURCE_DIR}/hpx/hpx_fork_join.cc")
  target_link_libraries(hpx_fork_join PUBLIC HPX::hpx HPX::wrap_main)
endif()
target_include_directories(hpx_fork_join PRIVATE "${TASK_BENCH_CORE_DIR}")
target_link_libraries(hpx_fork_join PUBLIC task_bench_core)
add_hpx_example_target_dependencies("task_bench" hpx_fork_join)

# Optionally build the distributed driver if MPI is available.
find_package(MPI)
if(MPI_FOUND)
  if(COMMAND add_hpx_executable)
    add_hpx_executable(
      hpx_distributed
      SOURCES "${task_bench_SOURCE_DIR}/hpx/hpx_distributed.cc"
      FOLDER "Examples/TaskBench"
    )
    target_link_libraries(hpx_distributed PRIVATE hpx_include)
  else()
    add_executable(
      hpx_distributed "${task_bench_SOURCE_DIR}/hpx/hpx_distributed.cc"
    )
    target_link_libraries(hpx_distributed PUBLIC HPX::hpx HPX::wrap_main)
  endif()
  target_include_directories(hpx_distributed PRIVATE "${TASK_BENCH_CORE_DIR}")
  target_link_libraries(hpx_distributed PUBLIC task_bench_core MPI::MPI_C)
  add_hpx_example_target_dependencies("task_bench" hpx_distributed)
  # Suppress OpenMPI/MPICH C++ bindings header pulled in when <mpi.h> is
  # included from a C++ TU; hpx_distributed.cc only uses the C MPI API.
  target_compile_definitions(
    hpx_distributed PRIVATE OMPI_SKIP_MPICXX MPICH_SKIP_MPICXX
  )
endif()
