# Copyright (c) 2025 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)

macro(hpx_check_cxx_modules_support)
  if(HPX_WITH_CXX_MODULES)
    if(NOT (CMAKE_VERSION VERSION_GREATER_EQUAL "3.28"))
      hpx_fatal(
        "Please use a version of CMake 3.28 or newer in order to enable C++ module support for HPX"
      )
    endif()

    if(NOT (CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES
                                               "Visual Studio")
    )
      hpx_error(
        "C++20 modules require Ninja or Visual Studio generator. Current generator: ${CMAKE_GENERATOR}\n"
        "Please reconfigure with: cmake -G Ninja ..."
      )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
      hpx_error(
        "AppleClang does not support C++20 module dependency scanning.\n"
        "Please install and use LLVM Clang 16+ instead:\n"
        "  macOS: brew install llvm\n"
        "  Then: cmake -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ ..."
      )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.0")
        hpx_error(
          "Clang 16+ is required for C++20 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}"
        )
      endif()
      set(HPX_MODULE_INTERFACE_EXTENSION ".cppm")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
        hpx_error(
          "GCC 14+ is required for C++20 modules support (experimental). Current version: ${CMAKE_CXX_COMPILER_VERSION}"
        )
      endif()
      set(HPX_MODULE_INTERFACE_EXTENSION ".cxx")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      if(MSVC_VERSION LESS 1934)
        hpx_error(
          "Visual Studio 17.4+ is required for C++20 modules support. Current version: ${MSVC_VERSION}"
        )
      endif()
      set(HPX_MODULE_INTERFACE_EXTENSION ".ixx")
    else()
      hpx_warn(
        "C++20 modules support for compiler '${CMAKE_CXX_COMPILER_ID}' is unknown. Proceed with caution."
      )
      set(HPX_MODULE_INTERFACE_EXTENSION ".cppm")
    endif()
  endif()
endmacro()

if(NOT HPX_WITH_CXX_MODULES)
  return()
endif()

# hpx_configure_module_producer(<producer>)
#
# * Creates an interface target '<producer>_if' for consumers to link to.
# * Sets INTERFACE_CXX_SCAN_FOR_MODULES ON so that CMake's native module
#   dependency tracking propagates to all consumers.
#
# CMake 3.28+ with FILE_SET CXX_MODULES handles BMI generation and
# -fmodule-output/-fprebuilt-module-path flags automatically. No manual compiler
# flags are needed here.
function(hpx_configure_module_producer producer)
  if(NOT TARGET ${producer})
    hpx_error("hpx_configure_module_producer: target '${producer}' not found")
  endif()

  set(_iface "${producer}_if")
  if(NOT TARGET ${_iface})
    add_library(${_iface} INTERFACE)
    target_link_libraries(${_iface} INTERFACE ${producer})
  endif()

  # Propagate scanning requirement to consumers via the interface target. CMake
  # uses this to enable its native module dependency scanning for any target
  # that links to this interface.
  set_target_properties(${_iface} PROPERTIES INTERFACE_CXX_SCAN_FOR_MODULES ON)
endfunction()

# hpx_configure_module_consumer(<consumer> <producer>)
#
# * Links the consumer to the producer interface target.
# * Enables CMake's native module scanning on the consumer.
#
# CMake 3.29+ automatically resolves the BMI location from the FILE_SET
# CXX_MODULES declared on the producer. No -fprebuilt-module-path needed.
function(hpx_configure_module_consumer consumer producer)
  if(NOT TARGET ${consumer})
    hpx_error("hpx_configure_module_consumer: target '${consumer}' not found")
  endif()
  if(NOT TARGET ${producer})
    hpx_error("hpx_configure_module_consumer: target '${producer}' not found")
  endif()

  # Imported module metadata is only picked up from direct link dependencies.
  # Link the underlying module target directly when the producer follows the
  # '<module>_if' wrapper pattern.
  if(producer MATCHES "_if$")
    string(REGEX REPLACE "_if$" "" _producer_target "${producer}")
    if(TARGET ${_producer_target})
      target_link_libraries(${consumer} PRIVATE ${_producer_target})
    endif()
  endif()

  target_link_libraries(${consumer} PRIVATE ${producer})

  get_target_property(_scan ${producer} INTERFACE_CXX_SCAN_FOR_MODULES)
  if(_scan)
    set_target_properties(${consumer} PROPERTIES CXX_SCAN_FOR_MODULES ${_scan})
  endif()
endfunction()
