# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/components_base/include
set(components_base_headers
    hpx/components_base/agas_interface.hpp
    hpx/components_base/get_lva.hpp
    hpx/components_base/components_base_fwd.hpp
    hpx/components_base/component_type.hpp
    hpx/components_base/component_commandline.hpp
    hpx/components_base/component_startup_shutdown.hpp
    hpx/components_base/detail/agas_interface_functions.hpp
    hpx/components_base/generate_unique_ids.hpp
    hpx/components_base/macros.hpp
    hpx/components_base/pinned_ptr.hpp
    hpx/components_base/server/abstract_component_base.hpp
    hpx/components_base/server/abstract_migration_support.hpp
    hpx/components_base/server/component.hpp
    hpx/components_base/server/component_base.hpp
    hpx/components_base/server/component_heap.hpp
    hpx/components_base/server/create_component.hpp
    hpx/components_base/server/create_component_fwd.hpp
    hpx/components_base/server/fixed_component_base.hpp
    hpx/components_base/server/locking_hook.hpp
    hpx/components_base/server/managed_component_base.hpp
    hpx/components_base/server/migration_support.hpp
    hpx/components_base/server/one_size_heap_list.hpp
    hpx/components_base/server/wrapper_heap.hpp
    hpx/components_base/server/wrapper_heap_base.hpp
    hpx/components_base/server/wrapper_heap_list.hpp
    hpx/components_base/stub_base.hpp
    hpx/components_base/traits/action_decorate_function.hpp
    hpx/components_base/traits/component_config_data.hpp
    hpx/components_base/traits/component_heap_type.hpp
    hpx/components_base/traits/component_pin_support.hpp
    hpx/components_base/traits/component_supports_migration.hpp
    hpx/components_base/traits/component_type_is_compatible.hpp
    hpx/components_base/traits/component_type_database.hpp
    hpx/components_base/traits/is_component.hpp
    hpx/components_base/traits/managed_component_policies.hpp
)

set(components_base_macros_headers hpx/components_base/macros.hpp)

# Default location is $HPX_ROOT/libs/components_base/include_compatibility
# cmake-format: off
set(components_base_compat_headers
    hpx/runtime/agas/interface.hpp => hpx/modules/components_base.hpp
    hpx/runtime/get_lva.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/component_type.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/abstract_component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/component.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/managed_component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/pinned_ptr.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/simple_component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/abstract_component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/abstract_migration_support.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/component.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/component_database.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/component_heap.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/create_component.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/create_component_fwd.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/fixed_component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/locking_hook.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/managed_component_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/migration_support.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/wrapper_heap.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/wrapper_heap_base.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/server/wrapper_heap_list.hpp => hpx/modules/components_base.hpp
    hpx/runtime/components/stubs/stub_base.hpp => hpx/modules/components.hpp
    hpx/traits/action_decorate_function.hpp => hpx/modules/components_base.hpp
    hpx/traits/component_config_data.hpp => hpx/modules/components_base.hpp
    hpx/traits/component_heap_type.hpp => hpx/modules/components_base.hpp
    hpx/traits/component_pin_support.hpp => hpx/modules/components_base.hpp
    hpx/traits/component_supports_migration.hpp => hpx/modules/components_base.hpp
    hpx/traits/component_type_is_compatible.hpp => hpx/modules/components_base.hpp
    hpx/traits/component_type_database.hpp => hpx/modules/components_base.hpp
    hpx/traits/is_component.hpp => hpx/modules/components_base.hpp
    hpx/traits/managed_component_policies.hpp => hpx/modules/components_base.hpp
    hpx/util/generate_unique_ids.hpp => hpx/modules/components_base.hpp
    hpx/util/one_size_heap_list.hpp => hpx/modules/components_base.hpp
)
# cmake-format: on

set(components_base_sources
    address_ostream.cpp
    agas_interface.cpp
    component_type.cpp
    detail/agas_interface_functions.cpp
    generate_unique_ids.cpp
    server/component_base.cpp
    server/one_size_heap_list.cpp
    server/wrapper_heap.cpp
)

include(HPX_AddModule)
add_hpx_module(
  full components_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  ADD_TO_GLOBAL_HEADER "hpx/components_base/detail/agas_interface_functions.hpp"
  SOURCES ${components_base_sources}
  HEADERS ${components_base_headers}
  MACRO_HEADERS ${components_base_macros_headers}
  COMPAT_HEADERS ${components_base_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_naming_base hpx_parcelset_base
  CMAKE_SUBDIRS examples tests
)
