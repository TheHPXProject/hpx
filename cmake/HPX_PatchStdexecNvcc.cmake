#  Copyright (c) 2026 Hartmut Kaiser
#  Copyright (c) 2026 Anshuman Agrawal
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Applies one compatibility patch, HPX_STDEXEC_PATCH_FILE, to the fetched
# stdexec sources in HPX_STDEXEC_SOURCE_DIR. HPX_SetupStdexec.cmake runs this
# script once for each of these patches (see each patch file for the details):
#
# * HPX_StdexecNvccWorkarounds.patch: lets nvcc parse the HPX execution headers
#   (stdexec's own GPU CI covers clang-CUDA and nvc++ only)
# * HPX_StdexecInlineSchedulerNvcc.patch: inline_scheduler's static_assert under
#   nvcc
# * HPX_StdexecAsyncScopeNvcc.patch: exec::async_scope::on_empty() under nvcc
# * HPX_StdexecNvccGcc13.patch: sender instantiations with nvcc and a GCC 13
#   host compiler
# * HPX_StdexecNvccDiagnostics.patch: nonvirtual diagnostics for nvcc's
#   host-side HPX sender chains
# * HPX_StdexecCudaStdAtomic.patch: opt-out of CUDA's atomics for builds without
#   CUDA and for SYCL builds

if(NOT DEFINED HPX_STDEXEC_SOURCE_DIR)
  message(FATAL_ERROR "HPX_STDEXEC_SOURCE_DIR must be defined")
endif()

if(NOT DEFINED HPX_STDEXEC_PATCH_FILE)
  message(FATAL_ERROR "HPX_STDEXEC_PATCH_FILE must be defined")
endif()

# HPX_STDEXEC_PATCH_REQUIRED says whether the selected configuration needs this
# patch (nvcc for the nvcc patches, SYCL for the CUDA atomic opt-out). Fail hard
# there; warn elsewhere so unrelated configurations can use another stdexec
# revision.
if(HPX_STDEXEC_PATCH_REQUIRED)
  set(_hpx_stdexec_patch_severity FATAL_ERROR)
else()
  set(_hpx_stdexec_patch_severity WARNING)
endif()

find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(${_hpx_stdexec_patch_severity}
          "git not found, cannot apply stdexec patch: ${HPX_STDEXEC_PATCH_FILE}"
  )
  return()
endif()

# Already applied (a reconfigure re-runs the patch command)?
execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --reverse --check
          "${HPX_STDEXEC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_patch_applied
  OUTPUT_QUIET ERROR_QUIET
)
if(_hpx_stdexec_patch_applied EQUAL 0)
  return()
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply "${HPX_STDEXEC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_patch_result
  ERROR_VARIABLE _hpx_stdexec_patch_error
)
if(NOT _hpx_stdexec_patch_result EQUAL 0)
  message(
    ${_hpx_stdexec_patch_severity}
    "Failed to apply stdexec patch ${HPX_STDEXEC_PATCH_FILE}: ${_hpx_stdexec_patch_error}"
  )
endif()
