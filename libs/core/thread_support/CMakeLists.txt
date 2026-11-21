# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(_runs_as_child_description
    "Futures attempt to run associated threads directly if those have not been started (default: OFF)"
)

# The option was called HPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD
# before it moved here with thread_enums.hpp. A value given for the old name, ON
# or OFF, is carried over to the new one. The old entry is removed from the
# cache below, so an old entry seen later was given explicitly. A build
# directory configured before the rename still has the entry hpx_option()
# created, together with its Category entry: its value is carried over only if
# the new option isn't set yet, and only ON is reported, OFF is the default.
set(_runs_as_child_old HPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD)
set(_runs_as_child_new
    HPX_THREAD_SUPPORT_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD
)
if(DEFINED ${_runs_as_child_old})
  set(_runs_as_child_from_old_cache FALSE)
  if(DEFINED ${_runs_as_child_old}Category)
    set(_runs_as_child_from_old_cache TRUE)
  endif()

  if(NOT (_runs_as_child_from_old_cache AND DEFINED ${_runs_as_child_new}))
    if(NOT _runs_as_child_from_old_cache OR ${_runs_as_child_old})
      # hpx_warn is silent at the default log level, a deprecation is not
      message(
        DEPRECATION
          "${_runs_as_child_old} is deprecated, use ${_runs_as_child_new} instead"
      )
    endif()

    set(_runs_as_child_value OFF)
    if(${_runs_as_child_old})
      set(_runs_as_child_value ON)
    endif()
    set(${_runs_as_child_new}
        ${_runs_as_child_value}
        CACHE BOOL "${_runs_as_child_description}" FORCE
    )
    mark_as_advanced(${_runs_as_child_new})
  endif()
endif()
unset(${_runs_as_child_old} CACHE)
unset(${_runs_as_child_old}Category CACHE)

hpx_option(
  HPX_THREAD_SUPPORT_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD BOOL
  "${_runs_as_child_description}" OFF
  CATEGORY "Thread Manager"
  ADVANCED
  MODULE THREAD_SUPPORT
)

if(HPX_THREAD_SUPPORT_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD)
  hpx_add_config_define_namespace(
    DEFINE HPX_THREAD_SUPPORT_HAVE_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD
    NAMESPACE THREAD_SUPPORT
  )
endif()

set(thread_support_headers
    hpx/thread_support/atomic_count.hpp
    hpx/thread_support/set_thread_name.hpp
    hpx/thread_support/spinlock.hpp
    hpx/thread_support/stack_size.hpp
    hpx/thread_support/task_size.hpp
    hpx/thread_support/thread_enums.hpp
    hpx/thread_support/thread_specific_ptr.hpp
    hpx/thread_support/unlock_guard.hpp
    hpx/thread_support/win_nanosleep.hpp
)

# cmake-format: off
set(thread_support_compat_headers
    hpx/thread_support.hpp => hpx/modules/thread_support.hpp
    hpx/util/atomic_count.hpp => hpx/modules/thread_support.hpp
    hpx/util/set_thread_name.hpp => hpx/modules/thread_support.hpp
    hpx/util/thread_specific_ptr.hpp => hpx/modules/thread_support.hpp
    hpx/util/unlock_guard.hpp => hpx/modules/thread_support.hpp
)
# cmake-format: on

set(thread_support_sources set_thread_name.cpp spinlock.cpp thread_enums.cpp
                           win_nanosleep.cpp
)

include(HPX_AddModule)
add_hpx_module(
  core thread_support
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${thread_support_sources}
  HEADERS ${thread_support_headers}
  COMPAT_HEADERS ${thread_support_compat_headers}
  MODULE_DEPENDENCIES hpx_assertion hpx_config hpx_concepts hpx_tracing
                      hpx_type_support
  CMAKE_SUBDIRS examples tests
)
