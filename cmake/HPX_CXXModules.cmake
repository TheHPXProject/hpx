# Copyright (c) 2025 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_AddCompileFlag)
include(HPX_Message)

macro(hpx_check_cxx_modules_support)
  if(HPX_WITH_CXX_MODULES)
    if(NOT (CMAKE_VERSION VERSION_GREATER_EQUAL "3.29"))
      hpx_fatal(
        "Please use a version of CMake newer than V3.28 in order to enable C++ module support for HPX"
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
# * Creates an interface target '<producer>_if' for consumers to link to
# * Marks the interface target for CMake's native module scanning
function(hpx_configure_module_producer producer)
  if(NOT TARGET ${producer})
    hpx_error("hpx_configure_module_producer: target '${producer}' not found")
  endif()

  set(_iface "${producer}_if")
  if(NOT TARGET ${_iface})
    add_library(${_iface} INTERFACE)
    target_link_libraries(${_iface} INTERFACE ${producer})
  endif()

  # Make sure consumers scan for modules through CMake's native module handling
  set_target_properties(${_iface} PROPERTIES INTERFACE_CXX_SCAN_FOR_MODULES ON)

  # Define HPX_COMPILE_BMI to ensure that internal headers use #include instead
  # of import during the generation of the BMI.
  target_compile_definitions(${producer} PRIVATE HPX_COMPILE_BMI)
endfunction()

# hpx_configure_module_consumer(<consumer> <producer>])
#
# * propagates module-related properties from producer interface target
# * sets necessary consumer compiler flags for clang and gcc
function(hpx_configure_module_consumer consumer producer)
  if(NOT TARGET ${consumer})
    hpx_error("hpx_configure_module_consumer: target '${consumer}' not found")
  endif()
  if(NOT TARGET ${producer})
    hpx_error("hpx_configure_module_consumer: target '${producer}' not found")
  endif()

  target_link_libraries(${consumer} PRIVATE ${producer})
  get_target_property(_scan ${producer} INTERFACE_CXX_SCAN_FOR_MODULES)
  if(_scan)
    set_target_properties(${consumer} PROPERTIES CXX_SCAN_FOR_MODULES ${_scan})

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
                                                "AppleClang"
    )
      get_target_property(_type ${consumer} TYPE)
      if((_type STREQUAL "SHARED_LIBRARY") OR (_type STREQUAL "EXECUTABLE"))
        target_link_options(${consumer} PRIVATE "-fuse-ld=lld")
        target_link_options(${consumer} PRIVATE "-Wl,--error-limit=0")
      endif()
    endif()
  endif()
endfunction()
