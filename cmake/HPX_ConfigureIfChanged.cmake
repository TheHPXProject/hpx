# Copyright (c) 2024 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Usage: configure_if_changed(<INPUT> <OUTPUT> [CONFIGURE_ARGS...])
#
# Example: configure_if_changed("${CMAKE_CURRENT_SOURCE_DIR}/template.in"
# "${CMAKE_CURRENT_BINARY_DIR}/output.txt" @ONLY)
function(hpx_configure_if_changed)
  set(options)
  set(oneValueArgs INPUT OUTPUT)
  set(multiArgs CONFIGURE_ARGS)
  cmake_parse_arguments(
    _cfg "${options}" "${oneValueArgs}" "${multiArgs}" ${ARGN}
  )

  if(NOT _cfg_INPUT)
    hpx_error("hpx_configure_if_changed: INPUT argument not specified")
  endif()
  if(NOT _cfg_OUTPUT)
    hpx_error("hpx_configure_if_changed: OUTPUT argument not specified")
  endif()

  set(tmp "${_cfg_OUTPUT}.tmp")

  # Run configure_file into temp file. If user passed additional args, append
  # them.
  if(_cfg_CONFIGURE_ARGS)
    configure_file("${_cfg_INPUT}" "${tmp}" ${_cfg_CONFIGURE_ARGS})
  else()
    configure_file("${_cfg_INPUT}" "${tmp}")
  endif()

  if(NOT EXISTS "${_cfg_OUTPUT}")
    file(RENAME "${tmp}" "${_cfg_OUTPUT}")
    return()
  endif()

  file(READ "${_cfg_OUTPUT}" _old CONTENTS)
  file(READ "${tmp}" _new CONTENTS)

  if(NOT _old STREQUAL _new)
    # atomic replace where possible
    file(RENAME "${tmp}" "${_cfg_OUTPUT}")
  else()
    file(REMOVE "${tmp}")
  endif()
endfunction()
