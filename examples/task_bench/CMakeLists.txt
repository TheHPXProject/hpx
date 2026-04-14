# Copyright (c) 2026 Pratyksh Gupta
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This example builds the HPX driver from the upstream task-bench repository
# (https://github.com/StanfordLegion/task-bench) as an external dependency.
# No task-bench source files are vendored here.

cmake_minimum_required(VERSION 3.17)

include(FetchContent)

# Fetch task-bench from upstream. Pin to the commit that includes the HPX
# driver so the build is reproducible.
FetchContent_Declare(
  task_bench
  GIT_REPOSITORY https://github.com/StanfordLegion/task-bench.git
  GIT_TAG        master
  SOURCE_SUBDIR  hpx
)

# Build the core library first (required by the HPX driver).
FetchContent_GetProperties(task_bench)
if(NOT task_bench_POPULATED)
  FetchContent_Populate(task_bench)
endif()

set(TASK_BENCH_CORE_DIR "${task_bench_SOURCE_DIR}/core")

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

# Build the HPX fork-join driver (intra-node parallelism, no MPI required).
if(COMMAND add_hpx_executable)
  add_hpx_executable(
    hpx_fork_join
    SOURCES "${task_bench_SOURCE_DIR}/hpx/hpx_fork_join.cc"
    FOLDER "Examples/TaskBench"
  )
else()
  find_package(HPX REQUIRED)
  add_executable(hpx_fork_join "${task_bench_SOURCE_DIR}/hpx/hpx_fork_join.cc")
  target_link_libraries(hpx_fork_join PUBLIC HPX::hpx HPX::wrap_main)
endif()
target_include_directories(hpx_fork_join PRIVATE "${TASK_BENCH_CORE_DIR}")
target_link_libraries(hpx_fork_join PUBLIC task_bench_core)

# Optionally build the distributed driver if MPI is available.
find_package(MPI)
if(MPI_FOUND)
  if(COMMAND add_hpx_executable)
    add_hpx_executable(
      hpx_distributed
      SOURCES "${task_bench_SOURCE_DIR}/hpx/hpx_distributed.cc"
      FOLDER "Examples/TaskBench"
    )
  else()
    add_executable(hpx_distributed
                   "${task_bench_SOURCE_DIR}/hpx/hpx_distributed.cc")
    target_link_libraries(hpx_distributed PUBLIC HPX::hpx HPX::wrap_main)
  endif()
  target_include_directories(hpx_distributed PRIVATE "${TASK_BENCH_CORE_DIR}")
  target_link_libraries(hpx_distributed PUBLIC task_bench_core MPI::MPI_CXX)
endif()
