#  Copyright (c) 2026 Hartmut Kaiser
#  Copyright (c) 2026 Anshuman Agrawal
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Applies a compatibility patch to the fetched stdexec sources. An optional
# source probe lets revisions that predate the affected code skip the patch.

if(NOT DEFINED HPX_STDEXEC_SOURCE_DIR)
  message(FATAL_ERROR "HPX_STDEXEC_SOURCE_DIR must be defined")
endif()

if(NOT DEFINED HPX_STDEXEC_PATCH_FILE)
  message(FATAL_ERROR "HPX_STDEXEC_PATCH_FILE must be defined")
endif()

find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "git not found, cannot patch the fetched stdexec sources")
endif()

# Already applied (a reconfigure re-runs the patch command)?
execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero --reverse --check
          "${HPX_STDEXEC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_patch_applied
  OUTPUT_QUIET ERROR_QUIET
)
if(_hpx_stdexec_patch_applied EQUAL 0)
  return()
endif()

if(DEFINED HPX_STDEXEC_PATCH_PROBE_FILE OR DEFINED
                                           HPX_STDEXEC_PATCH_PROBE_STRING
)
  if(NOT DEFINED HPX_STDEXEC_PATCH_PROBE_FILE OR NOT DEFINED
                                                 HPX_STDEXEC_PATCH_PROBE_STRING
  )
    message(
      FATAL_ERROR
        "HPX_STDEXEC_PATCH_PROBE_FILE and HPX_STDEXEC_PATCH_PROBE_STRING must be defined together"
    )
  endif()

  if(NOT EXISTS "${HPX_STDEXEC_PATCH_PROBE_FILE}")
    message(
      FATAL_ERROR
        "Stdexec patch probe file does not exist: ${HPX_STDEXEC_PATCH_PROBE_FILE}"
    )
  endif()

  file(READ "${HPX_STDEXEC_PATCH_PROBE_FILE}" _hpx_stdexec_patch_probe_contents)
  string(FIND "${_hpx_stdexec_patch_probe_contents}"
              "${HPX_STDEXEC_PATCH_PROBE_STRING}"
              _hpx_stdexec_patch_probe_result
  )
  if(_hpx_stdexec_patch_probe_result EQUAL -1)
    return()
  endif()
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero "${HPX_STDEXEC_PATCH_FILE}"
  WORKING_DIRECTORY "${HPX_STDEXEC_SOURCE_DIR}"
  RESULT_VARIABLE _hpx_stdexec_patch_result
  ERROR_VARIABLE _hpx_stdexec_patch_error
)
if(NOT _hpx_stdexec_patch_result EQUAL 0)
  message(
    FATAL_ERROR
      "Failed to patch the fetched stdexec sources: ${_hpx_stdexec_patch_error}"
  )
endif()
