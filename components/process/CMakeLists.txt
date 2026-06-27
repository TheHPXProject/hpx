# Copyright (c) 2007-2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

set(HPX_COMPONENTS
    ${HPX_COMPONENTS} process
    CACHE INTERNAL "list of HPX components"
)

set(process_headers
    hpx/components/process/util/wait_for_exit.hpp
    hpx/components/process/util/terminate.hpp
    hpx/components/process/util/shell_path.hpp
    hpx/components/process/util/search_path.hpp
    hpx/components/process/util/pipe.hpp
    hpx/components/process/util/mitigate.hpp
    hpx/components/process/util/initializers.hpp
    hpx/components/process/util/executor.hpp
    hpx/components/process/util/execute.hpp
    hpx/components/process/util/create_pipe.hpp
    hpx/components/process/util/child.hpp
    hpx/components/process/server/child.hpp
    hpx/components/process/process.hpp
    hpx/components/process/export_definitions.hpp
    hpx/components/process/child.hpp
    hpx/include/process.hpp
)

if(WIN32)
  set(process_headers
      ${process_headers}
      hpx/components/process/util/windows/wait_for_exit.hpp
      hpx/components/process/util/windows/terminate.hpp
      hpx/components/process/util/windows/shell_path.hpp
      hpx/components/process/util/windows/search_path.hpp
      hpx/components/process/util/windows/pipe.hpp
      hpx/components/process/util/windows/initializers/wait_on_latch.hpp
      hpx/components/process/util/windows/initializers/throw_on_error.hpp
      hpx/components/process/util/windows/initializers/start_in_dir.hpp
      hpx/components/process/util/windows/initializers/show_window.hpp
      hpx/components/process/util/windows/initializers/set_env.hpp
      hpx/components/process/util/windows/initializers/set_cmd_line.hpp
      hpx/components/process/util/windows/initializers/set_args.hpp
      hpx/components/process/util/windows/initializers/run_exe.hpp
      hpx/components/process/util/windows/initializers/on_CreateProcess_success.hpp
      hpx/components/process/util/windows/initializers/on_CreateProcess_setup.hpp
      hpx/components/process/util/windows/initializers/on_CreateProcess_error.hpp
      hpx/components/process/util/windows/initializers/initializer_base.hpp
      hpx/components/process/util/windows/initializers/inherit_env.hpp
      hpx/components/process/util/windows/initializers.hpp
      hpx/components/process/util/windows/initializers/hide_console.hpp
      hpx/components/process/util/windows/initializers/close_stdout.hpp
      hpx/components/process/util/windows/initializers/close_stdin.hpp
      hpx/components/process/util/windows/initializers/close_stderr.hpp
      hpx/components/process/util/windows/initializers/bind_stdout.hpp
      hpx/components/process/util/windows/initializers/bind_stdin.hpp
      hpx/components/process/util/windows/initializers/bind_stderr.hpp
      hpx/components/process/util/windows/executor.hpp
      hpx/components/process/util/windows/execute.hpp
      hpx/components/process/util/windows/create_pipe.hpp
      hpx/components/process/util/windows/child.hpp
  )
else()
  set(process_headers
      ${process_headers}
      hpx/components/process/util/posix/wait_for_exit.hpp
      hpx/components/process/util/posix/terminate.hpp
      hpx/components/process/util/posix/shell_path.hpp
      hpx/components/process/util/posix/search_path.hpp
      hpx/components/process/util/posix/pipe.hpp
      hpx/components/process/util/posix/initializers/wait_on_latch.hpp
      hpx/components/process/util/posix/initializers/throw_on_error.hpp
      hpx/components/process/util/posix/initializers/start_in_dir.hpp
      hpx/components/process/util/posix/initializers/set_env.hpp
      hpx/components/process/util/posix/initializers/set_cmd_line.hpp
      hpx/components/process/util/posix/initializers/set_args.hpp
      hpx/components/process/util/posix/initializers/run_exe.hpp
      hpx/components/process/util/posix/initializers/on_fork_success.hpp
      hpx/components/process/util/posix/initializers/on_fork_setup.hpp
      hpx/components/process/util/posix/initializers/on_fork_error.hpp
      hpx/components/process/util/posix/initializers/on_exec_setup.hpp
      hpx/components/process/util/posix/initializers/on_exec_error.hpp
      hpx/components/process/util/posix/initializers/notify_io_service.hpp
      hpx/components/process/util/posix/initializers/initializer_base.hpp
      hpx/components/process/util/posix/initializers/inherit_env.hpp
      hpx/components/process/util/posix/initializers.hpp
      hpx/components/process/util/posix/initializers/hide_console.hpp
      hpx/components/process/util/posix/initializers/close_stdout.hpp
      hpx/components/process/util/posix/initializers/close_stdin.hpp
      hpx/components/process/util/posix/initializers/close_stderr.hpp
      hpx/components/process/util/posix/initializers/close_fds_if.hpp
      hpx/components/process/util/posix/initializers/close_fds.hpp
      hpx/components/process/util/posix/initializers/close_fd.hpp
      hpx/components/process/util/posix/initializers/bind_stdout.hpp
      hpx/components/process/util/posix/initializers/bind_stdin.hpp
      hpx/components/process/util/posix/initializers/bind_stderr.hpp
      hpx/components/process/util/posix/initializers/bind_fd.hpp
      hpx/components/process/util/posix/executor.hpp
      hpx/components/process/util/posix/execute.hpp
      hpx/components/process/util/posix/create_pipe.hpp
      hpx/components/process/util/posix/child.hpp
  )
endif()

set(process_sources server/child_component.cpp process.cpp)

if(WIN32)
  set(process_sources
      ${process_sources} util/windows/shell_path_w.cpp
      util/windows/search_path_w.cpp util/windows/create_pipe_w.cpp
  )
else()
  set(process_sources ${process_sources} util/posix/shell_path_u.cpp
                      util/posix/search_path_u.cpp util/posix/create_pipe_u.cpp
  )
endif()

add_hpx_component(
  process INTERNAL_FLAGS
  FOLDER "Core/Components/Process"
  INSTALL_HEADERS PREPEND_HEADER_ROOT
  INSTALL_COMPONENT runtime
  HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include"
  HEADERS ${process_headers}
  PREPEND_SOURCE_ROOT ${HPX_WITH_UNITY_BUILD_OPTION}
  ${HPX_WITH_CXX_MODULES_OPTION}
  SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src"
  SOURCES ${process_sources}
)

target_compile_definitions(process_component PRIVATE HPX_PROCESS_EXPORTS)

add_hpx_pseudo_dependencies(components.process process_component)

# MSVC V1951 and V1952 exposes ICE compiling this in C++20 module mode
if(HPX_WITH_CXX_MODULES
   AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   AND ((MSVC_VERSION EQUAL 1951) OR (MSVC_VERSION EQUAL 1952))
)
  target_compile_definitions(
    process_component PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
  )
endif()

add_subdirectory(tests)
add_subdirectory(examples)
