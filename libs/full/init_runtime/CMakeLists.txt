# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(init_runtime_headers
    hpx/hpx_finalize.hpp
    hpx/hpx_init.hpp
    hpx/hpx_init_params.hpp
    hpx/hpx_start.hpp
    hpx/hpx_suspend.hpp
    hpx/hpx_user_main_config.hpp
    hpx/init.hpp
    hpx/init_runtime/detail/init_logging.hpp
    hpx/init_runtime/detail/run_or_start.hpp
    hpx/init_runtime/detail/main_winsocket.hpp
    hpx/init_runtime/finalize.hpp
    hpx/init_runtime/init_impl.hpp
    hpx/init_runtime/start_impl.hpp
    hpx/init_runtime/suspend.hpp
)

set(init_runtime_sources hpx_init.cpp hpx_main_winsocket.cpp init_logging.cpp)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  set(init_runtime_headers ${init_runtime_headers}
                           hpx/init_runtime/pre_main.hpp
  )

  set(init_runtime_sources ${init_runtime_sources} pre_main.cpp)

  set(init_runtime_optional_module_dependencies
      hpx_agas
      hpx_actions_base
      hpx_async_distributed
      hpx_collectives
      hpx_components_base
      hpx_naming
      hpx_parcelports
      hpx_performance_counters
      hpx_runtime_components
      hpx_runtime_distributed
      hpx_supervision
  )
endif()

include(HPX_AddModule)
add_hpx_module(
  full init_runtime
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${init_runtime_headers}
  SOURCES ${init_runtime_sources}
  ADD_TO_GLOBAL_HEADER "hpx/init_runtime/detail/run_or_start.hpp"
  EXCLUDE_FROM_GLOBAL_HEADER
    "hpx/hpx_finalize.hpp"
    "hpx/hpx_init.hpp"
    "hpx/hpx_init_params.hpp"
    "hpx/hpx_start.hpp"
    "hpx/hpx_suspend.hpp"
    "hpx/init.hpp"
    "hpx/init_runtime/init_impl.hpp"
    "hpx/init_runtime/main_winsocket.hpp"
    "hpx/init_runtime/start_impl.hpp"
    "hpx/init_runtime/pre_main.hpp"
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_command_line_handling hpx_parcelset_base
                      ${init_runtime_optional_module_dependencies}
  CMAKE_SUBDIRS examples tests
)
