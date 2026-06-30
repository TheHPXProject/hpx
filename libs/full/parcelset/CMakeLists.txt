# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(parcelset_headers
    hpx/parcelset/coalescing_message_handler_registration.hpp
    hpx/parcelset/connection_cache.hpp
    hpx/parcelset/decode_parcels.hpp
    hpx/parcelset/detail/call_for_each.hpp
    hpx/parcelset/detail/parcel_await.hpp
    hpx/parcelset/detail/message_handler_interface_functions.hpp
    hpx/parcelset/encode_parcels.hpp
    hpx/parcelset/init_parcelports.hpp
    hpx/parcelset/macros.hpp
    hpx/parcelset/message_handler_fwd.hpp
    hpx/parcelset/parcel.hpp
    hpx/parcelset/parcelhandler.hpp
    hpx/parcelset/parcelport_impl.hpp
    hpx/parcelset/parcelport_connection.hpp
    hpx/parcelset/parcelset_fwd.hpp
)

set(parcelset_macro_headers hpx/parcelset/macros.hpp)

# cmake-format: off
set(parcelset_compat_headers
    hpx/plugins/parcel/coalescing_message_handler_registration.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/message_handler_fwd.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/connection_cache.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/decode_parcels.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/encode_parcels.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/parcel.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/parcelport_connection.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/parcelhandler.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset/parcelport_impl.hpp => hpx/modules/parcelset.hpp
    hpx/runtime/parcelset_fwd.hpp => hpx/modules/parcelset.hpp
)
# cmake-format: on

set(parcelset_sources
    decode_parcels.cpp
    detail/message_handler_interface_functions.cpp
    detail/parcel_await.cpp
    encode_parcels.cpp
    message_handler.cpp
    parcel.cpp
    parcelhandler.cpp
)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  set(parcelset_optional_module_dependencies
      hpx_actions hpx_actions_base hpx_components_base hpx_naming
      hpx_naming_base
  )
endif()

include(HPX_AddModule)
add_hpx_module(
  full parcelset
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${parcelset_sources}
  HEADERS ${parcelset_headers}
  ADD_TO_GLOBAL_HEADER
    "hpx/parcelset/detail/message_handler_interface_functions.hpp"
    "hpx/parcelset/detail/parcel_await.hpp"
  MACRO_HEADERS ${parcelset_macro_headers}
  COMPAT_HEADERS ${parcelset_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_parcelset_base hpx_plugin_factories
                      ${parcelset_optional_module_dependencies}
  CMAKE_SUBDIRS examples tests
)
