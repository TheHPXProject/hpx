# Copyright (c) 2020-2024 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(async_base_headers
    hpx/async_base/async.hpp
    hpx/async_base/dataflow.hpp
    hpx/async_base/detail/policy_holder_query.hpp
    hpx/async_base/detail/query_first_fallback.hpp
    hpx/async_base/launch_policy.hpp
    hpx/async_base/post.hpp
    hpx/async_base/query_dispatch.hpp
    hpx/async_base/scheduling_properties.hpp
    hpx/async_base/sync.hpp
    hpx/async_base/traits/is_launch_policy.hpp
)

set(async_base_sources launch_policy.cpp)

# cmake-format: off
set(async_base_compat_headers
    hpx/async_base/apply.hpp => hpx/async_base/post.hpp
    hpx/runtime/launch_policy.hpp => hpx/future.hpp
    hpx/traits/is_launch_policy.hpp => hpx/future.hpp
)
# cmake-format: on

include(HPX_AddModule)
add_hpx_module(
  core async_base
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${async_base_headers}
  COMPAT_HEADERS ${async_base_compat_headers}
  SOURCES ${async_base_sources}
  MODULE_DEPENDENCIES
    hpx_allocator_support
    hpx_concepts
    hpx_config
    hpx_concurrency
    hpx_coroutines
    hpx_serialization
    hpx_tag_invoke
  CMAKE_SUBDIRS examples tests
)
