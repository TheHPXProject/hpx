# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(plugin_factories_headers
    hpx/plugin_factories/binary_filter_factory.hpp
    hpx/plugin_factories/binary_filter_factory_base.hpp
    hpx/plugin_factories/macros.hpp
    hpx/plugin_factories/message_handler_factory.hpp
    hpx/plugin_factories/message_handler_factory_base.hpp
    hpx/plugin_factories/parcelport_factory.hpp
    hpx/plugin_factories/parcelport_factory_base.hpp
    hpx/plugin_factories/plugin_factory_base.hpp
    hpx/plugin_factories/plugin_registry.hpp
    hpx/plugin_factories/unique_plugin_name.hpp
)

set(plugin_factories_macro_headers hpx/plugin_factories/macros.hpp)

# cmake-format: off
set(plugin_factories_compat_headers
    hpx/plugins/binary_filter_factory.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/binary_filter_factory_base.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/message_handler_factory.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/message_handler_factory_base.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/parcelport_factory.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/parcelport_factory_base.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/plugin_factory_base.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/plugin_registry.hpp => hpx/modules/plugin_factories.hpp
    hpx/plugins/unique_plugin_name.hpp => hpx/modules/plugin_factories.hpp
)
# cmake-format: on

set(plugin_factories_sources parcelport_factory_base.cpp)

include(HPX_AddModule)
add_hpx_module(
  full plugin_factories
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${plugin_factories_sources}
  HEADERS ${plugin_factories_headers}
  MACRO_HEADERS ${plugin_factories_macro_headers}
  COMPAT_HEADERS ${plugin_factories_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_command_line_handling hpx_parcelset_base
  CMAKE_SUBDIRS examples tests
)
