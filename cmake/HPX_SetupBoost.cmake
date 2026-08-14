# Copyright (c) 2024 Panos Syskakis
# Copyright (c) 2018 Christopher Hinz
# Copyright (c) 2014 Thomas Heller
# Copyright (c) 2007-2024 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# In case find_package(HPX) is called multiple times
if(NOT TARGET hpx_dependencies_boost)
  if(HPX_WITH_STATIC_LINKING)
    set(Boost_USE_STATIC_LIBS ON)
  endif()

  set(__boost_libraries "")

  if(HPX_WITH_GENERIC_CONTEXT_COROUTINES)
    # if context is needed, we should still link with boost thread and chrono
    set(__boost_libraries ${__boost_libraries} context thread chrono)
  endif()

  # compatibility with older CMake versions
  if(BOOST_ROOT AND NOT Boost_ROOT)
    set(Boost_ROOT
        ${BOOST_ROOT}
        CACHE PATH "Boost base directory"
    )
    unset(BOOST_ROOT CACHE)
  endif()

  if(NOT HPX_WITH_FETCH_BOOST OR HPX_FIND_PACKAGE)

    set(Boost_MINIMUM_VERSION
        "1.71"
        CACHE INTERNAL "1.71" FORCE
    )

    # Search using CONFIG mode first
    find_package(
      Boost ${Boost_MINIMUM_VERSION} CONFIG QUIET NO_POLICY_SCOPE
      COMPONENTS ${__boost_libraries} HINTS ${HPX_BOOST_ROOT} $ENV{BOOST_ROOT}
    )

    if(NOT Boost_FOUND)
      # Search using MODULE mode
      find_package(
        Boost ${Boost_MINIMUM_VERSION} MODULE QUIET NO_POLICY_SCOPE
        COMPONENTS ${__boost_libraries}
      )
    endif()

    if(NOT Boost_FOUND AND NOT Boost_USE_STATIC_LIBS)
      hpx_info(
        "Shared Boost not found, attempting to find static Boost libraries."
      )
      set(Boost_USE_STATIC_LIBS ON)
      find_package(
        Boost ${Boost_MINIMUM_VERSION} MODULE REQUIRED NO_POLICY_SCOPE
        COMPONENTS ${__boost_libraries}
      )
    endif()

    if(NOT Boost_FOUND)
      hpx_error(
        "Boost could not be found. Please either specify Boost_ROOT to point to the correct location, "
        "install it using your system's package manager, or configure CMake with -DHPX_WITH_FETCH_BOOST=ON "
        "to download it automatically."
      )
    endif()

    add_library(hpx_dependencies_boost INTERFACE IMPORTED)

    if(TARGET Boost::boost)
      target_link_libraries(hpx_dependencies_boost INTERFACE Boost::boost)
    else()
      target_link_libraries(hpx_dependencies_boost INTERFACE Boost::headers)
    endif()

    foreach(__boost_library ${__boost_libraries})
      target_link_libraries(
        hpx_dependencies_boost INTERFACE Boost::${__boost_library}
      )
    endforeach()

  elseif(NOT TARGET Boost::boost AND NOT HPX_FIND_PACKAGE)
    # Fetch Boost using CMake's FetchContent
    if(NOT HPX_WITH_BOOST_VERSION)
      set(HPX_WITH_BOOST_VERSION "1.86.0")
    endif()

    hpx_info(
      "HPX_WITH_FETCH_BOOST=${HPX_WITH_FETCH_BOOST}, Boost v${HPX_WITH_BOOST_VERSION} will be fetched using CMake's FetchContent"
    )

    include(FetchContent)
    fetchcontent_declare(
      Boost
      URL https://github.com/boostorg/boost/releases/download/boost-${HPX_WITH_BOOST_VERSION}/boost-${HPX_WITH_BOOST_VERSION}-cmake.tar.xz
      TLS_VERIFY true
      DOWNLOAD_EXTRACT_TIMESTAMP true SYSTEM
    )

    # Also need to explicitly list header-only dependencies, since Cmake-Boost
    # installs each library's headers individually, as opposed to b2-built
    # Boost.
    set(__boost_libraries
        ${__boost_libraries}
        accumulators
        bind
        config
        exception
        functional
        fusion
        optional
        parameter
        phoenix
        spirit
        variant
    )

    if(HPX_FILESYSTEM_WITH_BOOST_FILESYSTEM_COMPATIBILITY)
      set(__boost_libraries ${__boost_libraries} filesystem)
    endif()

    if(HPX_WITH_TOOLS)
      set(__boost_libraries ${__boost_libraries} regex)
    endif()

    if(HPX_WITH_DISTRIBUTED_RUNTIME)
      set(__boost_libraries ${__boost_libraries} iostreams)
    endif()

    set(BOOST_INCLUDE_LIBRARIES ${__boost_libraries})
    set(BOOST_SKIP_INSTALL_RULES OFF)

    fetchcontent_makeavailable(Boost)

    add_library(hpx_dependencies_boost INTERFACE)

    list(TRANSFORM __boost_libraries
         PREPEND "Boost::" OUTPUT_VARIABLE __boost_libraries_prefixed
    )

    target_link_libraries(
      hpx_dependencies_boost INTERFACE ${__boost_libraries_prefixed}
    )

    install(
      TARGETS hpx_dependencies_boost
      EXPORT HPXBoostTarget
      COMPONENT core
    )

    install(
      EXPORT HPXBoostTarget
      FILE HPXBoostTarget.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
    )

    export(
      TARGETS hpx_dependencies_boost
      FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXBoostTarget.cmake"
    )

  endif()

  # We are assuming that there is only one Boost Root
  if(NOT Boost_ROOT AND DEFINED ENV{BOOST_ROOT})
    set(Boost_ROOT $ENV{BOOST_ROOT})
  elseif(NOT Boost_ROOT)
    string(REPLACE "/include" "" Boost_ROOT "${Boost_INCLUDE_DIRS}")
  endif()

  if(Boost_ROOT)
    file(TO_CMAKE_PATH ${Boost_ROOT} Boost_ROOT)
  endif()

  if(HPX_WITH_HIP AND Boost_VERSION VERSION_LESS 1.78)
    target_compile_definitions(
      hpx_dependencies_boost
      INTERFACE "BOOST_NOINLINE=__attribute__ ((noinline))"
    )
  endif()

  include(HPX_AddDefinitions)

  # Boost preprocessor definitions
  if(NOT Boost_USE_STATIC_LIBS)
    hpx_add_config_cond_define(BOOST_ALL_DYN_LINK)
  endif()

  if(NOT MSVC)
    hpx_add_config_define(HPX_COROUTINE_NO_SEPARATE_CALL_SITES)
  endif()

  hpx_add_config_cond_define(BOOST_BIGINT_HAS_NATIVE_INT64)
endif()
