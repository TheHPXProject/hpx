# Copyright (c) 2022 Srinivas Yadav
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(("${HPX_WITH_DATAPAR_BACKEND}" STREQUAL "SVE") AND NOT TARGET SVE::sve)

  # compatibility with older CMake versions
  if(SVE_ROOT AND NOT Sve_ROOT)
    set(Sve_ROOT
        ${SVE_ROOT}
        CACHE PATH "SVE base directory"
    )
    unset(SVE_ROOT CACHE)
  endif()

  if(HPX_WITH_FETCH_SVE)
    if(FETCHCONTENT_SOURCE_DIR_SVE)
      hpx_info(
        "HPX_WITH_FETCH_SVE=${HPX_WITH_FETCH_SVE}, SVE will be used through CMake's FetchContent and installed alongside HPX (FETCHCONTENT_SOURCE_DIR_SVE=${FETCHCONTENT_SOURCE_DIR_SVE})"
      )
    else()
      hpx_info(
        "HPX_WITH_FETCH_SVE=${HPX_WITH_FETCH_SVE}, SVE will be fetched using CMake's FetchContent and installed alongside HPX (HPX_WITH_Sve_TAG=${HPX_WITH_Sve_TAG})"
      )
    endif()

    if("${HPX_WITH_SVE_LENGTH}" STREQUAL "")
      hpx_error(
        "When using HPX_WITH_FETCH_SVE, set HPX_WITH_SVE_LENGTH to appropriate length based on the architecture."
      )
    endif()

    set(Sve_LENGTH "${HPX_WITH_SVE_LENGTH}")

    include(FetchContent)
    fetchcontent_declare(
      sve
      GIT_REPOSITORY https://github.com/TheHPXProject/sve.git
      GIT_TAG ${HPX_WITH_SVE_TAG}
    )

    fetchcontent_makeavailable(sve)

    set(Sve_ROOT ${sve_SOURCE_DIR})

    install(
      TARGETS sve
      EXPORT HPXSVETarget
      COMPONENT core
    )

    install(
      DIRECTORY ${Sve_ROOT}/include/
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      COMPONENT core
      FILES_MATCHING
      PATTERN "*.hpp"
      PATTERN "*.ipp"
    )

    export(
      TARGETS sve
      NAMESPACE SVE::
      FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXSVETarget.cmake"
    )

    install(
      EXPORT HPXSVETarget
      NAMESPACE SVE::
      FILE HPXSVETarget.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
      COMPONENT cmake
    )

  else()
    if(Sve_ROOT)
      find_package(SVE REQUIRED PATHS ${Sve_ROOT})
    else()
      hpx_error("Sve_ROOT not set")
    endif()
  endif()
endif()
