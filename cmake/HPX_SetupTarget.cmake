# Copyright (c) 2014      Thomas Heller
# Copyright (c) 2007-2023 Hartmut Kaiser
# Copyright (c) 2011      Bryce Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_CXXModules)

cmake_policy(PUSH)

hpx_set_cmake_policy(CMP0054 NEW)
hpx_set_cmake_policy(CMP0060 NEW)
hpx_set_cmake_policy(CMP0074 NEW)
hpx_set_cmake_policy(CMP0144 NEW)
hpx_set_cmake_policy(CMP0167 OLD)

function(hpx_setup_target target)
  # retrieve arguments
  set(options
      EXPORT
      INSTALL
      INSTALL_HEADERS
      INTERNAL_FLAGS
      NOLIBS
      NONAMEPREFIX
      NOTLLKEYWORD
      PLUGIN
      UNITY_BUILD
  )
  set(one_value_args
      TYPE
      FOLDER
      NAME
      SOVERSION
      VERSION
      HPX_PREFIX
      HEADER_ROOT
      SCAN_FOR_MODULES
  )
  set(multi_value_args DEPENDENCIES COMPONENT_DEPENDENCIES COMPILE_FLAGS
                       LINK_FLAGS INSTALL_FLAGS INSTALL_PDB
  )
  cmake_parse_arguments(
    target "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  if(NOT TARGET ${target})
    hpx_error("${target} does not represent a target")
  endif()

  # Figure out which type we want...
  if(target_TYPE)
    string(TOUPPER "${target_TYPE}" _type)
  else()
    get_target_property(type_prop ${target} TYPE)
    if(type_prop STREQUAL "STATIC_LIBRARY")
      set(_type "LIBRARY")
    endif()
    if(type_prop STREQUAL "MODULE_LIBRARY")
      set(_type "LIBRARY")
    endif()
    if(type_prop STREQUAL "SHARED_LIBRARY")
      set(_type "LIBRARY")
    endif()
    if(type_prop STREQUAL "EXECUTABLE")
      set(_type "EXECUTABLE")
    endif()
  endif()
  hpx_debug("setup_target.${target}" "TYPE: ${_type}")

  if(target_FOLDER)
    set_target_properties(${target} PROPERTIES FOLDER "${target_FOLDER}")
    hpx_debug("setup_target.${target}" "FOLDER: ${target_FOLDER}")
  endif()

  get_target_property(target_SOURCES ${target} SOURCES)

  if(target_COMPILE_FLAGS)
    hpx_append_property(${target} COMPILE_FLAGS "${target_COMPILE_FLAGS}")
    hpx_debug("setup_target.${target}" "COMPILE_FLAGS: ${target_COMPILE_FLAGS}")
  endif()

  if(target_LINK_FLAGS)
    hpx_append_property(${target} LINK_FLAGS "${target_LINK_FLAGS}")
    hpx_debug("setup_target.${target}" "LINK_FLAGS: ${target_LINK_FLAGS}")
  endif()

  if(target_NAME)
    set(name "${target_NAME}")
  else()
    set(name "${target}")
  endif()

  if(target_NOTLLKEYWORD)
    set(__tll_private)
    set(__tll_public)
  else()
    set(__tll_private PRIVATE)
    set(__tll_public PUBLIC)
  endif()

  set(target_STATIC_LINKING OFF)
  if(HPX_WITH_STATIC_LINKING)
    set(target_STATIC_LINKING ON)
  else()
    set(_hpx_library_type)
    if(TARGET hpx)
      get_target_property(_hpx_library_type hpx TYPE)
    endif()

    if("${_hpx_library_type}" STREQUAL "STATIC_LIBRARY")
      set(target_STATIC_LINKING ON)
    endif()
  endif()
  hpx_debug("setup_target.${target}" "STATIC_LINKING: ${target_STATIC_LINKING}")

  if(target_HPX_PREFIX)
    hpx_debug("setup_target.${target}" "HPX_PREFIX: ${target_HPX_PREFIX}")
  endif()

  if("${_type}" STREQUAL "EXECUTABLE")
    target_compile_definitions(
      ${target} PRIVATE "HPX_APPLICATION_NAME=${name}"
                        "HPX_APPLICATION_STRING=\"${name}\""
    )

    if(target_HPX_PREFIX)
      set(_prefix ${target_HPX_PREFIX})

      if(MSVC)
        string(REPLACE ";" ":" _prefix "${_prefix}")
      endif()

      target_compile_definitions(${target} PRIVATE "HPX_PREFIX=\"${_prefix}\"")
    endif()
  endif()

  if("${_type}" STREQUAL "LIBRARY" OR "${_type}" STREQUAL "COMPONENT")
    if(DEFINED HPX_LIBRARY_VERSION AND DEFINED HPX_SOVERSION)
      # set properties of generated shared library
      set_target_properties(
        ${target} PROPERTIES VERSION ${HPX_LIBRARY_VERSION} SOVERSION
                                                            ${HPX_SOVERSION}
      )
    endif()
    if(NOT target_NONAMEPREFIX)
      hpx_set_lib_name(${target} ${name})
    endif()
    set_target_properties(
      ${target}
      PROPERTIES # create *nix style library versions + symbolic links
                 # allow creating static and shared libs without conflicts
                 CLEAN_DIRECT_OUTPUT 1 OUTPUT_NAME ${name}
    )
  endif()

  if("${_type}" STREQUAL "LIBRARY" AND target_PLUGIN)
    set(plugin_name "HPX_PLUGIN_NAME=hpx_${name}")
    target_link_libraries(${target} ${__tll_private} HPX::plugin)
  endif()

  if("${_type}" STREQUAL "COMPONENT")
    target_compile_definitions(
      ${target} PRIVATE "HPX_COMPONENT_NAME=hpx_${name}"
                        "HPX_COMPONENT_STRING=\"hpx_${name}\""
    )
    target_link_libraries(${target} ${__tll_private} HPX::component)
  endif()

  if(NOT target_NOLIBS)
    set(_wrap_main_deps)
    if("${_type}" STREQUAL "EXECUTABLE")
      set(_wrap_main_deps HPX::wrap_main)
    endif()
    target_link_libraries(${target} ${__tll_public} HPX::hpx ${_wrap_main_deps})

    if(HPX_WITH_DISTRIBUTED_RUNTIME)
      hpx_handle_component_dependencies(target_COMPONENT_DEPENDENCIES)
      target_link_libraries(
        ${target} ${__tll_public} ${target_COMPONENT_DEPENDENCIES}
      )
    endif()

    if(HPX_WITH_PRECOMPILED_HEADERS_INTERNAL)
      if("${_type}" STREQUAL "EXECUTABLE")
        target_precompile_headers(
          ${target} REUSE_FROM hpx_exe_precompiled_headers
        )
      endif()
    endif()
  elseif(HPX_WITH_CXX_MODULES AND target_SCAN_FOR_MODULES)
    set(target_SCAN_FOR_MODULES OFF)
  endif()

  if(("${_type}" STREQUAL "EXECUTABLE") AND MINGW)
    # Increase the maximum size of object file sections
    target_compile_options(${target} BEFORE PRIVATE "-Wa,-mbig-obj")
  endif()

  if(target_COMPONENT_DEPENDENCIES)
    hpx_print_list(
      "DEBUG" "setup_target.${target} COMPONENT_DEPENDENCIES: "
      target_COMPONENT_DEPENDENCIES
    )
  endif()

  target_link_libraries(${target} ${__tll_public} ${target_DEPENDENCIES})
  if(target_COMPONENT_DEPENDENCIES)
    hpx_print_list(
      "DEBUG" "setup_target.${target} DEPENDENCIES: " target_DEPENDENCIES
    )
  endif()

  if(target_INTERNAL_FLAGS AND TARGET hpx_private_flags)
    target_link_libraries(${target} ${__tll_private} hpx_private_flags)
    hpx_print_list(
      "DEBUG" "setup_target.${target} private_flags: " hpx_private_flags
    )
  endif()

  if(target_UNITY_BUILD)
    set_target_properties(${target} PROPERTIES UNITY_BUILD ON)
    hpx_debug("setup_target.${target} UNITY_BUILD: ON")
  else()
    hpx_debug("setup_target.${target} UNITY_BUILD: OFF")
  endif()

  set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  if(HPX_WITH_CXX_MODULES AND target_SCAN_FOR_MODULES)
    hpx_debug("setup_target.${target} SCAN_FOR_MODULES: ON")

    if(TARGET hpx_core_module_if)
      hpx_configure_module_consumer(${target} hpx_core_module_if)
    elseif(TARGET HPXInternal::hpx_core_module_if)
      hpx_configure_module_consumer(${target} HPXInternal::hpx_core_module_if)
    else()
      hpx_error(
        "setup_target.${target}: C++ modules scanning is enabled, but neither "
        "hpx_core_module_if nor HPXInternal::hpx_core_module_if exists"
      )
    endif()

    if(TARGET hpx_full_module_if)
      hpx_configure_module_consumer(${target} hpx_full_module_if)
    elseif(TARGET HPXInternal::hpx_full_module_if)
      hpx_configure_module_consumer(${target} HPXInternal::hpx_full_module_if)
    endif()
  else()
    hpx_debug("setup_target.${target} SCAN_FOR_MODULES: OFF")

    if(HPX_WITH_CXX_MODULES)
      # explicitly disable C++ modules
      target_compile_definitions(
        ${target} PRIVATE HPX_HAVE_FORCE_NO_CXX_MODULES
      )

      # If modules are enabled, Clang emits DWARF v5, which requires using lld
      # instead of ld.
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
                                                  "AppleClang"
      )
        get_target_property(_type ${target} TYPE)
        if((_type STREQUAL "SHARED_LIBRARY") OR (_type STREQUAL "EXECUTABLE"))
          target_link_options(${target} PRIVATE "-fuse-ld=lld")
        endif()
      endif()
    endif()

    set_target_properties(${target} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
  endif()

  get_target_property(target_EXCLUDE_FROM_ALL ${target} EXCLUDE_FROM_ALL)

  if(target_EXPORT AND NOT target_EXCLUDE_FROM_ALL)
    hpx_export_targets(${target})
    set(install_export EXPORT HPXTargets)
  endif()

  if(target_INSTALL AND NOT target_EXCLUDE_FROM_ALL)
    install(TARGETS ${target} ${install_export} ${target_INSTALL_FLAGS})
    hpx_debug("setup_target.${target}" "INSTALL_FLAGS: ${target_INSTALL_FLAGS}")

    if(target_INSTALL_PDB)
      install(FILES ${target_INSTALL_PDB})
      hpx_debug("setup_target.${target}" "INSTALL_PDB: ${target_INSTALL_PDB}")
    endif()
    if(target_INSTALL_HEADERS AND (NOT target_HEADER_ROOT STREQUAL ""))
      install(
        DIRECTORY "${target_HEADER_ROOT}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT ${name}
      )
      hpx_debug("setup_target.${target}"
                "INSTALL_HEADERS: ${target_INSTALL_HEADERS}"
      )
    endif()
  endif()
endfunction()

cmake_policy(POP)
