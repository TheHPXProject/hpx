# Copyright (c) 2025 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)

# Function to parse standard_headers.inc and extract names, guards, and macro
# headers
function(hpx_get_standard_headers headers_var guards_prefix macro_headers_var)
  file(STRINGS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/standard_headers.inc"
       _raw_lines
  )
  set(_std_lines)
  set(_accum)
  foreach(_line ${_raw_lines})
    if(NOT _line MATCHES "^[ \t]*//")
      if(_accum)
        string(APPEND _accum " ${_line}")
      else()
        set(_accum "${_line}")
      endif()
      if(_accum MATCHES "\\)")
        list(APPEND _std_lines "${_accum}")
        set(_accum)
      endif()
    endif()
  endforeach()

  set(_headers)
  set(_macro_headers)
  foreach(_line ${_std_lines})
    if(_line MATCHES
       "HPX_STD_HEADER\\([ \t]*\"([^\"]+)\",[ \t]*\"([^\"]*)\",[ \t]*([01])\\)"
    )
      set(_hdr "${CMAKE_MATCH_1}")
      set(_guard "${CMAKE_MATCH_2}")
      set(_is_macro "${CMAKE_MATCH_3}")

      list(APPEND _headers "<${_hdr}>")
      if(_is_macro EQUAL 1)
        list(APPEND _macro_headers "${_hdr}")
      endif()
      if(_guard AND guards_prefix)
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" _var_suffix "${_hdr}")
        set(${guards_prefix}${_var_suffix}
            "${_guard}"
            PARENT_SCOPE
        )
      endif()
    endif()
  endforeach()

  set(${headers_var}
      "${_headers}"
      PARENT_SCOPE
  )
  if(macro_headers_var)
    set(${macro_headers_var}
        "${_macro_headers}"
        PARENT_SCOPE
    )
  endif()
endfunction()

# List of known standard library headers (read from central definition in
# standard_headers.inc)
hpx_get_standard_headers(STANDARD_LIBRARY_HEADERS "" "")

# Function to extract #includes from a file recursively
function(hpx_extract_includes_from_file module)

  # retrieve arguments
  set(options)
  set(one_value_args SOURCE FOUND_HEADERS)
  set(multi_value_args INCLUDE_DIRS)
  cmake_parse_arguments(
    ${module} "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  # look at every file only once
  set(this_file ${${module}_SOURCE})
  get_property(current_checked_list GLOBAL PROPERTY checked_list)
  if(${this_file} IN_LIST current_checked_list)
    return()
  endif()
  list(APPEND current_checked_list "${this_file}")
  set_property(GLOBAL PROPERTY checked_list "${current_checked_list}")

  file(READ ${this_file} file_content)

  # Regex to match #include directives for standard headers
  string(REGEX MATCHALL "#include <[^>]+>" includes ${file_content})

  set(found_includes)
  foreach(include ${includes})
    string(REGEX REPLACE "#include (<[^>]+>)" "\\1" filename ${include})

    if("${filename}" IN_LIST STANDARD_LIBRARY_HEADERS)
      # Capture only headers explicitly listed in STANDARD_LIBRARY_HEADERS.
      list(APPEND found_includes "${filename}")
    endif()
  endforeach()

  # Regex to match #include directives for hpx/ files
  string(REGEX MATCHALL "#include <hpx/[^>]+>" hpx_includes ${file_content})

  foreach(hpx_include ${hpx_includes})
    # Extract the file name from the hpx angle include
    string(REGEX REPLACE "#include <(hpx/[^>]+)>" "\\1" hpx_file ${hpx_include})

    # Check if the file exists in any of the base directories
    set(file_found FALSE)
    foreach(base_dir ${${module}_INCLUDE_DIRS})

      set(full_filename "${base_dir}/${hpx_file}")
      if(EXISTS "${full_filename}")
        set(file_found TRUE)

        # Recursively extract includes from the hpx file
        set(found_recursive_includes "")
        hpx_extract_includes_from_file(
          ${module}
          SOURCE "${full_filename}"
          INCLUDE_DIRS ${${module}_INCLUDE_DIRS}
          FOUND_HEADERS found_recursive_includes
        )
        list(APPEND found_includes ${found_recursive_includes})

        break()
      endif()
    endforeach()

    # we know that the file modules_enabled.hpp will be generated later
    if(NOT file_found AND NOT hpx_file MATCHES "hpx/config/modules_enabled.hpp")
      hpx_debug("Included file does not exist: ${hpx_file}")
    endif()
  endforeach()

  set(${${module}_FOUND_HEADERS}
      ${found_includes}
      PARENT_SCOPE
  )
endfunction()

function(hpx_collect_std_headers module)
  # retrieve arguments
  set(options)
  set(one_value_args SOURCE_ROOT GENERATED_ROOT FOUND_HEADERS)
  set(multi_value_args SOURCES)
  cmake_parse_arguments(
    ${module} "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  if(NOT ${module}_SOURCE_ROOT)
    hpx_error("SOURCE_ROOT argument not specified")
  endif()
  if(NOT ${module}_FOUND_HEADERS)
    hpx_error("FOUND_HEADERS argument not specified")
  endif()
  if(NOT ${module}_SOURCES)
    hpx_error("SOURCES argument not specified")
  endif()

  set_property(GLOBAL PROPERTY checked_list)

  # Generate a list of base directories that end with '/include'
  file(
    GLOB_RECURSE header_dirs
    LIST_DIRECTORIES true
    "${${module}_SOURCE_ROOT}/**/"
  )

  if(${module}_GENERATED_ROOT)
    file(
      GLOB_RECURSE generated_header_dirs
      LIST_DIRECTORIES true
      "${${module}_GENERATED_ROOT}/**/"
    )
    list(APPEND header_dirs ${generated_header_dirs})
  endif()

  # Make sure only to consider directories ending with 'include/'
  set(include_dirs)
  foreach(item ${header_dirs})
    # Check if the item matches the regex and add it to the filtered list
    string(REGEX MATCH "include$" match_result "${item}")
    if(match_result)
      list(APPEND include_dirs "${item}")
    endif()
  endforeach()

  # Loop through each source file and extract includes
  set(found_includes)
  foreach(source_file ${${module}_SOURCES})
    set(found_recursive_includes)
    hpx_extract_includes_from_file(
      ${module}
      SOURCE ${source_file}
      INCLUDE_DIRS ${include_dirs}
      FOUND_HEADERS found_recursive_includes
    )
    list(APPEND found_includes ${found_recursive_includes})
  endforeach()

  set(${${module}_FOUND_HEADERS}
      ${found_includes}
      PARENT_SCOPE
  )
endfunction()
