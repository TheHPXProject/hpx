# Copyright (c) 2011 Bryce Lelbach
# Copyright (c) 2012-2013 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# find required packages
if(HPX_WITH_DOCUMENTATION)
  find_package(Doxygen)
  find_package(Sphinx)
  find_package(Breathe)

  if(NOT Sphinx_FOUND)
    hpx_error(
      "Sphinx is unavailable, sphinx documentation generation disabled. Set Sphinx_ROOT to your sphinx-build installation directory."
    )
    set(HPX_WITH_DOCUMENTATION OFF)
  elseif(NOT Breathe_FOUND)
    hpx_error(
      "Breathe is unavailable, sphinx documentation generation disabled. Set Breathe_APIDOC_ROOT to your breathe-apidoc installation directory."
    )
    set(HPX_WITH_DOCUMENTATION OFF)
  elseif(NOT DOXYGEN_FOUND)
    hpx_error(
      "Doxygen tool is unavailable, sphinx documentation generation disabled. Add the doxygen executable to your path or set the DOXYGEN_EXECUTABLE variable manually."
    )
    set(HPX_WITH_DOCUMENTATION OFF)
  endif()

  if(HPX_WITH_DOCUMENTATION)
    # used while generating sphinx config file and doxygen configuration
    set(doxygen_definition_list
        "DOXYGEN:=1"
        "BOOST_SYSTEM_NOEXCEPT="
        "HPX_EXCEPTION_EXPORT="
        "HPX_CXX_CORE_EXPORT="
        "HPX_CXX_EXPORT="
        "HPX_CORE_EXPORT="
        "HPX_FULL_EXPORT="
        "HPX_EXPORT="
        "HPX_ALWAYS_EXPORT="
        "extern="
        "HPX_FORCEINLINE="
        "HPX_CONCEPT_REQUIRES_(...)="
        "requires(...)="
        "HPX_HOST_DEVICE="
        "HPX_SUPERVISION_DISPATCH_EXPORT="
        "HPX_MOVE(x)=std::move(x)"
        "HPX_FORWARD(t,v)=std::forward<t>(v)"
    )

    foreach(doxygen_predef ${doxygen_definition_list})
      set(doxygen_definitions "${doxygen_definitions} \"${doxygen_predef}\"")
    endforeach()

    # cmake-format: off
    set(DOXYGEN_ALIASES
        "namedrequirement{1}=\"<a href=\"https://en.cppreference.com/w/cpp/named_req/\\1\">\\1</a>\""
    )
    # cmake-format: on
    set(DOXYGEN_EXCLUDE_SYMBOLS "detail")
    # set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_GENERATE_XML YES)
    set(DOXYGEN_GENERATE_HTML NO)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs/hpx_autodoc")
    set(DOXYGEN_PREDEFINED ${doxygen_definition_list})
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_STRIP_FROM_PATH ${HPX_SOURCE_DIR})
    set(DOXYGEN_XML_OUTPUT ".") # index.xml directly under OUTPUT_DIRECTORY

    # Doxygen verbatim configuration variables
    set(DOXYGEN_DIRECTORY_GRAPH NO)
    set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)

    set(DOXYGEN_VERBATIM_VARS
        DOXYGEN_DIRECTORY_GRAPH DOXYGEN_EXPAND_ONLY_PREDEF
        DOXYGEN_EXTRACT_PRIVATE DOXYGEN_MACRO_EXPANSION
        DOXYGEN_WARN_IF_UNDOCUMENTED
    )

    doxygen_add_docs(
      hpx_autodoc "${PROJECT_SOURCE_DIR}/libs"
      "${PROJECT_SOURCE_DIR}/components" "${PROJECT_SOURCE_DIR}/hpx"
      COMMENT "Generating Doxygen XML for Breathe/Sphinx"
    )

    # doxygen_add_docs() only registers index.xml as a BYPRODUCT of the
    # hpx_autodoc target's own build step, which does not give Ninja/Make a
    # build edge whose OUTPUT is index.xml. Add a real OUTPUT-based rule so
    # other targets (e.g. docs/CMakeLists.txt) can DEPENDS on the file path.
    add_custom_command(
      OUTPUT "${DOXYGEN_OUTPUT_DIRECTORY}/index.xml"
      COMMAND ${CMAKE_COMMAND} -E true
      DEPENDS hpx_autodoc
      COMMENT ""
      VERBATIM
    )
    add_custom_target(
      hpx_autodoc_index_xml DEPENDS "${DOXYGEN_OUTPUT_DIRECTORY}/index.xml"
    )
  endif()
endif()
