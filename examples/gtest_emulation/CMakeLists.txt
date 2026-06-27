# Copyright (c) 2020  ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.18)

project(gtest CXX)

if(EXISTS "${HPX_DIR}")

  find_package(HPX REQUIRED)

  if(HPX_WITH_CXX_MODULES)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    # CMake only propagates imported C++ module metadata from targets that are
    # linked directly by the consumer. The exported HPXInternal module targets
    # are therefore linked explicitly for the external build tests.
    set(hpx_cxx_module_targets)
    if(TARGET HPXInternal::hpx_core_module)
      list(APPEND hpx_cxx_module_targets HPXInternal::hpx_core_module)
    endif()
    if(TARGET HPXInternal::hpx_full_module)
      list(APPEND hpx_cxx_module_targets HPXInternal::hpx_full_module)
    endif()
  endif()

  # Add a static library which contains a main to emulate gtest_main
  add_library(static_main_lib STATIC static_main.cpp)
  set_target_properties(
    static_main_lib PROPERTIES CXX_STANDARD ${HPX_CXX_STANDARD}
  )

  # /!\ This helper interface is needed to keep the right linking order
  add_library(hpx_helper_interface INTERFACE)
  target_link_libraries(
    hpx_helper_interface INTERFACE HPX::hpx HPX::wrap_main static_main_lib
  )

  # Test with the main function in a separate static library
  add_executable(hpx_main_ext_main hpx_main_ext_main.cpp)
  # Keep the helper interface for link order, but link module targets directly
  # to the executable so CMake can see the imported BMI metadata.
  target_link_libraries(
    hpx_main_ext_main PRIVATE hpx_helper_interface ${hpx_cxx_module_targets}
  )
  set_target_properties(
    hpx_main_ext_main PROPERTIES CXX_STANDARD ${HPX_CXX_STANDARD}
  )

  enable_testing()
  add_test(hello_world_test hpx_main_ext_main)

else()
  message(
    WARNING
      "Skipping build test because HPX_DIR=${HPX_DIR} does not exist. This \
      last test requires HPX to be installed.  Did you forget to run the \
      install rule?"
  )
endif()
