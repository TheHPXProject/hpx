#  Copyright (c) 2026 Hartmut Kaiser
#  Copyright (c) 2026 Anshuman Agrawal
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Applies the nvcc compatibility workarounds to the fetched stdexec sources.
# stdexec's own GPU CI covers clang-CUDA and nvc++ only; without these patches
# no translation unit compiled by nvcc can include the HPX execution headers.
# See cmake/HPX_StdexecNvccWorkarounds.patch for the details.

if(NOT DEFINED HPX_STDEXEC_SOURCE_DIR)
  message(FATAL_ERROR "HPX_STDEXEC_SOURCE_DIR must be defined")
endif()

if(NOT DEFINED HPX_STDEXEC_NVCC_PATCH_FILE)
  message(FATAL_ERROR "HPX_STDEXEC_NVCC_PATCH_FILE must be defined")
endif()

# Do not modify fetched sources for compilers that do not need these
# workarounds. In particular, this keeps an upstream source change from breaking
# non-nvcc configurations merely because the nvcc patch is stale.
if(NOT HPX_STDEXEC_NVCC_PATCH_REQUIRED)
  return()
endif()

find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(
    FATAL_ERROR "git not found, cannot apply the stdexec nvcc workarounds"
  )
endif()

# Already applied (a reconfigure re-runs the patch command)?
execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero --reverse --check
          "${HPX_STDEXEC_NVCC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_nvcc_patch_applied
  OUTPUT_QUIET ERROR_QUIET
)
if(_hpx_stdexec_nvcc_patch_applied EQUAL 0)
  return()
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero
          "${HPX_STDEXEC_NVCC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_nvcc_patch_result
  ERROR_VARIABLE _hpx_stdexec_nvcc_patch_error
)
if(NOT _hpx_stdexec_nvcc_patch_result EQUAL 0)
  message(
    FATAL_ERROR
      "Failed to apply the stdexec nvcc workarounds: ${_hpx_stdexec_nvcc_patch_error}"
  )
endif()
