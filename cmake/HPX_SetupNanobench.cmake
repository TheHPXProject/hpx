# Copyright (c) 2024 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(FetchContent)

fetchcontent_declare(
  nanobench
  GIT_REPOSITORY https://github.com/martinus/nanobench.git
  GIT_TAG v4.3.11
  GIT_SHALLOW TRUE
)

if(NOT nanobench_POPULATED)
  fetchcontent_populate(nanobench)
endif()
set(Nanobench_ROOT ${nanobench_SOURCE_DIR})

add_library(nanobench INTERFACE)
target_include_directories(
  nanobench SYSTEM INTERFACE $<BUILD_INTERFACE:${Nanobench_ROOT}/src/include/>
                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
  TARGETS nanobench
  EXPORT HPXNanobenchTarget
  COMPONENT core
)

install(
  FILES ${Nanobench_ROOT}/src/include/nanobench.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT core
)

export(
  TARGETS nanobench
  NAMESPACE nanobench::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXNanobenchTarget.cmake"
)

install(
  EXPORT HPXNanobenchTarget
  NAMESPACE nanobench::
  FILE HPXNanobenchTarget.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
  COMPONENT cmake
)

add_library(nanobench::nanobench ALIAS nanobench)
