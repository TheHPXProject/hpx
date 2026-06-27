# Copyright (c) 2007-2023 Hartmut Kaiser
# Copyright (c) 2011      Bryce Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

function(add_hpx_component name)
  # retrieve arguments
  set(options
      EXCLUDE_FROM_ALL
      INSTALL_HEADERS
      INTERNAL_FLAGS
      NOEXPORT
      AUTOGLOB
      STATIC
      PLUGIN
      PREPEND_SOURCE_ROOT
      PREPEND_HEADER_ROOT
      UNITY_BUILD
  )
  set(one_value_args
      INI
      FOLDER
      SOURCE_ROOT
      HEADER_ROOT
      SOURCE_GLOB
      HEADER_GLOB
      OUTPUT_SUFFIX
      INSTALL_SUFFIX
      INSTALL_COMPONENT
      LANGUAGE
      SCAN_FOR_MODULES
  )
  set(multi_value_args
      SOURCES
      HEADERS
      AUXILIARY
      DEPENDENCIES
      COMPONENT_DEPENDENCIES
      COMPILE_FLAGS
      LINK_FLAGS
  )
  cmake_parse_arguments(
    ${name} "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  if(NOT ${name}_LANGUAGE)
    set(${name}_LANGUAGE CXX)
  endif()

  if(NOT ${name}_SOURCE_ROOT)
    set(${name}_SOURCE_ROOT ".")
  endif()
  hpx_debug(
    "Add component ${name}: ${name}_SOURCE_ROOT: ${${name}_SOURCE_ROOT}"
  )

  if(NOT ${name}_HEADER_ROOT)
    set(${name}_HEADER_ROOT ".")
  endif()
  hpx_debug(
    "Add component ${name}: ${name}_HEADER_ROOT: ${${name}_HEADER_ROOT}"
  )

  # Collect sources and headers from the given (current) directory
  # (recursively), but only if AUTOGLOB flag is specified.
  if(${${name}_AUTOGLOB})
    if(NOT ${name}_SOURCE_GLOB)
      set(${name}_SOURCE_GLOB
          "${${name}_SOURCE_ROOT}/*.cpp"
          "${${name}_SOURCE_ROOT}/*.c"
          "${${name}_SOURCE_ROOT}/*.f"
          "${${name}_SOURCE_ROOT}/*.F"
          "${${name}_SOURCE_ROOT}/*.f77"
          "${${name}_SOURCE_ROOT}/*.F77"
          "${${name}_SOURCE_ROOT}/*.f90"
          "${${name}_SOURCE_ROOT}/*.F90"
          "${${name}_SOURCE_ROOT}/*.f95"
          "${${name}_SOURCE_ROOT}/*.F95"
      )
    endif()
    hpx_debug(
      "Add component ${name}: ${name}_SOURCE_GLOB: ${${name}_SOURCE_GLOB}"
    )

    add_hpx_library_sources(
      ${name}_component GLOB_RECURSE GLOBS "${${name}_SOURCE_GLOB}"
    )

    set(${name}_SOURCES ${${name}_component_SOURCES})
    add_hpx_source_group(
      NAME ${name}
      CLASS "Source Files"
      ROOT ${${name}_SOURCE_ROOT}
      TARGETS ${${name}_component_SOURCES}
    )

    if(NOT ${name}_HEADER_GLOB)
      set(${name}_HEADER_GLOB "${${name}_HEADER_ROOT}/*.hpp"
                              "${${name}_HEADER_ROOT}/*.h"
      )
    endif()
    hpx_debug(
      "Add component ${name}: ${name}_HEADER_GLOB: ${${name}_HEADER_GLOB}"
    )

    add_hpx_library_headers(
      ${name}_component GLOB_RECURSE GLOBS "${${name}_HEADER_GLOB}"
    )

    set(${name}_HEADERS ${${name}_component_HEADERS})
    add_hpx_source_group(
      NAME ${name}
      CLASS "Header Files"
      ROOT ${${name}_HEADER_ROOT}
      TARGETS ${${name}_component_HEADERS}
    )
  else()
    if(${name}_PREPEND_SOURCE_ROOT)
      list(TRANSFORM ${name}_SOURCES PREPEND ${${name}_SOURCE_ROOT}/)
    endif()
    if(${name}_PREPEND_HEADER_ROOT)
      list(TRANSFORM ${name}_HEADERS PREPEND ${${name}_HEADER_ROOT}/)
    endif()

    add_hpx_library_sources_noglob(
      ${name}_component SOURCES "${${name}_SOURCES}"
    )

    add_hpx_source_group(
      NAME ${name}
      CLASS "Source Files"
      ROOT ${${name}_SOURCE_ROOT}
      TARGETS ${${name}_component_SOURCES}
    )

    add_hpx_library_headers_noglob(
      ${name}_component HEADERS "${${name}_HEADERS}"
    )

    add_hpx_source_group(
      NAME ${name}
      CLASS "Header Files"
      ROOT ${${name}_HEADER_ROOT}
      TARGETS ${${name}_component_HEADERS}
    )
  endif()

  hpx_print_list(
    "DEBUG" "Add component ${name}: Sources for ${name}" ${name}_SOURCES
  )
  hpx_print_list(
    "DEBUG" "Add component ${name}: Headers for ${name}" ${name}_HEADERS
  )
  hpx_print_list(
    "DEBUG" "Add component ${name}: Dependencies for ${name}"
    ${name}_DEPENDENCIES
  )
  hpx_print_list(
    "DEBUG" "Add component ${name}: Component dependencies for ${name}"
    ${name}_COMPONENT_DEPENDENCIES
  )
  hpx_print_list(
    "DEBUG" "Add component ${name}: Configuration files for ${name}"
    ${name}_INI
  )

  set(exclude_from_all)
  if(${name}_EXCLUDE_FROM_ALL)
    set(exclude_from_all EXCLUDE_FROM_ALL)
  else()
    if(${name}_PLUGIN AND NOT HPX_WITH_STATIC_LINKING)
      if(MSVC)
        set(library_install_destination ${CMAKE_INSTALL_BINDIR}/hpx)
      else()
        set(library_install_destination ${CMAKE_INSTALL_LIBDIR}/hpx)
      endif()
      set(archive_install_destination ${CMAKE_INSTALL_LIBDIR}/hpx)
      set(runtime_install_destination ${CMAKE_INSTALL_BINDIR}/hpx)
      set(${name}_OUTPUT_SUFFIX hpx)
    else()
      if(MSVC)
        set(library_install_destination ${CMAKE_INSTALL_BINDIR})
      else()
        set(library_install_destination ${CMAKE_INSTALL_LIBDIR})
      endif()
      set(archive_install_destination ${CMAKE_INSTALL_LIBDIR})
      set(runtime_install_destination ${CMAKE_INSTALL_BINDIR})
    endif()
    if(${name}_INSTALL_SUFFIX)
      set(library_install_destination ${${name}_INSTALL_SUFFIX})
      set(archive_install_destination ${${name}_INSTALL_SUFFIX})
      set(runtime_install_destination ${${name}_INSTALL_SUFFIX})
    endif()
    if(${name}_INSTALL_COMPONENT)
      set(install_component COMPONENT ${${name}_INSTALL_COMPONENT})
    endif()
    # cmake-format: off
    set(_target_flags
        INSTALL INSTALL_FLAGS
          LIBRARY DESTINATION ${library_install_destination} ${install_component}
          ARCHIVE DESTINATION ${archive_install_destination} ${install_component}
          RUNTIME DESTINATION ${runtime_install_destination} ${install_component}
    )
    # cmake-format: on

    # install PDB if needed
    if(MSVC
       AND NOT ${name}_STATIC
       AND NOT HPX_WITH_STATIC_LINKING
    )
      # cmake-format: off
      set(_target_flags
          ${_target_flags}
          INSTALL_PDB $<TARGET_PDB_FILE:${name}_component>
            DESTINATION ${runtime_install_destination}
            ${install_component}
          CONFIGURATIONS Debug RelWithDebInfo
          OPTIONAL
      )
      # cmake-format: on
    endif()
  endif()

  if(${name}_INTERNAL_FLAGS)
    set(_target_flags ${_target_flags} INTERNAL_FLAGS)
  endif()

  if(${name}_PLUGIN)
    set(_target_flags ${_target_flags} PLUGIN)
  endif()

  if(NOT ${${name}_NOEXPORT})
    set(_target_flags ${_target_flags} EXPORT)
  endif()

  if(${name}_UNITY_BUILD)
    set(_target_flags ${_target_flags} UNITY_BUILD)
  endif()

  if(${${name}_STATIC})
    set(${name}_lib_linktype STATIC)
  else()
    if(HPX_WITH_STATIC_LINKING)
      set(${name}_lib_linktype STATIC)
    else()
      set(${name}_lib_linktype SHARED)
    endif()
  endif()

  add_library(
    ${name}_component
    ${${name}_lib_linktype} ${exclude_from_all} ${${name}_SOURCES}
    ${${name}_HEADERS} ${${name}_AUXILIARY}
  )

  if(NOT ${${name}_HEADER_ROOT} STREQUAL ".")
    target_include_directories(
      ${name}_component PUBLIC $<BUILD_INTERFACE:${${name}_HEADER_ROOT}>
    )
    if(${name}_INSTALL_HEADERS)
      set(_target_flags ${_target_flags} INSTALL_HEADERS HEADER_ROOT
                        "${${name}_HEADER_ROOT}"
      )
    endif()
  endif()

  if(${name}_OUTPUT_SUFFIX)
    if(MSVC)
      set_target_properties(
        "${name}_component"
        PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY_RELEASE
          "${PROJECT_BINARY_DIR}/Release/bin/${${name}_OUTPUT_SUFFIX}"
          LIBRARY_OUTPUT_DIRECTORY_RELEASE
          "${PROJECT_BINARY_DIR}/Release/bin/${${name}_OUTPUT_SUFFIX}"
          ARCHIVE_OUTPUT_DIRECTORY_RELEASE
          "${PROJECT_BINARY_DIR}/Release/lib/${${name}_OUTPUT_SUFFIX}"
          RUNTIME_OUTPUT_DIRECTORY_DEBUG
          "${PROJECT_BINARY_DIR}/Debug/bin/${${name}_OUTPUT_SUFFIX}"
          LIBRARY_OUTPUT_DIRECTORY_DEBUG
          "${PROJECT_BINARY_DIR}/Debug/bin/${${name}_OUTPUT_SUFFIX}"
          ARCHIVE_OUTPUT_DIRECTORY_DEBUG
          "${PROJECT_BINARY_DIR}/Debug/lib/${${name}_OUTPUT_SUFFIX}"
          RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL
          "${PROJECT_BINARY_DIR}/MinSizeRel/bin/${${name}_OUTPUT_SUFFIX}"
          LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL
          "${PROJECT_BINARY_DIR}/MinSizeRel/bin/${${name}_OUTPUT_SUFFIX}"
          ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL
          "${PROJECT_BINARY_DIR}/MinSizeRel/lib/${${name}_OUTPUT_SUFFIX}"
          RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO
          "${PROJECT_BINARY_DIR}/RelWithDebInfo/bin/${${name}_OUTPUT_SUFFIX}"
          LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO
          "${PROJECT_BINARY_DIR}/RelWithDebInfo/bin/${${name}_OUTPUT_SUFFIX}"
          ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO
          "${PROJECT_BINARY_DIR}/RelWithDebInfo/lib/${${name}_OUTPUT_SUFFIX}"
      )
    else()
      set_target_properties(
        "${name}_component"
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                   "${PROJECT_BINARY_DIR}/bin/${${name}_OUTPUT_SUFFIX}"
                   LIBRARY_OUTPUT_DIRECTORY
                   "${PROJECT_BINARY_DIR}/lib/${${name}_OUTPUT_SUFFIX}"
                   ARCHIVE_OUTPUT_DIRECTORY
                   "${PROJECT_BINARY_DIR}/lib/${${name}_OUTPUT_SUFFIX}"
      )
    endif()
  endif()

  # if modules are not enabled for this executable, then we need to add a
  # special preprocessor constant preventing the code from trying to use the
  # module interface unit exposed from the HPX libraries
  if(HPX_WITH_CXX_MODULES)
    if((NOT CMAKE_CXX_SCAN_FOR_MODULES) AND (NOT ${name}_SCAN_FOR_MODULES))
      hpx_debug("add_component.${name} SCAN_FOR_MODULES: OFF")
      set(${name}_SCAN_FOR_MODULES OFF)
    else()
      hpx_debug("add_component.${name} SCAN_FOR_MODULES: ON")
      set(${name}_SCAN_FOR_MODULES ON)
    endif()
  endif()

  if(HPX_WITH_CXX_MODULES AND MSVC)
    # ignore warnings about exported symbols being imported
    set(${name}_LINK_FLAGS "${${name}_LINK_FLAGS} /ignore:4217 /ignore:4286")
  endif()

  hpx_setup_target(
    ${name}_component
    TYPE COMPONENT
    NAME ${name}
    FOLDER ${${name}_FOLDER}
    COMPILE_FLAGS ${${name}_COMPILE_FLAGS}
    LINK_FLAGS ${${name}_LINK_FLAGS}
    DEPENDENCIES ${${name}_DEPENDENCIES}
    COMPONENT_DEPENDENCIES ${${name}_COMPONENT_DEPENDENCIES}
    SCAN_FOR_MODULES ${${name}_SCAN_FOR_MODULES} ${_target_flags}
  )

endfunction()
