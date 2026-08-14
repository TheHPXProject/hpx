# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(runtime_local_headers
    hpx/runtime_local/component_startup_shutdown_base.hpp
    hpx/runtime_local/config_entry.hpp
    hpx/runtime_local/custom_exception_info.hpp
    hpx/runtime_local/debugging.hpp
    hpx/runtime_local/detail/runtime_local_fwd.hpp
    hpx/runtime_local/detail/serialize_exception.hpp
    hpx/runtime_local/get_locality_id.hpp
    hpx/runtime_local/get_locality_name.hpp
    hpx/runtime_local/get_num_all_localities.hpp
    hpx/runtime_local/get_os_thread_count.hpp
    hpx/runtime_local/get_thread_name.hpp
    hpx/runtime_local/get_worker_thread_num.hpp
    hpx/runtime_local/interval_timer.hpp
    hpx/runtime_local/macros.hpp
    hpx/runtime_local/os_thread_type.hpp
    hpx/runtime_local/pool_timer.hpp
    hpx/runtime_local/report_error.hpp
    hpx/runtime_local/run_as_hpx_thread.hpp
    hpx/runtime_local/run_as_os_thread.hpp
    hpx/runtime_local/runtime_handlers.hpp
    hpx/runtime_local/runtime_local.hpp
    hpx/runtime_local/runtime_local_fwd.hpp
    hpx/runtime_local/service_executors.hpp
    hpx/runtime_local/state.hpp
    hpx/runtime_local/shutdown_function.hpp
    hpx/runtime_local/startup_function.hpp
    hpx/runtime_local/termination_detection.hpp
    hpx/runtime_local/thread_hooks.hpp
    hpx/runtime_local/thread_mapper.hpp
    hpx/runtime_local/thread_pool_helpers.hpp
    hpx/runtime_local/thread_stacktrace.hpp
)

set(runtime_local_macro_headers hpx/runtime_local/macros.hpp)

# cmake-format: off
set(runtime_local_compat_headers
    hpx/custom_exception_info.hpp => hpx/modules/runtime_local.hpp
    hpx/parallel/executors/service_executors.hpp => hpx/include/parallel_executors.hpp
    hpx/runtime/components/component_startup_shutdown_base.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/config_entry.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/get_locality_id.hpp => hpx/runtime.hpp
    hpx/runtime/get_num_localities.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/get_os_thread_count.hpp => hpx/runtime.hpp
    hpx/runtime/get_thread_name.hpp => hpx/runtime.hpp
    hpx/runtime/get_worker_thread_num.hpp => hpx/runtime.hpp
    hpx/runtime/report_error.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/shutdown_function.hpp => hpx/runtime.hpp
    hpx/runtime/startup_function.hpp => hpx/runtime.hpp
    hpx/runtime/thread_hooks.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/thread_pool_helpers.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/threads/executors/service_executors.hpp => hpx/modules/runtime_local.hpp
    hpx/runtime/threads/run_as_hpx_thread.hpp => hpx/include/run_as.hpp
    hpx/runtime/threads/run_as_os_thread.hpp => hpx/include/run_as.hpp
    hpx/runtime_handlers.hpp => hpx/modules/runtime_local.hpp
    hpx/util/pool_timer.hpp => hpx/modules/runtime_local.hpp
    hpx/util/debug/thread_stacktrace.hpp => hpx/modules/runtime_local.hpp
    hpx/util/debugging.hpp => hpx/modules/runtime_local.hpp
    hpx/util/interval_timer.hpp => hpx/modules/runtime_local.hpp
    hpx/util/thread_mapper.hpp => hpx/modules/runtime_local.hpp
)
# cmake-format: on

set(runtime_local_sources
    custom_exception_info.cpp
    debugging.cpp
    interval_timer.cpp
    get_locality_name.cpp
    os_thread_type.cpp
    pool_timer.cpp
    runtime_handlers.cpp
    runtime_local.cpp
    serialize_exception.cpp
    service_executors.cpp
    state.cpp
    thread_mapper.cpp
    thread_pool_helpers.cpp
    thread_stacktrace.cpp
)

set(additional_dependencies)
if(HPX_LIKWID_WITH_LIKWID)
  set(additional_dependencies ${additional_dependencies} hpx_likwid)
  set(runtime_local_sources ${runtime_local_sources} likwid_init.cpp)
endif()

include(HPX_AddModule)
add_hpx_module(
  core runtime_local
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${runtime_local_sources}
  HEADERS ${runtime_local_headers}
  MACRO_HEADERS ${runtime_local_macro_headers}
  COMPAT_HEADERS ${runtime_local_compat_headers}
  ADD_TO_GLOBAL_HEADER "hpx/runtime_local/detail/runtime_local_fwd.hpp"
                       "hpx/runtime_local/detail/serialize_exception.hpp"
  MODULE_DEPENDENCIES
    hpx_async_base
    hpx_command_line_handling_local
    hpx_coroutines
    hpx_debugging
    hpx_errors
    hpx_datastructures
    hpx_execution
    hpx_execution_base
    hpx_executors
    hpx_format
    hpx_functional
    hpx_futures
    hpx_io_service
    hpx_lock_registration
    hpx_logging
    hpx_plugin
    hpx_program_options
    hpx_resource_partitioner
    hpx_runtime_configuration
    hpx_serialization
    hpx_static_reinit
    hpx_synchronization
    hpx_tag_invoke
    hpx_threading
    hpx_threading_base
    hpx_threadmanager
    hpx_thread_support
    hpx_timing
    hpx_topology
    hpx_tracing
    hpx_type_support
    hpx_util
    ${additional_dependencies}
  CMAKE_SUBDIRS examples tests
)
