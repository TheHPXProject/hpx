#  Copyright (c) 2026 Hartmut Kaiser
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This patch file works around a problem seen on ARM gcc 11.4.0, see:
# https://github.com/NVIDIA/stdexec/issues/2150

if(NOT DEFINED HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE)
  message(FATAL_ERROR "HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE must be defined")
endif()

if(NOT EXISTS "${HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE}")
  message(FATAL_ERROR "File not found: ${HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE}")
endif()

file(READ "${HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE}"
     _stdexec_spin_loop_pause_contents
)

set(_stdexec_spin_loop_pause_legacy "STDEXEC_ATTRIBUTE(always_inline) inline")
string(FIND "${_stdexec_spin_loop_pause_contents}"
            "${_stdexec_spin_loop_pause_legacy}"
            _stdexec_spin_loop_pause_legacy_pos
)

if(NOT _stdexec_spin_loop_pause_legacy_pos EQUAL -1)
  string(
    REPLACE "${_stdexec_spin_loop_pause_legacy}"
            "STDEXEC_ATTRIBUTE(always_inline)"
            _stdexec_spin_loop_pause_contents
            "${_stdexec_spin_loop_pause_contents}"
  )
  file(WRITE "${HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE}"
       "${_stdexec_spin_loop_pause_contents}"
  )
  return()
endif()

# Current stdexec versions already use the corrected declaration. Verify both
# parts explicitly so an upstream rewrite does not silently skip the workaround.
string(FIND "${_stdexec_spin_loop_pause_contents}"
            "STDEXEC_ATTRIBUTE(always_inline)"
            _stdexec_spin_loop_pause_attribute_pos
)
string(FIND "${_stdexec_spin_loop_pause_contents}"
            "void __spin_loop_pause() noexcept"
            _stdexec_spin_loop_pause_function_pos
)
if(_stdexec_spin_loop_pause_attribute_pos EQUAL -1
   OR _stdexec_spin_loop_pause_function_pos EQUAL -1
)
  message(
    WARNING "Failed to find either the legacy or corrected __spin_loop_pause "
            "declaration in ${HPX_STDEXEC_SPIN_LOOP_PAUSE_FILE}"
  )
endif()
