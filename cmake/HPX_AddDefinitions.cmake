# Copyright (c) 2013-2026 Hartmut Kaiser
# Copyright (c) 2014 Thomas Heller
# Copyright (c) 2016 John Biddiscombe
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_ConfigureIfChanged)

function(hpx_add_config_define definition)

  # if(ARGN) ignores an argument "0"
  set(Args ${ARGN})
  list(LENGTH Args ArgsLen)
  if(ArgsLen GREATER 0)
    set_property(
      GLOBAL APPEND PROPERTY HPX_CONFIG_DEFINITIONS "${definition} ${ARGN}"
    )
  else()
    set_property(GLOBAL APPEND PROPERTY HPX_CONFIG_DEFINITIONS "${definition}")
  endif()

endfunction()

function(hpx_add_config_cond_define definition)

  # if(ARGN) ignores an argument "0"
  set(Args ${ARGN})
  list(LENGTH Args ArgsLen)
  if(ArgsLen GREATER 0)
    set_property(
      GLOBAL APPEND PROPERTY HPX_CONFIG_COND_DEFINITIONS
                             "${definition} ${ARGN}"
    )
  else()
    set_property(
      GLOBAL APPEND PROPERTY HPX_CONFIG_COND_DEFINITIONS "${definition}"
    )
  endif()

endfunction()

# ---------------------------------------------------------------------
# Function to add config defines to a list that depends on a namespace variable
# #defines that match the namespace can later be written out to a file
# ---------------------------------------------------------------------
function(hpx_add_config_define_namespace)
  set(options)
  set(one_value_args DEFINE NAMESPACE)
  set(multi_value_args VALUE)
  cmake_parse_arguments(
    OPTION "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  set(DEF_VAR HPX_CONFIG_DEFINITIONS_${OPTION_NAMESPACE})

  # to avoid extra trailing spaces (no value), use an if check
  if(OPTION_VALUE)
    set_property(
      GLOBAL APPEND PROPERTY ${DEF_VAR} "${OPTION_DEFINE} ${OPTION_VALUE}"
    )
  else()
    set_property(GLOBAL APPEND PROPERTY ${DEF_VAR} "${OPTION_DEFINE}")
  endif()

endfunction()

function(hpx_add_config_cond_define_namespace)
  set(one_value_args DEFINE NAMESPACE)
  set(multi_value_args VALUE)
  cmake_parse_arguments(
    OPTION "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  set(DEF_VAR HPX_CONFIG_COND_DEFINITIONS_${OPTION_NAMESPACE})

  # to avoid extra trailing spaces (no value), use an if check
  if(OPTION_VALUE)
    set_property(
      GLOBAL APPEND PROPERTY ${DEF_VAR} "${OPTION_DEFINE} ${OPTION_VALUE}"
    )
  else()
    set_property(GLOBAL APPEND PROPERTY ${DEF_VAR} "${OPTION_DEFINE}")
  endif()

endfunction()

# ---------------------------------------------------------------------
# Function to write variables out from a global var that was set using the
# config_define functions into a config file
# ---------------------------------------------------------------------
function(write_config_defines_file)
  set(options)
  set(one_value_args TEMPLATE NAMESPACE FILENAME)
  set(multi_value_args)
  cmake_parse_arguments(
    OPTION "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  if(${OPTION_NAMESPACE} STREQUAL "default")
    get_property(DEFINITIONS_VAR GLOBAL PROPERTY HPX_CONFIG_DEFINITIONS)
    get_property(
      COND_DEFINITIONS_VAR GLOBAL PROPERTY HPX_CONFIG_COND_DEFINITIONS
    )
  else()
    get_property(
      DEFINITIONS_VAR GLOBAL
      PROPERTY HPX_CONFIG_DEFINITIONS_${OPTION_NAMESPACE}
    )
    get_property(
      COND_DEFINITIONS_VAR GLOBAL
      PROPERTY HPX_CONFIG_COND_DEFINITIONS_${OPTION_NAMESPACE}
    )
  endif()

  if(DEFINED DEFINITIONS_VAR)
    list(SORT DEFINITIONS_VAR)
    list(REMOVE_DUPLICATES DEFINITIONS_VAR)
  endif()

  set(hpx_config_defines "\n")
  foreach(def ${DEFINITIONS_VAR})
    # C++23 specific variable
    string(FIND ${def} "HAVE_CXX23" _pos)
    if(NOT ${_pos} EQUAL -1)
      set(hpx_config_defines
          "${hpx_config_defines}#if __cplusplus >= 202300\n#define ${def}\n#endif\n"
      )
    else()
      # C++20 specific variable
      string(FIND ${def} "HAVE_CXX20" _pos)
      if(NOT ${_pos} EQUAL -1)
        set(hpx_config_defines
            "${hpx_config_defines}#if __cplusplus >= 202002\n#define ${def}\n#endif\n"
        )
      else()
        # C++17 specific variable
        string(FIND ${def} "HAVE_CXX17" _pos)
        if(NOT ${_pos} EQUAL -1)
          set(hpx_config_defines
              "${hpx_config_defines}#if __cplusplus >= 201500\n#define ${def}\n#endif\n"
          )
        else()
          set(hpx_config_defines "${hpx_config_defines}#define ${def}\n")
        endif()
      endif()
    endif()
  endforeach()

  if(DEFINED COND_DEFINITIONS_VAR)
    list(SORT COND_DEFINITIONS_VAR)
    list(REMOVE_DUPLICATES COND_DEFINITIONS_VAR)
    set(hpx_config_defines "${hpx_config_defines}\n")
  endif()
  foreach(def ${COND_DEFINITIONS_VAR})
    string(FIND ${def} " " _pos)
    if(NOT ${_pos} EQUAL -1)
      string(SUBSTRING ${def} 0 ${_pos} defname)
    else()
      set(defname ${def})
      string(STRIP ${defname} defname)
    endif()

    # C++20 specific variable
    string(FIND ${def} "HAVE_CXX20" _pos)
    if(NOT ${_pos} EQUAL -1)
      set(hpx_config_defines
          "${hpx_config_defines}#if __cplusplus >= 202002 && !defined(${defname})\n#define ${def}\n#endif\n"
      )
    else()
      # C++17 specific variable
      string(FIND ${def} "HAVE_CXX17" _pos)
      if(NOT ${_pos} EQUAL -1)
        set(hpx_config_defines
            "${hpx_config_defines}#if __cplusplus >= 201500 && !defined(${defname})\n#define ${def}\n#endif\n"
        )
      else()
        set(hpx_config_defines
            "${hpx_config_defines}#if !defined(${defname})\n#define ${def}\n#endif\n"
        )
      endif()
    endif()
  endforeach()

  string(TOUPPER ${OPTION_NAMESPACE} NAMESPACE_UPPER)
  set(TEMP_FILENAME
      "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${NAMESPACE_UPPER}"
  )

  # if the user has not specified a template, generate a proper header file
  if(NOT OPTION_TEMPLATE)
    set(PREAMBLE
        "//  Copyright (c) 2019-@HPX_COPYRIGHT_YEAR@ STE||AR Group\n"
        "//\n"
        "//  SPDX-License-Identifier: BSL-1.0\n"
        "//  Distributed under the Boost Software License, Version 1.0. (See accompanying\n"
        "//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)\n"
        "\n"
        "// Do not edit this file! It has been generated by the cmake configuration step.\n"
        "\n"
        "#pragma once\n"
    )
    file(WRITE ${TEMP_FILENAME} ${PREAMBLE} ${hpx_config_defines} "\n")
    hpx_configure_if_changed(
      INPUT "${TEMP_FILENAME}"
      OUTPUT "${OPTION_FILENAME}"
      CONFIGURE_ARGS @ONLY
    )
  else()
    hpx_configure_if_changed(
      INPUT "${OPTION_TEMPLATE}"
      OUTPUT "${TEMP_FILENAME}"
      CONFIGURE_ARGS @ONLY
    )
    hpx_configure_if_changed(
      INPUT "${TEMP_FILENAME}"
      OUTPUT "${OPTION_FILENAME}"
      CONFIGURE_ARGS COPYONLY
    )
  endif()
  file(REMOVE "${TEMP_FILENAME}")
endfunction()
