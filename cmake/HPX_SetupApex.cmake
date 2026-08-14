# Copyright (c) 2007-2022 Hartmut Kaiser
# Copyright (c) 2011-2014 Thomas Heller
# Copyright (c) 2007-2008 Chirag Dekate
# Copyright (c)      2011 Bryce Lelbach
# Copyright (c)      2011 Vinay C Amatya
# Copyright (c)      2013 Jeroen Habraken
# Copyright (c) 2014-2016 Andreas Schaefer
# Copyright (c) 2017      Abhimanyu Rawat
# Copyright (c) 2017      Google
# Copyright (c) 2017      Taeguk Kwon
# Copyright (c) 2018 Christopher Hinz
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_APEX AND NOT TARGET APEX::apex)
  # compatibility with older CMake versions
  if(APEX_ROOT AND NOT Apex_ROOT)
    set(Apex_ROOT
        ${APEX_ROOT}
        CACHE PATH "Apex base directory"
    )
    unset(APEX_ROOT CACHE)
  endif()
  if(MSR_ROOT AND NOT Msr_ROOT)
    set(Msr_ROOT
        ${MSR_ROOT}
        CACHE PATH "MSR base directory"
    )
    unset(MSR_ROOT CACHE)
  endif()
  if(OTF2_ROOT AND NOT Otf2_ROOT)
    set(Otf2_ROOT
        ${OTF2_ROOT}
        CACHE PATH "OTF2 base directory"
    )
    unset(OTF2_ROOT CACHE)
  endif()

  if(NOT HPX_FIND_PACKAGE)
    if(NOT "${Apex_ROOT}" AND DEFINED ENV{APEX_ROOT})
      set(Apex_ROOT "$ENV{APEX_ROOT}")
    endif()

    # We want to track parent dependencies
    hpx_add_config_define(HPX_HAVE_THREAD_PARENT_REFERENCE)

    if(HPX_WITH_FETCH_APEX)
      set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

      # If Apex_ROOT not specified, local clone into hpx source dir
      include(FetchContent)
      fetchcontent_declare(
        apex
        GIT_REPOSITORY ${HPX_WITH_APEX_REPOSITORY}
        GIT_TAG ${HPX_WITH_APEX_TAG}
      )

      fetchcontent_getproperties(apex)
      if(NOT apex_POPULATED)
        # maintain compatibility
        if(HPX_WITH_APEX_NO_UPDATE)
          set(FETCHCONTENT_UPDATES_DISCONNECTED_APEX ON)
        endif()
        fetchcontent_populate(apex)
      endif()

      # check again to make sure we have received a copy of APEX
      fetchcontent_getproperties(apex)
      if(NOT apex_POPULATED)
        hpx_error("APEX could not be populated with HPX_WITH_APEX=On")
      endif()
      set(Apex_ROOT ${apex_SOURCE_DIR})

      hpx_info("Apex_ROOT is not set. Cloning APEX into ${apex_SOURCE_DIR}.")
      list(APPEND CMAKE_MODULE_PATH "${Apex_ROOT}/cmake/Modules")
      add_subdirectory(${Apex_ROOT}/src/apex ${CMAKE_BINARY_DIR}/apex/src/apex)
    endif()

    if(Amplifier_FOUND)
      hpx_error("Amplifier_FOUND has been set. Please disable the use of the \
        Intel Amplifier (WITH_AMPLIFIER=Off) in order to use APEX"
      )
    endif()
  endif()

  if(HPX_WITH_FETCH_APEX)
    add_library(APEX::apex INTERFACE IMPORTED)
    if(HPX_FIND_PACKAGE)
      target_link_libraries(APEX::apex INTERFACE HPX::apex)
    else()
      target_link_libraries(APEX::apex INTERFACE apex)
    endif()
  else()
    if(Apex_ROOT)
      find_package(APEX REQUIRED PATHS ${Apex_ROOT})
    else()
      hpx_error("Apex_ROOT not set.")
    endif()
  endif()

  if((UNIX AND NOT APPLE) OR MINGW)
    target_link_options(APEX::apex INTERFACE "-Wl,-no-as-needed")
  endif()

  if((HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI) OR HPX_WITH_ASYNC_MPI)
    # APEX now depends on MPI itself
    if(NOT TARGET Mpi::mpi)
      include(HPX_SetupMPI)
      hpx_setup_mpi()
    endif()
    target_link_libraries(APEX::apex INTERFACE Mpi::mpi)
  endif()

  # handle optional ITTNotify library (private dependency, skip when called in
  # find_package(HPX))
  if(HPX_WITH_ITTNOTIFY AND NOT HPX_FIND_PACKAGE)
    add_subdirectory(${Apex_ROOT}/src/ITTNotify)
    if(NOT Ittnotify_FOUND)
      hpx_error("ITTNotify could not be found and HPX_WITH_ITTNOTIFY=On")
    endif()

    add_library(ITTNotify::ittnotify INTERFACE IMPORTED)
    target_include_directories(
      ITTNotify::ittnotify SYSTEM INTERFACE ${Ittnotify_SOURCE_DIR}
    )
    target_link_libraries(APEX::apex INTERFACE ITTNotify::ittnotify)
  endif()
endif()
