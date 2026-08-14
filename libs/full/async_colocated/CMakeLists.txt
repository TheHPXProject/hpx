# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(async_colocated_headers
    hpx/async_colocated/async_colocated_callback_fwd.hpp
    hpx/async_colocated/async_colocated_callback.hpp
    hpx/async_colocated/async_colocated_fwd.hpp
    hpx/async_colocated/async_colocated.hpp
    hpx/async_colocated/macros.hpp
    hpx/async_colocated/post_colocated_callback_fwd.hpp
    hpx/async_colocated/post_colocated_callback.hpp
    hpx/async_colocated/post_colocated_fwd.hpp
    hpx/async_colocated/post_colocated.hpp
    hpx/async_colocated/functional/colocated_helpers.hpp
    hpx/async_colocated/get_colocation_id.hpp
    hpx/async_colocated/register_post_colocated.hpp
    hpx/async_colocated/server/destroy_component.hpp
)

set(async_colocated_macros_headers hpx/async_colocated/macros.hpp)

# cmake-format: off
set(async_colocated_compat_headers
    hpx/async_distributed/detail/async_colocated_callback_fwd.hpp => hpx/async_colocated/async_colocated_callback_fwd.hpp
    hpx/async_distributed/detail/async_colocated_callback.hpp => hpx/async_colocated/async_colocated_callback.hpp
    hpx/async_distributed/detail/async_colocated_fwd.hpp => hpx/async_colocated/async_colocated_fwd.hpp
    hpx/async_distributed/detail/async_colocated.hpp => hpx/async_colocated/async_colocated.hpp
    hpx/async_distributed/applier/detail/apply_colocated_callback_fwd.hpp => hpx/async_colocated/post_colocated_callback_fwd.hpp
    hpx/async_distributed/applier/detail/apply_colocated_callback.hpp => hpx/async_colocated/post_colocated_callback.hpp
    hpx/async_distributed/applier/detail/apply_colocated_fwd.hpp  => hpx/async_colocated/post_colocated_fwd.hpp
    hpx/async_distributed/applier/detail/apply_colocated.hpp  => hpx/async_colocated/post_colocated.hpp
    hpx/runtime/applier/register_apply_colocated.hpp => hpx/async_colocated/register_post_colocated.hpp
    hpx/runtime/components/server/destroy_component.hpp => hpx/async_colocated/server/destroy_component.hpp
    hpx/runtime/get_colocation_id.hpp  => hpx/async_colocated/get_colocation_id.hpp
    hpx/util/functional/colocated_helpers.hpp  => hpx/async_colocated/functional/colocated_helpers.hpp
)
# cmake-format: on

set(async_colocated_sources server/destroy_component.cpp get_colocation_id.cpp)

include(HPX_AddModule)
add_hpx_module(
  full async_colocated
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${async_colocated_sources}
  HEADERS ${async_colocated_headers}
  MACRO_HEADERS ${async_colocated_macros_headers}
  COMPAT_HEADERS ${async_colocated_compat_headers}
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions_base hpx_agas_base hpx_async_distributed
                      hpx_components_base hpx_naming_base
  CMAKE_SUBDIRS examples tests
)
