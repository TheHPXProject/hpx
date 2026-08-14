# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(asio_headers hpx/asio/asio_util.hpp hpx/asio/map_hostnames.hpp
                 hpx/asio/tcp_util.hpp
)

# cmake-format: off
set(asio_compat_headers
    hpx/util/asio_util.hpp => hpx/modules/asio.hpp
    hpx/util/map_hostnames.hpp => hpx/modules/asio.hpp
)
# cmake-format: on

set(asio_sources asio_util.cpp map_hostnames.cpp)

include(HPX_AddModule)
add_hpx_module(
  core asio
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${asio_sources}
  HEADERS ${asio_headers}
  COMPAT_HEADERS ${asio_compat_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/asio/asio_util.hpp"
  DEPENDENCIES Asio::asio
  MODULE_DEPENDENCIES hpx_assertion hpx_config hpx_errors hpx_format
                      hpx_functional
  CMAKE_SUBDIRS examples tests
)
