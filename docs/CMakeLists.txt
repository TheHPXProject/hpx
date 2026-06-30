# Copyright (c) 2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# used while generating sphinx config file
set(doxygen_definition_list
    "DOXYGEN:=1"
    "BOOST_SYSTEM_NOEXCEPT="
    "HPX_EXCEPTION_EXPORT="
    "HPX_CORE_EXPORT="
    "HPX_CXX_CORE_EXPORT="
    "HPX_CXX_EXPORT="
    "HPX_FULL_EXPORT="
    "HPX_EXPORT="
    "HPX_ALWAYS_EXPORT="
    "extern="
    "HPX_FORCEINLINE="
    "HPX_CONCEPT_REQUIRES_(...)="
    "requires(...)="
    "HPX_HOST_DEVICE="
)

foreach(doxygen_predef ${doxygen_definition_list})
  set(doxygen_definitions "${doxygen_definitions} \"${doxygen_predef}\"")
endforeach()

# Generate rst files with CMake variables and toolchains
set(HPX_CMAKE_OPTIONS_RST "")
foreach(_cat ${HPX_OPTION_CATEGORIES})
  string(REPLACE " " "_" _cat_link "${_cat}")
  set(HPX_CMAKE_OPTIONS_RST
      "${HPX_CMAKE_OPTIONS_RST}* :ref:`${_cat} options <${_cat_link}>`\n"
  )
endforeach()
set(HPX_CMAKE_OPTIONS_RST "${HPX_CMAKE_OPTIONS_RST}\n")

get_cmake_property(_variableNames CACHE_VARIABLES)
foreach(_cat ${HPX_OPTION_CATEGORIES})
  string(REPLACE " " "_" _cat_anchor "${_cat}")
  set(_cat_title "${_cat} options")
  string(REGEX REPLACE "." "-" _cat_title_underline "${_cat_title}")
  set(HPX_CMAKE_OPTIONS_RST
      "${HPX_CMAKE_OPTIONS_RST}.. _${_cat_anchor}:\n\n${_cat_title}\n${_cat_title_underline}\n\n"
  )
  foreach(_variableName ${_variableNames})
    if(${_variableName}Category)
      if(_cat STREQUAL ${_variableName}Category)
        string(REPLACE " " "_" _variableName_anchor "${_variableName}")

        get_property(
          _doc
          CACHE "${_variableName}"
          PROPERTY HELPSTRING
        )
        get_property(
          _type
          CACHE "${_variableName}"
          PROPERTY TYPE
        )
        if(NOT _type STREQUAL "UNINITIALIZED")
          set(_variableName "${_variableName}:${_type}")
        endif()
        set(HPX_CMAKE_OPTIONS_RST
            "${HPX_CMAKE_OPTIONS_RST}* :option:`${_variableName}`\n"
        )
      endif()
    endif()
  endforeach()
  set(HPX_CMAKE_OPTIONS_RST "${HPX_CMAKE_OPTIONS_RST}\n")

  foreach(_variableName ${_variableNames})
    if(${_variableName}Category)
      if(_cat STREQUAL ${_variableName}Category)
        string(REPLACE " " "_" _variableName_anchor "${_variableName}")

        get_property(
          _doc
          CACHE "${_variableName}"
          PROPERTY HELPSTRING
        )
        get_property(
          _type
          CACHE "${_variableName}"
          PROPERTY TYPE
        )
        if(NOT _type STREQUAL "UNINITIALIZED")
          set(_variableName "${_variableName}:${_type}")
        endif()
        set(HPX_CMAKE_OPTIONS_RST
            "${HPX_CMAKE_OPTIONS_RST}.. option:: ${_variableName}\n\n   ${_doc}\n\n"
        )
      endif()
    endif()
  endforeach()
endforeach()

set(SPHINX_CMAKE_VARIABLES_RST_DEST
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmake_variables.rst"
)
configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/templates/cmake_variables.rst.in"
  ${SPHINX_CMAKE_VARIABLES_RST_DEST} @ONLY
)

# Configure Sphinx conf.py
set(HPX_CONF_DOCUMENTATION_SHORT_VERSION "${HPX_VERSION}")
if(HPX_WITH_GIT_TAG)
  set(HPX_CONF_DOCUMENTATION_VERSION "${HPX_WITH_GIT_TAG}")
elseif(HPX_WITH_GIT_BRANCH)
  set(HPX_CONF_DOCUMENTATION_VERSION "${HPX_WITH_GIT_BRANCH}")
else()
  # HPX_VERSION is always available, use as fallback
  set(HPX_CONF_DOCUMENTATION_VERSION "${HPX_VERSION_FULL}")
endif()

set(SPHINX_CONF_PY_DEST "${CMAKE_CURRENT_BINARY_DIR}/sphinx/conf.py")
configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/templates/conf.py.in" ${SPHINX_CONF_PY_DEST}
  @ONLY
)

# Copy the Sphinx source files to build directory
file(
  GLOB_RECURSE sphinx_source_files
  LIST_DIRECTORIES false
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/"
  "sphinx/*"
)

# exclude api.rst...
list(FILTER sphinx_source_files EXCLUDE REGEX "sphinx/api/full_api.rst")

foreach(sphinx_source_file ${sphinx_source_files})
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${sphinx_source_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${sphinx_source_file}"
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/${sphinx_source_file}"
      "${CMAKE_CURRENT_BINARY_DIR}/${sphinx_source_file}"
  )
endforeach()

foreach(sphinx_source_file ${sphinx_source_files})
  set(sphinx_source_files_build
      ${sphinx_source_files_build}
      "${CMAKE_CURRENT_BINARY_DIR}/${sphinx_source_file}"
  )
endforeach()

# Copy the Sphinx source files of the libs directory to build directory
file(
  GLOB_RECURSE sphinx_libs_source_files
  LIST_DIRECTORIES false
  RELATIVE "${PROJECT_SOURCE_DIR}/libs"
  "${PROJECT_SOURCE_DIR}/libs/*/*/docs/*"
)

list(APPEND sphinx_libs_source_files "index.rst" "overview.rst")

foreach(lib ${HPX_LIBS})
  list(APPEND sphinx_libs_source_files "${lib}/modules.rst")
endforeach()

foreach(sphinx_source_file ${sphinx_libs_source_files})
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sphinx/libs/${sphinx_source_file}"
    DEPENDS "${PROJECT_SOURCE_DIR}/libs/${sphinx_source_file}"
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "${PROJECT_SOURCE_DIR}/libs/${sphinx_source_file}"
      "${CMAKE_CURRENT_BINARY_DIR}/sphinx/libs/${sphinx_source_file}"
  )
endforeach()

foreach(sphinx_source_file ${sphinx_libs_source_files})
  set(sphinx_source_files_build
      ${sphinx_source_files_build}
      "${CMAKE_CURRENT_BINARY_DIR}/sphinx/libs/${sphinx_source_file}"
  )
endforeach()

# Create links to source files so that they are visible to sphinx
file(CREATE_LINK "${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/src"
     SYMBOLIC
)
file(CREATE_LINK "${PROJECT_SOURCE_DIR}/hpx" "${CMAKE_CURRENT_BINARY_DIR}/hpx"
     SYMBOLIC
)
file(CREATE_LINK "${PROJECT_SOURCE_DIR}/examples"
     "${CMAKE_CURRENT_BINARY_DIR}/examples" SYMBOLIC
)
file(CREATE_LINK "${PROJECT_SOURCE_DIR}/tests"
     "${CMAKE_CURRENT_BINARY_DIR}/tests" SYMBOLIC
)
file(CREATE_LINK "${PROJECT_SOURCE_DIR}/libs"
     "${CMAKE_CURRENT_BINARY_DIR}/libs" SYMBOLIC
)

add_custom_target(docs)
add_custom_target(docs-html)
add_custom_target(docs-singlehtml)
add_custom_target(docs-latexpdf)
add_custom_target(docs-man)

set(SPHINX_DOCS_HTML_OUTPUT_FILE "index.html")
set(SPHINX_DOCS_SINGLEHTML_OUTPUT_FILE "index.html")
set(SPHINX_DOCS_PDF_OUTPUT_FILE "latex/HPX.pdf")
set(SPHINX_DOCS_MAN_OUTPUT_FILE "hpx.1")

foreach(output_format ${HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS})
  string(TOUPPER ${output_format} output_format_upper_case)
  set(SPHINX_DOCS_OUTPUT_DIR
      "${PROJECT_BINARY_DIR}/share/hpx/docs/${output_format}"
  )
  set(SPHINX_DOCS_OUTPUT_FILE
      "${SPHINX_DOCS_OUTPUT_DIR}/${SPHINX_DOCS_${output_format_upper_case}_OUTPUT_FILE}"
  )

  # NOTE: The PDF does not build without errors but still produces a reasonable
  # PDF. If there are real errors this will ignore them. Fixing the errors is at
  # the moment not worth the effort.
  if(${output_format} STREQUAL "latexpdf")
    set(SPHINX_DOCS_BUILD_COMMAND
        ${Sphinx_EXECUTABLE} -M ${output_format}
        "${CMAKE_CURRENT_BINARY_DIR}/sphinx" "${SPHINX_DOCS_OUTPUT_DIR}" || (
                                                                            exit
                                                                            0)
    )
  else()
    set(SPHINX_DOCS_BUILD_COMMAND
        ${Sphinx_EXECUTABLE} -b ${output_format} -n -d
        "${CMAKE_CURRENT_BINARY_DIR}/doctree"
        "${CMAKE_CURRENT_BINARY_DIR}/sphinx" "${SPHINX_DOCS_OUTPUT_DIR}"
    )
  endif()

  add_custom_command(
    OUTPUT "${SPHINX_DOCS_OUTPUT_FILE}"
    DEPENDS "${sphinx_source_files_build}"
    COMMAND ${SPHINX_DOCS_BUILD_COMMAND}
  )

  add_custom_target(
    docs-${output_format}-file ALL DEPENDS ${SPHINX_DOCS_OUTPUT_FILE}
  )
  add_dependencies(docs-${output_format} docs-${output_format}-file)

  add_dependencies(docs docs-${output_format})
endforeach()

add_custom_target(
  git_docs
  COMMAND
    "${CMAKE_COMMAND}" -DHPX_BINARY_DIR:PATH=${PROJECT_BINARY_DIR}
    -DHPX_VERSION:STRING=${HPX_VERSION}
    -DHPX_WITH_GIT_BRANCH=${HPX_WITH_GIT_BRANCH}
    -DHPX_WITH_GIT_TAG=${HPX_WITH_GIT_TAG}
    -DHPX_WITH_DOCUMENTATION_OUTPUT_FORMATS="${HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS}"
    -P "${PROJECT_SOURCE_DIR}/cmake/HPX_UpdateGitDocs.cmake"
)
