# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2017 John Biddiscombe
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(CTEST_BUILD_PARALLELISM 20)
set(CTEST_CMAKE_GENERATOR Ninja)
set(CTEST_SITE "lsu(rostam)")
set(CTEST_UPDATE_COMMAND "git")
set(CTEST_UPDATE_VERSION_ONLY "ON")
set(CTEST_SUBMIT_RETRY_COUNT 5)
set(CTEST_SUBMIT_RETRY_DELAY 60)

if(NOT "$ENV{ghprbPullId}" STREQUAL "")
  set(CTEST_BUILD_NAME "$ENV{ghprbPullId}-${CTEST_BUILD_CONFIGURATION_NAME}")
  set(CTEST_TRACK "Pull_Requests")
else()
  set(CTEST_BUILD_NAME
      "$ENV{git_local_branch}-${CTEST_BUILD_CONFIGURATION_NAME}"
  )
  set(CTEST_TRACK "$ENV{git_local_branch}")
endif()

# Messages about what happens, in the format of perftest_log in report_common.sh
function(perftest_log)
  string(TIMESTAMP now "%H:%M:%S" UTC)
  list(JOIN ARGN "" text)
  message("[perftests ${now} ctest.cmake] ${text}")
endfunction()

ctest_start(Experimental TRACK "${CTEST_TRACK}")
perftest_log("CDash build ${CTEST_BUILD_NAME}, track ${CTEST_TRACK}")

ctest_update()
ctest_submit(
  PARTS Update
  BUILD_ID __ctest_build_id
  RETURN_VALUE __update_result
)

set(CTEST_CONFIGURE_COMMAND "${CMAKE_COMMAND} ${CTEST_SOURCE_DIRECTORY}")
set(CTEST_CONFIGURE_COMMAND
    "${CTEST_CONFIGURE_COMMAND} -G${CTEST_CMAKE_GENERATOR}"
)
set(CTEST_CONFIGURE_COMMAND
    "${CTEST_CONFIGURE_COMMAND} -B${CTEST_BINARY_DIRECTORY}"
)
set(CTEST_CONFIGURE_COMMAND
    "${CTEST_CONFIGURE_COMMAND} -DHPX_WITH_NANOBENCH=ON"
)
set(CTEST_CONFIGURE_COMMAND
    "${CTEST_CONFIGURE_COMMAND} -DHPX_WITH_PARALLEL_TESTS_BIND_NONE=ON"
)
set(CTEST_CONFIGURE_COMMAND
    "${CTEST_CONFIGURE_COMMAND} ${CTEST_CONFIGURE_EXTRA_OPTIONS}"
)

# The performance reports registered by add_hpx_performance_report_test, which
# the configure step lists in this file (see hpx_add_perftests_plot_target in
# cmake/HPX_AddTest.cmake)
set(perftest_reports_file
    "${CTEST_BINARY_DIRECTORY}/tools/perftests_ci/perftests.cmake"
)
file(REMOVE "${perftest_reports_file}")

perftest_log("Configuring HPX in ${CTEST_BINARY_DIRECTORY}")
ctest_configure()
ctest_submit(
  PARTS Configure
  BUILD_ID __ctest_build_id
  RETURN_VALUE __configure_result
)
if(NOT CTEST_BUILD_ID AND __ctest_build_id)
  set(CTEST_BUILD_ID ${__ctest_build_id})
endif()
set(ctest_submission_result ${ctest_submission_result} "Configure: "
                            ${__configure_result} "\n"
)

# Steps that failed: the configuration (no benchmarks to run), a benchmark that
# failed to build or to run, or results that couldn't be compared with their
# references. batch.sh turns the GitHub status red if there are any.
set(perftest_errors)
set(perftest_reports)
include("${perftest_reports_file}" OPTIONAL)
if(NOT perftest_reports)
  perftest_log("The configuration registered no performance tests")
  list(APPEND perftest_errors "configure")
else()
  list(LENGTH perftest_reports perftest_count)
  list(JOIN perftest_reports ", " perftest_list)
  perftest_log("${perftest_count} performance tests: ${perftest_list}")
  perftest_log("Building the benchmarks")
  # Build all benchmarks first, so that they are compiled in parallel instead of
  # one after the other between the runs of the performance tests below
  set(benchmark_executables)
  foreach(benchmark ${perftest_reports})
    list(APPEND benchmark_executables ${benchmark}_test)
  endforeach()
  list(JOIN benchmark_executables " " benchmark_executables)
  set(CTEST_BUILD_COMMAND
      "\"${CMAKE_COMMAND}\" --build \"${CTEST_BINARY_DIRECTORY}\" --target ${benchmark_executables} -- -k0 -j ${CTEST_BUILD_PARALLELISM}"
  )
  ctest_build(RETURN_VALUE __step_result)
  if(NOT __step_result EQUAL 0)
    perftest_log("Building some of the benchmarks failed")
    list(APPEND perftest_errors "build")
  endif()
  unset(CTEST_BUILD_COMMAND)

  set(perftest_index 0)
  foreach(benchmark ${perftest_reports})
    math(EXPR perftest_index "${perftest_index} + 1")
    perftest_log(
      "Running ${benchmark} (${perftest_index} of ${perftest_count})"
    )
    ctest_build(
      TARGET ${benchmark}_perftest
      FLAGS "-k0 -j ${CTEST_BUILD_PARALLELISM}"
      RETURN_VALUE __step_result
    )
    if(NOT __step_result EQUAL 0)
      perftest_log("${benchmark} failed")
      list(APPEND perftest_errors ${benchmark})
    endif()
  endforeach()

  # Compare all results with their references and plot them, once all benchmarks
  # have run
  perftest_log("Comparing the results with the references and plotting them")
  ctest_build(
    TARGET perftests_plot
    FLAGS "-k0"
    RETURN_VALUE __step_result
  )
  if(NOT __step_result EQUAL 0)
    perftest_log("The comparison failed")
    list(APPEND perftest_errors "perftests_plot")
  endif()
endif()

list(JOIN perftest_errors "\n" perftest_errors)
file(WRITE "jenkins-hpx-${CTEST_BUILD_CONFIGURATION_NAME}-perftest-errors.txt"
     "${perftest_errors}"
)

ctest_submit(
  PARTS Build
  BUILD_ID __ctest_build_id
  RETURN_VALUE __build_result
)
if(NOT CTEST_BUILD_ID AND __ctest_build_id)
  set(CTEST_BUILD_ID ${__ctest_build_id})
endif()
set(ctest_submission_result ${ctest_submission_result} "Build: "
                            ${__build_result} "\n"
)

# The benchmarks run as the build steps above, there are no tests to run: an
# empty ctest_test would make ctest fail ("No tests were found")

file(WRITE "jenkins-hpx-${CTEST_BUILD_CONFIGURATION_NAME}-cdash-build-id.txt"
     "${CTEST_BUILD_ID}"
)
file(WRITE "jenkins-hpx-${CTEST_BUILD_CONFIGURATION_NAME}-cdash-submission.txt"
     ${ctest_submission_result}
)
