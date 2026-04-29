#  Copyright (c) 2024 Isidoros Tsaousis-Seiras
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(STDEXEC_ROOT AND NOT Stdexec_ROOT)
  set(Stdexec_ROOT ${STDEXEC_ROOT})
  # remove STDEXEC_ROOT from the cache
  unset(STDEXEC_ROOT CACHE)
endif()

if(Stdexec_ROOT AND HPX_WITH_FETCH_STDEXEC)
  hpx_warn(
    "Both Stdexec_ROOT and HPX_WITH_FETCH_STDEXEC are provided. HPX_WITH_FETCH_STDEXEC will take precedence."
  )
endif()

if(HPX_WITH_FETCH_STDEXEC)
  hpx_info(
    "HPX_WITH_FETCH_STDEXEC=${HPX_WITH_FETCH_STDEXEC}, Stdexec will be fetched using CMake's FetchContent and installed alongside HPX (HPX_WITH_STDEXEC_TAG=${HPX_WITH_STDEXEC_TAG})"
  )

  include(FetchContent)
  fetchcontent_declare(
    Stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG ${HPX_WITH_STDEXEC_TAG}
  )

  fetchcontent_getproperties(Stdexec)
  if(NOT Stdexec_POPULATED)
    fetchcontent_populate(Stdexec)
  endif()
  set(Stdexec_ROOT ${stdexec_SOURCE_DIR})

  add_library(Stdexec INTERFACE)
  target_include_directories(
    Stdexec SYSTEM INTERFACE $<BUILD_INTERFACE:${stdexec_SOURCE_DIR}/include>
                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  install(
    TARGETS Stdexec
    EXPORT HPXStdexecTarget
    COMPONENT core
  )

  install(
    DIRECTORY ${Stdexec_ROOT}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT core
    FILES_MATCHING
    PATTERN "*.hpp"
  )

  export(
    TARGETS Stdexec
    NAMESPACE Stdexec::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXStdexecTarget.cmake"
  )

  install(
    EXPORT HPXStdexecTarget
    NAMESPACE Stdexec::
    FILE HPXStdexecTarget.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
    COMPONENT cmake
  )

  # TODO: Enforce a single spelling
  add_library(Stdexec::Stdexec ALIAS Stdexec)
  add_library(STDEXEC::stdexec ALIAS Stdexec)
else()
  find_package(Stdexec)

  if(NOT Stdexec_FOUND)
    hpx_error(
      "Stdexec could not be found, please specify Stdexec_ROOT to point to the correct location or enable HPX_WITH_FETCH_STDEXEC"
    )
  endif()
endif()

# stdexec is now unconditionally required; define HPX_HAVE_STDEXEC so that
# downstream code using #if defined(HPX_HAVE_STDEXEC) continues to work.
hpx_add_config_define(HPX_HAVE_STDEXEC)
