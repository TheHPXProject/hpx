# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/actions_base/include
set(actions_base_headers
    hpx/actions_base/actions_base_fwd.hpp
    hpx/actions_base/actions_base_support.hpp
    hpx/actions_base/action_priority.hpp
    hpx/actions_base/action_stacksize.hpp
    hpx/actions_base/basic_action.hpp
    hpx/actions_base/basic_action_fwd.hpp
    hpx/actions_base/component_action.hpp
    hpx/actions_base/reflect_action.hpp
    hpx/actions_base/detail/action_factory.hpp
    hpx/actions_base/detail/invocation_count_registry.hpp
    hpx/actions_base/detail/per_action_data_counter_registry.hpp
    hpx/actions_base/lambda_to_action.hpp
    hpx/actions_base/macros.hpp
    hpx/actions_base/plain_action.hpp
    hpx/actions_base/preassigned_action_id.hpp
    hpx/actions_base/traits/action_continuation.hpp
    hpx/actions_base/traits/action_decorate_continuation.hpp
    hpx/actions_base/traits/action_does_termination_detection.hpp
    hpx/actions_base/traits/action_is_target_valid.hpp
    hpx/actions_base/traits/action_priority.hpp
    hpx/actions_base/traits/action_remote_result.hpp
    hpx/actions_base/traits/action_schedule_thread.hpp
    hpx/actions_base/traits/action_select_direct_execution.hpp
    hpx/actions_base/traits/action_stacksize.hpp
    hpx/actions_base/traits/action_trigger_continuation_fwd.hpp
    hpx/actions_base/traits/action_was_object_migrated.hpp
    hpx/actions_base/traits/extract_action.hpp
    hpx/actions_base/traits/is_client.hpp
    hpx/actions_base/traits/is_continuation.hpp
    hpx/actions_base/traits/is_distribution_policy.hpp
    hpx/actions_base/traits/is_valid_action.hpp
)
set(actions_base_macro_headers hpx/actions_base/macros.hpp)

# Default location is $HPX_ROOT/libs/actions_base/include_compatibility
# cmake-format: off
set(actions_base_compat_headers
    hpx/runtime/actions_base_fwd.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/action_priority.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/action_support.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/basic_action.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/basic_action_fwd.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/component_action.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/detail/action_factory.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/detail/invocation_count_registry.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/lambda_to_action.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/plain_action.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/actions/preassigned_action_id.hpp => hpx/modules/actions_base.hpp
    hpx/runtime/parcelset/detail/per_action_data_counter_registry.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_continuation.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_does_termination_detection.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_is_target_valid.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_priority.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_remote_result.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_schedule_thread.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_select_direct_execution.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_stacksize.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_was_object_migrated.hpp => hpx/modules/actions_base.hpp
    hpx/traits/extract_action.hpp => hpx/modules/actions_base.hpp
    hpx/traits/is_client.hpp => hpx/modules/actions_base.hpp
    hpx/traits/is_continuation.hpp => hpx/modules/actions_base.hpp
    hpx/traits/is_distribution_policy.hpp => hpx/modules/actions_base.hpp
    hpx/traits/is_valid_action.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_continuation.hpp => hpx/modules/actions_base.hpp
    hpx/traits/action_decorate_continuation.hpp => hpx/modules/actions_base.hpp
)
# cmake-format: on

set(actions_base_sources
    detail/action_factory.cpp detail/invocation_count_registry.cpp
    detail/per_action_data_counter_registry.cpp
)

include(HPX_AddModule)

add_hpx_module(
  full actions_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${actions_base_sources}
  HEADERS ${actions_base_headers}
  MACRO_HEADERS ${actions_base_macro_headers}
  COMPAT_HEADERS ${actions_base_compat_headers}
  MODULE_DEPENDENCIES hpx_components_base hpx_naming_base
  DEPENDENCIES hpx_core
  CMAKE_SUBDIRS examples tests
)
