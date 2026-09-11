# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(compute_local_headers
    hpx/compute_local/detail/get_proxy_type.hpp
    hpx/compute_local/detail/iterator.hpp
    hpx/compute_local/host.hpp
    hpx/compute_local/host/block_allocator.hpp
    hpx/compute_local/host/block_executor.hpp
    hpx/compute_local/host/block_fork_join_executor.hpp
    hpx/compute_local/host/get_targets.hpp
    hpx/compute_local/host/numa_allocator.hpp
    hpx/compute_local/host/numa_binding_allocator.hpp
    hpx/compute_local/host/numa_domains.hpp
    hpx/compute_local/host/target.hpp
    hpx/compute_local/host/traits/access_target.hpp
    hpx/compute_local/macros.hpp
    hpx/compute_local/serialization/vector.hpp
    hpx/compute_local/traits/access_target.hpp
    hpx/compute_local/traits/allocator_traits.hpp
    hpx/compute_local/traits.hpp
    hpx/compute_local/vector.hpp
)

set(compute_local_macro_headers hpx/compute_local/macros.hpp)

# cmake-format: off
set(compute_local_compat_headers
    hpx/compute/vector.hpp => hpx/compute.hpp
    hpx/compute/traits/allocator_traits.hpp => hpx/compute.hpp
    hpx/compute/host/numa_allocator.hpp => hpx/compute.hpp
    hpx/compute/host/numa_binding_allocator.hpp => hpx/compute.hpp
)
# cmake-format: on

set(compute_local_sources get_host_targets.cpp host_target.cpp numa_domains.cpp)

include(HPX_AddModule)
add_hpx_module(
  core compute_local
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${compute_local_sources}
  HEADERS ${compute_local_headers}
  MACRO_HEADERS ${compute_local_macro_headers}
  COMPAT_HEADERS ${compute_local_compat_headers}
  MODULE_DEPENDENCIES
    hpx_algorithms
    hpx_allocator_support
    hpx_async_combinators
    hpx_async_local
    hpx_concepts
    hpx_config
    hpx_coroutines
    hpx_datastructures
    hpx_execution
    hpx_execution_base
    hpx_executors
    hpx_errors
    hpx_functional
    hpx_futures
    hpx_iterator_support
    hpx_resource_partitioner
    hpx_runtime_local
    hpx_serialization
    hpx_synchronization
    hpx_threadmanager
    hpx_topology
    hpx_type_support
  CMAKE_SUBDIRS examples tests
)
