# Copyright (c) 2019-2024 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_FILESYSTEM_WITH_BOOST_FILESYSTEM_COMPATIBILITY)
  # In case find_package(HPX) is called multiple times
  if(NOT TARGET Boost::filesystem)

    find_package(
      Boost ${Boost_MINIMUM_VERSION} NO_POLICY_SCOPE CONFIG QUIET
      COMPONENTS filesystem
    )
    if(NOT Boost_FOUND)
      find_package(
        Boost ${Boost_MINIMUM_VERSION} NO_POLICY_SCOPE MODULE
        COMPONENTS filesystem
      )
    endif()

    if(NOT Boost_FILESYSTEM_FOUND)
      hpx_error(
        "Could not find Boost.Filesystem Provide a boost installation including the filesystem library"
      )
    endif()
  endif()

  if(NOT HPX_FIND_PACKAGE)
    hpx_add_config_define_namespace(
      DEFINE HPX_FILESYSTEM_HAVE_BOOST_FILESYSTEM_COMPATIBILITY
      NAMESPACE FILESYSTEM
    )
  endif()
else()
  if(NOT HPX_WITH_CXX17_FILESYSTEM)
    hpx_error(
      "Could not find std::filesystem. Use a compiler with support for "
      "the C++17 filesystem library or enable Boost.FileSystem compatibility "
      "(set HPX_FILESYSTEM_WITH_BOOST_FILESYSTEM_COMPATIBILITY to ON)"
    )
  endif()
endif()
