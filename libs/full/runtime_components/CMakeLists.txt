# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(runtime_components_headers
    hpx/runtime_components/components_fwd.hpp
    hpx/runtime_components/component_factory.hpp
    hpx/runtime_components/component_registry.hpp
    hpx/runtime_components/console_error_sink.hpp
    hpx/runtime_components/console_logging.hpp
    hpx/runtime_components/create_component_helpers.hpp
    hpx/runtime_components/derived_component_factory.hpp
    hpx/runtime_components/distributed_metadata_base.hpp
    hpx/runtime_components/macros.hpp
    hpx/runtime_components/new.hpp
    hpx/runtime_components/server/console_error_sink.hpp
    hpx/runtime_components/server/console_error_sink_singleton.hpp
    hpx/runtime_components/server/console_logging.hpp
)

set(runtime_components_macro_headers hpx/runtime_components/macros.hpp)

# cmake-format: off
set(runtime_components_compat_headers
    hpx/runtime/components_fwd.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/component_factory.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/component_registry.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/console_error_sink.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/console_logging.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/create_component_helpers.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/derived_component_factory.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/new.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/console_error_sink.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/console_logging.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/console_error_sink_singleton.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/copy_component.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/distributed_metadata_base.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/migrate_component.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/console_logging.hpp => hpx/modules/runtime_components.hpp
    hpx/runtime/components/server/console_logging.hpp => hpx/modules/runtime_components.hpp
)
# cmake-format: on

set(runtime_components_sources
    component_registry.cpp console_error_sink.cpp console_logging.cpp
    server/console_error_sink_server.cpp
    server/console_error_sink_singleton.cpp server/console_logging_server.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full runtime_components
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${runtime_components_headers}
  MACRO_HEADERS ${runtime_components_macro_headers}
  SOURCES ${runtime_components_sources}
  COMPAT_HEADERS ${runtime_components_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES
    hpx_actions
    hpx_actions_base
    hpx_agas
    hpx_async_colocated
    hpx_async_distributed
    hpx_components
    hpx_components_base
    hpx_naming_base
  CMAKE_SUBDIRS examples tests
)
