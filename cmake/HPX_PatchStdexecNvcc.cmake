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

# Only an nvcc build is broken by an unpatched stdexec, and
# HPX_STDEXEC_NVCC_PATCH_REQUIRED says whether this is one. Fail hard there;
# warn everywhere else rather than failing configuration for users whose
# compiler parses stdexec unpatched.
if(HPX_STDEXEC_NVCC_PATCH_REQUIRED)
  set(_hpx_stdexec_nvcc_patch_severity FATAL_ERROR)
else()
  set(_hpx_stdexec_nvcc_patch_severity WARNING)
endif()

find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(${_hpx_stdexec_nvcc_patch_severity}
          "git not found, cannot apply the stdexec nvcc workarounds"
  )
  return()
endif()

# Already applied (a reconfigure re-runs the patch command)?
execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --reverse --check
          "${HPX_STDEXEC_NVCC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_nvcc_patch_applied
  OUTPUT_QUIET ERROR_QUIET
)
if(_hpx_stdexec_nvcc_patch_applied EQUAL 0)
  return()
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply "${HPX_STDEXEC_NVCC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_nvcc_patch_result
  ERROR_VARIABLE _hpx_stdexec_nvcc_patch_error
)
if(NOT _hpx_stdexec_nvcc_patch_result EQUAL 0)
  message(
    ${_hpx_stdexec_nvcc_patch_severity}
    "Failed to apply the stdexec nvcc workarounds: ${_hpx_stdexec_nvcc_patch_error}"
  )
endif()
