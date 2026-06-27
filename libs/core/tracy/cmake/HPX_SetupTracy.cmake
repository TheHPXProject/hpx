# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_AddDefinitions)

# compatibility with older CMake versions
if(TRACY_ROOT AND NOT Tracy_ROOT)
  set(Tracy_ROOT
      ${TRACY_ROOT}
      CACHE PATH "Tracy base directory"
  )
  unset(TRACY_ROOT CACHE)
endif()

if(NOT HPX_WITH_TRACY_TAG)
  set(HPX_WITH_TRACY_TAG "v0.13.1")
endif()

if(NOT HPX_WITH_FETCH_TRACY)
  find_package(Tracy)
  if(NOT Tracy_FOUND)
    hpx_error(
      "Could not find Tracy. Set Tracy_ROOT as a CMake or environment variable to point to the Tracy root install directory. Alternatively, set HPX_WITH_FETCH_TRACY=ON to fetch Tracy using CMake's FetchContent (when using this option Asio will be installed together with HPX, be careful about conflicts with separately installed versions of Tracy)."
    )
  endif()
  if(TARGET Tracy::TracyClient AND NOT TARGET tracy::tracy)
    add_library(tracy::tracy ALIAS Tracy::TracyClient)
  endif()
elseif(NOT TARGET tracy::tracy)
  if(FETCHCONTENT_SOURCE_DIR_TRACY)
    hpx_info(
      "HPX_WITH_FETCH_TRACY=${HPX_WITH_FETCH_TRACY}, Tracy will be used through CMake's FetchContent and installed alongside HPX (FETCHCONTENT_SOURCE_DIR_TRACY=${FETCHCONTENT_SOURCE_DIR_TRACY})"
    )
  else()
    hpx_info(
      "HPX_WITH_FETCH_TRACY=${HPX_WITH_FETCH_TRACY}, TRACY will be fetched using CMake's FetchContent and installed alongside HPX (HPX_WITH_TRACY_TAG=${HPX_WITH_TRACY_TAG})"
    )
  endif()

  include(FetchContent)
  fetchcontent_declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy
    GIT_TAG ${HPX_WITH_TRACY_TAG}
    GIT_SHALLOW TRUE
  )

  # Set the correct build options for Tracy and make it available
  set(TRACY_FIBERS
      ON
      CACHE BOOL "" FORCE
  )
  set(TRACY_ON_DEMAND
      ON
      CACHE BOOL "" FORCE
  )

  fetchcontent_makeavailable(tracy)

  set(TRACY_ROOT ${tracy_SOURCE_DIR})

  # adjust more settings for the TracyClient target
  target_compile_definitions(
    TracyClient
    PUBLIC $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:TRACY_NO_VERIFY>
  )
  target_compile_definitions(
    TracyClient PUBLIC $<$<CONFIG:Debug>:TRACY_VERBOSE>
  )
  target_compile_features(TracyClient PRIVATE cxx_std_${HPX_CXX_STANDARD})

  # cmake-format: off
  set_target_properties(
    TracyClient PROPERTIES
        FOLDER "Core/Dependencies"
        POSITION_INDEPENDENT_CODE ON
  )
  # cmake-format: on

  add_library(tracy INTERFACE)
  target_include_directories(
    tracy SYSTEM INTERFACE $<BUILD_INTERFACE:${TRACY_ROOT}/public>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_link_libraries(tracy INTERFACE TracyClient)

  install(
    TARGETS tracy
    EXPORT HPXTracyTarget
    COMPONENT core
  )

  install(
    DIRECTORY ${TRACY_ROOT}/public/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT core
    FILES_MATCHING
    PATTERN "*.hpp"
  )

  export(
    TARGETS tracy
    NAMESPACE tracy::
    FILE "${CMAKE_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXTracyTarget.cmake"
  )

  install(
    EXPORT HPXTracyTarget
    NAMESPACE tracy::
    FILE HPXTracyTarget.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
    COMPONENT cmake
  )

  add_library(tracy::tracy ALIAS tracy)
endif()
