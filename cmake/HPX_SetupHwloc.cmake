# Copyright (c) 2007-2019 Hartmut Kaiser
# Copyright (c) 2011-2014 Thomas Heller
# Copyright (c) 2007-2008 Chirag Dekate
# Copyright (c)      2011 Bryce Lelbach
# Copyright (c)      2011 Vinay C Amatya
# Copyright (c)      2013 Jeroen Habraken
# Copyright (c) 2014-2016 Andreas Schaefer
# Copyright (c) 2017      Abhimanyu Rawat
# Copyright (c) 2017      Google
# Copyright (c) 2017      Taeguk Kwon
# Copyright (c) 2025      Srinivas Yadav Singanaboina
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_FETCH_HWLOC)
  find_package(Hwloc)
  if(NOT Hwloc_FOUND)
    hpx_error(
      "Hwloc could not be found. Please either specify Hwloc_ROOT to point to the correct location, "
      "install it using your system's package manager, or configure CMake with -DHPX_WITH_FETCH_HWLOC=ON "
      "to download it automatically."
    )
  endif()
else()
  # Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
  endif()

  if(NOT HPX_WITH_HWLOC_VERSION)
    set(HPX_WITH_HWLOC_VERSION "2.10")
    set(HPX_WITH_HWLOC_RELEASE "2.10.0")
  endif()

  hpx_info(
    "HPX_WITH_FETCH_HWLOC=${HPX_WITH_FETCH_HWLOC}, Hwloc v${HPX_WITH_HWLOC_RELEASE} will be fetched using CMake's FetchContent"
  )

  if(UNIX)
    include(FetchContent)
    fetchcontent_declare(
      HWLoc
      URL https://download.open-mpi.org/release/hwloc/v${HPX_WITH_HWLOC_VERSION}/hwloc-${HPX_WITH_HWLOC_RELEASE}.tar.gz
      TLS_VERIFY true
    )
    if(NOT HWLoc_POPULATED)
      fetchcontent_populate(HWLoc)
      set(HPX_HWLOC_BUILD_DIR_INSTALLATION_PATH
          "${FETCHCONTENT_BASE_DIR}/hwloc_installed"
      )
      if(NOT Hwloc_BUILD_INSTALLED)
        execute_process(
          COMMAND
            sh -c
            "cd ${FETCHCONTENT_BASE_DIR}/hwloc-src && ./configure --prefix=${HPX_HWLOC_BUILD_DIR_INSTALLATION_PATH} && make -j && make install"
        )
        set(Hwloc_BUILD_INSTALLED
            TRUE
            CACHE INTERNAL ""
        )
      else()
        message(
          "HWLoc is installed at ${FETCHCONTENT_BASE_DIR}/hwloc-installed"
        )
      endif()
      set(HWLOC_ROOT ${HPX_HWLOC_BUILD_DIR_INSTALLATION_PATH})
    endif()
    set(Hwloc_INCLUDE_DIR
        ${HWLOC_ROOT}/include
        CACHE INTERNAL ""
    )
    if(APPLE)
      set(Hwloc_LIBRARY
          ${HWLOC_ROOT}/lib/libhwloc.dylib
          CACHE INTERNAL ""
      )
    else()
      set(Hwloc_LIBRARY
          ${HWLOC_ROOT}/lib/libhwloc.so
          CACHE INTERNAL ""
      )
    endif()

  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(hwloc_subdir win64)
    else()
      set(hwloc_subdir win32)
    endif()

    fetchcontent_declare(
      HWLoc
      URL https://download.open-mpi.org/release/hwloc/v${HPX_WITH_HWLOC_VERSION}/hwloc-${hwloc_subdir}-build-${HPX_WITH_HWLOC_RELEASE}.zip
      TLS_VERIFY true
    )
    if(NOT HWLoc_POPULATED)
      fetchcontent_populate(HWLoc)
    endif()
    set(HWLOC_ROOT
        "${FETCHCONTENT_BASE_DIR}/hwloc-src"
        CACHE INTERNAL ""
    )
    include_directories(${HWLOC_ROOT}/include)
    link_directories(${HWLOC_ROOT}/lib)
    set(Hwloc_INCLUDE_DIR
        ${HWLOC_ROOT}/include
        CACHE INTERNAL ""
    )
    set(Hwloc_LIBRARY
        ${HWLOC_ROOT}/lib/libhwloc.dll.a
        CACHE INTERNAL ""
    )
    file(GLOB HWLOC_DLL ${HWLOC_ROOT}/bin/libhwloc*.dll)
  else()
    hpx_error(
      "Building HWLOC as part of HPX' configuration process is not supported on this platform"
    )
  endif() # End hwloc installation

  find_package(Hwloc)

  if(HPX_WITH_FETCH_HWLOC AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if(RUNTIME_OUTPUT_DIRECTORY)
      set(EXE_DIRECTORY_PATH "${RUNTIME_OUTPUT_DIRECTORY}")
    else()
      set(EXE_DIRECTORY_PATH "${CMAKE_BINARY_DIR}/$<CONFIG>/bin/")
    endif()

    message("Copying ${HWLOC_DLL} to ${EXE_DIRECTORY_PATH}")
    add_custom_target(
      HwlocDLL ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory ${EXE_DIRECTORY_PATH}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${HWLOC_DLL}
              ${EXE_DIRECTORY_PATH}
    )
    install(FILES ${HWLOC_DLL} DESTINATION ${CMAKE_INSTALL_BINDIR})
    add_hpx_pseudo_target(HwlocDLL)
    add_dependencies(Hwloc::hwloc HwlocDLL)
  endif()

  install(
    DIRECTORY ${HWLOC_ROOT}/
    DESTINATION ${HPX_HWLOC_INSTALL_PATH}
    COMPONENT core
  )
endif()
