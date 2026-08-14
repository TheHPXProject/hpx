# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(parcelports_headers hpx/parcelports/init_all_parcelports.hpp)

set(parcelports_sources static_parcelports.cpp)

set(parcelports_generated_headers hpx/parcelports/static_parcelports.hpp)

set(parcelport_module_dependencies)
set(parcelport_export_declarations)
set(parcelport_init_definitions)

foreach(parcelport ${HPX_STATIC_PARCELPORT_PLUGINS})
  set(parcelport_module_dependencies ${parcelport_module_dependencies}
                                     hpx_${parcelport}
  )

  # generate header defining initialization functions for parcelports
  set(parcelport_export_declarations
      "${parcelport_export_declarations}HPX_EXPORT hpx::plugins::parcelport_factory_base *\n"
  )
  set(parcelport_export_declarations
      "${parcelport_export_declarations}    ${parcelport}_factory_init(\n"
  )
  set(parcelport_export_declarations
      "${parcelport_export_declarations}        std::vector<hpx::plugins::parcelport_factory_base *>& factories);\n"
  )
  set(parcelport_init_definitions
      "${parcelport_init_definitions}        ${parcelport}_factory_init(factories);\n"
  )
endforeach()

include(HPX_ConfigureIfChanged)
hpx_configure_if_changed(
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/static_parcelports.hpp.in"
  OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/include/hpx/parcelports/static_parcelports.hpp"
  CONFIGURE_ARGS @ONLY
)

include(HPX_AddModule)
add_hpx_module(
  full parcelports
  GLOBAL_HEADER_GEN OFF
  GENERATED_HEADERS ${parcelports_generated_headers}
  SOURCES ${parcelports_sources}
  HEADERS ${parcelports_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_parcelset hpx_plugin_factories
                      ${parcelport_module_dependencies}
)
