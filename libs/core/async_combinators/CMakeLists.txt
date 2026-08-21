# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(async_combinators_headers
    hpx/async_combinators/detail/throw_if_exceptional.hpp
    hpx/async_combinators/split_future.hpp
    hpx/async_combinators/wait_all.hpp
    hpx/async_combinators/wait_all_for.hpp
    hpx/async_combinators/wait_any.hpp
    hpx/async_combinators/wait_each.hpp
    hpx/async_combinators/wait_some.hpp
    hpx/async_combinators/when_all.hpp
    hpx/async_combinators/when_any.hpp
    hpx/async_combinators/when_each.hpp
    hpx/async_combinators/when_some.hpp
)

# cmake-format: off
set(async_combinators_compat_headers
    hpx/lcos/split_future.hpp => hpx/future.hpp
    hpx/lcos/wait_all.hpp => hpx/future.hpp
    hpx/lcos/wait_any.hpp => hpx/future.hpp
    hpx/lcos/wait_each.hpp => hpx/future.hpp
    hpx/lcos/wait_some.hpp => hpx/future.hpp
    hpx/lcos/when_all.hpp => hpx/future.hpp
    hpx/lcos/when_any.hpp => hpx/future.hpp
    hpx/lcos/when_each.hpp => hpx/future.hpp
    hpx/lcos/when_some.hpp => hpx/future.hpp
)
# cmake-format: on

include(HPX_AddModule)
add_hpx_module(
  core async_combinators
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  HEADERS ${async_combinators_headers}
  COMPAT_HEADERS ${async_combinators_compat_headers}
  MODULE_DEPENDENCIES
    hpx_allocator_support
    hpx_async_base
    hpx_concurrency
    hpx_config
    hpx_datastructures
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_futures
    hpx_iterator_support
    hpx_memory
    hpx_pack_traversal
    hpx_preprocessor
    hpx_thread_support
    hpx_timing
    hpx_type_support
    hpx_util
  CMAKE_SUBDIRS examples tests
)
