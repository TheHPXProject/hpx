# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

hpx_option(
  HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION BOOL
  "Emulate SwapContext API for coroutines (Windows only, default: OFF)" OFF
  CATEGORY "Thread Manager"
  ADVANCED
  MODULE COROUTINES
)

hpx_option(
  HPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD
  BOOL
  "Futures attempt to run associated threads directly if those have not been started (default: OFF)"
  OFF
  CATEGORY "Thread Manager"
  ADVANCED
  MODULE COROUTINES
)

if(HPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD)
  hpx_add_config_define_namespace(
    DEFINE HPX_COROUTINES_HAVE_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD
    NAMESPACE COROUTINES
  )
endif()

set(coroutines_headers
    hpx/coroutines/coroutine.hpp
    hpx/coroutines/coroutine_fwd.hpp
    hpx/coroutines/stackless_coroutine.hpp
    hpx/coroutines/detail/combined_tagged_state.hpp
    hpx/coroutines/detail/context_base.hpp
    hpx/coroutines/detail/context_generic_context.hpp
    hpx/coroutines/detail/context_impl.hpp
    hpx/coroutines/detail/context_linux_x86.hpp
    hpx/coroutines/detail/context_posix.hpp
    hpx/coroutines/detail/context_windows_fibers.hpp
    hpx/coroutines/detail/coroutine_accessor.hpp
    hpx/coroutines/detail/coroutine_impl.hpp
    hpx/coroutines/detail/coroutine_self.hpp
    hpx/coroutines/detail/coroutine_stackful_self.hpp
    hpx/coroutines/detail/coroutine_stackful_self_direct.hpp
    hpx/coroutines/detail/coroutine_stackless_self.hpp
    hpx/coroutines/detail/get_stack_pointer.hpp
    hpx/coroutines/detail/posix_utility.hpp
    hpx/coroutines/detail/swap_context.hpp
    hpx/coroutines/detail/tss.hpp
    hpx/coroutines/signal_handler_debugging.hpp
    hpx/coroutines/thread_enums.hpp
    hpx/coroutines/thread_id_type.hpp
)

# cmake-format: off
set(coroutines_compat_headers
    hpx/coroutines.hpp => hpx/modules/coroutines.hpp
    hpx/runtime/threads/coroutines/coroutine.hpp => hpx/modules/coroutines.hpp
    hpx/runtime/threads/coroutines/coroutine_fwd.hpp => hpx/modules/coroutines.hpp
    hpx/runtime/threads/thread_enums.hpp => hpx/modules/coroutines.hpp
    hpx/runtime/threads/thread_id_type.hpp => hpx/modules/coroutines.hpp
)
# cmake-format: on

set(coroutines_sources
    detail/context_base.cpp
    detail/context_posix.cpp
    detail/coroutine_impl.cpp
    detail/coroutine_self.cpp
    detail/get_stack_pointer.cpp
    detail/posix_utility.cpp
    detail/tss.cpp
    swapcontext.cpp
    thread_enums.cpp
    thread_id_type.cpp
    signal_handler_debugging.cpp
)

if(MSVC)
  if(HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION)
    # ##########################################################################
    # Emulation of SwapContext on Windows
    # ##########################################################################
    if(NOT "${__target_arch}" STREQUAL "x86_64")
      hpx_error(
        "The option HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION is only \
         implemented for x86-64 targets, src/switch_to_fiber.asm is x86-64 \
         assembly, but the target architecture is ${__target_arch}. Please \
         disable the emulation by setting it to Off."
      )
    endif()

    enable_language(ASM_MASM)
    if(NOT CMAKE_ASM_MASM_COMPILER)
      hpx_error(
        "SwitchToFiber emulation can not be enabled. The masm compiler \
         could not be found. Try setting the ASM_MASM environment variable to the \
         assembler executable (ml.exe/ml64.exe) or disable the emulation by setting \
         HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION to Off"
      )
    endif()

    hpx_add_config_define_namespace(
      DEFINE HPX_COROUTINES_HAVE_SWAP_CONTEXT_EMULATION NAMESPACE COROUTINES
    )

    set(switch_to_fiber_source
        "${CMAKE_CURRENT_SOURCE_DIR}/src/switch_to_fiber.asm"
    )
    set(switch_to_fiber_object
        "${CMAKE_CURRENT_BINARY_DIR}/switch_to_fiber.obj"
    )
    add_custom_command(
      OUTPUT "${switch_to_fiber_object}"
      COMMAND "${CMAKE_ASM_MASM_COMPILER}" /Fo "${switch_to_fiber_object}"
              /nologo /c "${switch_to_fiber_source}"
      DEPENDS "${switch_to_fiber_source}"
      VERBATIM
    )
  endif()
elseif(HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION)
  hpx_error(
    "The option HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION is not supported on "
    "this platform, please disable the emulation by setting it to Off"
  )
endif()

set_source_files_properties(
  src/detail/coroutine_impl.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE
)
set_source_files_properties(
  src/detail/coroutine_self.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE
)

include(HPX_AddModule)
add_hpx_module(
  core coroutines
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${coroutines_sources}
  HEADERS ${coroutines_headers}
  OBJECTS "${switch_to_fiber_object}"
  COMPAT_HEADERS ${coroutines_compat_headers}
  ADD_TO_GLOBAL_HEADER
    "hpx/coroutines/detail/coroutine_impl.hpp"
    "hpx/coroutines/detail/coroutine_accessor.hpp"
    "hpx/coroutines/detail/coroutine_stackful_self.hpp"
    "hpx/coroutines/detail/tss.hpp"
  MODULE_DEPENDENCIES
    hpx_assertion
    hpx_config
    hpx_debugging
    hpx_errors
    hpx_format
    hpx_functional
    hpx_memory
    hpx_thread_support
    hpx_type_support
    hpx_util
    hpx_version
  CMAKE_SUBDIRS examples tests
)

if(MSVC AND HPX_COROUTINES_WITH_SWAP_CONTEXT_EMULATION)
  hpx_info(
    "    SwitchToFiber emulation is enabled, using compiler: '${CMAKE_ASM_MASM_COMPILER}'"
  )
endif()
