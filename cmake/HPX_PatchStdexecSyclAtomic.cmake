#  Copyright (c) 2026 Dominic Marcello
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The pinned stdexec selects CUDA atomics based only on header availability.
# CUDA 12.9 and Intel LLVM v6.0.1 have incompatible __assert_fail declarations.
# Add an opt-out that HPX exports for every translation unit in a SYCL build.
# Other builds retain stdexec's original header-selection behavior.

if(NOT DEFINED HPX_STDEXEC_ATOMIC_FILE)
  message(FATAL_ERROR "HPX_STDEXEC_ATOMIC_FILE must be defined")
endif()

file(READ "${HPX_STDEXEC_ATOMIC_FILE}" _hpx_stdexec_atomic_contents)
set(_hpx_stdexec_atomic_original "#    if __has_include(<cuda/std/atomic>)")
set(_hpx_stdexec_atomic_patched
    "#    if !defined(STDEXEC_NO_CUDA_STD_ATOMIC) && __has_include(<cuda/std/atomic>)"
)

string(FIND "${_hpx_stdexec_atomic_contents}"
       "${_hpx_stdexec_atomic_patched}" _hpx_stdexec_atomic_applied
)
if(NOT _hpx_stdexec_atomic_applied EQUAL -1)
  return()
endif()

string(FIND "${_hpx_stdexec_atomic_contents}"
       "${_hpx_stdexec_atomic_original}" _hpx_stdexec_atomic_position
)
if(_hpx_stdexec_atomic_position EQUAL -1)
  message(FATAL_ERROR "Cannot patch stdexec CUDA atomic selection for SYCL")
endif()

string(REPLACE "${_hpx_stdexec_atomic_original}"
               "${_hpx_stdexec_atomic_patched}" _hpx_stdexec_atomic_contents
               "${_hpx_stdexec_atomic_contents}"
)
file(WRITE "${HPX_STDEXEC_ATOMIC_FILE}" "${_hpx_stdexec_atomic_contents}")
