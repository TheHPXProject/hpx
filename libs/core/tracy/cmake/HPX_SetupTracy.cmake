# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_AddDefinitions)

if(NOT TARGET tracy::tracy)
  if(FETCHCONTENT_SOURCE_DIR_TRACY)
    hpx_info(
      "Tracy will be used through CMake's FetchContent from FETCHCONTENT_SOURCE_DIR_TRACY=${FETCHCONTENT_SOURCE_DIR_TRACY}"
    )
  else()
    hpx_info(
      "Tracy will be fetched using CMake's FetchContent (HPX_WITH_TRACY_TAG=${HPX_WITH_TRACY_TAG})"
    )
  endif()

  include(FetchContent)
  fetchcontent_declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy
    GIT_TAG ${HPX_WITH_TRACY_TAG}
    GIT_SHALLOW TRUE
  )

  # Set the correct build options for Tracy and make it available. 0.14 defaults
  # TRACY_ENABLE OFF; without forcing it on, a HPX build with HPX_WITH_TRACY=ON
  # compiles, links, and records nothing. TRACY_STATIC=ON keeps TracyClient a
  # static library even under BUILD_SHARED_LIBS=ON, so its link step does not
  # need HpxDbgHelp* to be resolvable outside hpx_tracy (LTO still picks OBJECT
  # visibility and is unaffected).
  set(TRACY_ENABLE
      ON
      CACHE BOOL "" FORCE
  )
  set(TRACY_FIBERS
      ON
      CACHE BOOL "" FORCE
  )
  set(TRACY_ON_DEMAND
      ON
      CACHE BOOL "" FORCE
  )
  set(TRACY_STATIC
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
  # Serialise Tracy's DbgHelp calls against HPX's own via the wrappers in
  # hpx_debugging (dbghelp_lock.cpp). TracyClient is static (forced above), so
  # HpxDbgHelp* resolves when hpx_tracy links.
  if(WIN32)
    target_compile_definitions(TracyClient PUBLIC TRACY_DBGHELP_LOCK=HpxDbgHelp)
  endif()
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
