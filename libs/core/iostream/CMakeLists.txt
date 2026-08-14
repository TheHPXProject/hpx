# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

hpx_option(
  HPX_IOSTREAM_WITH_WIDE_STREAMS BOOL "Enable wide IO streams. (default: OFF)"
  OFF ADVANCED
  CATEGORY "Modules"
  MODULE iostream
)

if(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
  hpx_add_config_define_namespace(
    DEFINE HPX_IOSTREAM_HAVE_WIDE_STREAMS NAMESPACE iostream
  )
endif()

set(iostream_headers
    hpx/iostream/categories.hpp
    hpx/iostream/chain.hpp
    hpx/iostream/char_traits.hpp
    hpx/iostream/checked_operations.hpp
    hpx/iostream/close.hpp
    hpx/iostream/combine.hpp
    hpx/iostream/compose.hpp
    hpx/iostream/concepts.hpp
    hpx/iostream/constants.hpp
    hpx/iostream/copy.hpp
    hpx/iostream/detail/access_control.hpp
    hpx/iostream/detail/adapter/concept_adapter.hpp
    hpx/iostream/detail/adapter/direct_adapter.hpp
    hpx/iostream/detail/adapter/filter_adapter.hpp
    hpx/iostream/detail/adapter/mode_adapter.hpp
    hpx/iostream/detail/adapter/non_blocking_adapter.hpp
    hpx/iostream/detail/adapter/output_iterator_adapter.hpp
    hpx/iostream/detail/adapter/range_adapter.hpp
    hpx/iostream/detail/buffer.hpp
    hpx/iostream/detail/dispatch.hpp
    hpx/iostream/detail/double_object.hpp
    hpx/iostream/detail/error.hpp
    hpx/iostream/detail/execute.hpp
    hpx/iostream/detail/functional.hpp
    hpx/iostream/detail/resolve.hpp
    hpx/iostream/detail/streambuf/chainbuf.hpp
    hpx/iostream/detail/streambuf/direct_streambuf.hpp
    hpx/iostream/detail/streambuf/indirect_streambuf.hpp
    hpx/iostream/detail/streambuf/linked_streambuf.hpp
    hpx/iostream/detail/wrap_unwrap.hpp
    hpx/iostream/device/array.hpp
    hpx/iostream/device/back_inserter.hpp
    hpx/iostream/device/file.hpp
    hpx/iostream/device/file_descriptor.hpp
    hpx/iostream/device/null.hpp
    hpx/iostream/filter/symmetric.hpp
    hpx/iostream/filtering_stream.hpp
    hpx/iostream/filtering_streambuf.hpp
    hpx/iostream/flush.hpp
    hpx/iostream/imbue.hpp
    hpx/iostream/input_sequence.hpp
    hpx/iostream/operations_fwd.hpp
    hpx/iostream/operations.hpp
    hpx/iostream/optimal_buffer_size.hpp
    hpx/iostream/output_sequence.hpp
    hpx/iostream/pipeline.hpp
    hpx/iostream/positioning.hpp
    hpx/iostream/read.hpp
    hpx/iostream/seek.hpp
    hpx/iostream/stream.hpp
    hpx/iostream/stream_buffer.hpp
    hpx/iostream/tee.hpp
    hpx/iostream/traits.hpp
    hpx/iostream/write.hpp
)

set(iostream_sources file_descriptor.cpp)

set(additional_dependencies)
set(additional_include_directories)

if(HPX_WITH_COMPRESSION_BZIP2)
  set(iostream_headers ${iostream_headers} hpx/iostream/filter/bzip2.hpp)
  set(iostream_sources ${iostream_sources} bzip2.cpp)

  find_package(BZip2 REQUIRED)
  set(additional_include_directories ${additional_include_directories}
                                     ${BZIP2_INCLUDE_DIR}
  )
  set(additional_dependencies ${additional_dependencies} ${BZIP2_LIBRARIES})
endif()

if(HPX_WITH_COMPRESSION_ZLIB)
  set(iostream_headers ${iostream_headers} hpx/iostream/filter/zlib.hpp)
  set(iostream_sources ${iostream_sources} zlib.cpp)

  find_package(ZLIB REQUIRED)
  set(additional_include_directories ${additional_include_directories}
                                     ${ZLIB_INCLUDE_DIR}
  )
  set(additional_dependencies ${additional_dependencies} ${ZLIB_LIBRARIES})
endif()

include(HPX_AddModule)
add_hpx_module(
  core iostream
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${iostream_sources}
  HEADERS ${iostream_headers}
  COMPAT_HEADERS ${iostream_compat_headers}
  DEPENDENCIES ${additional_dependencies}
  INCLUDE_DIRECTORIES ${additional_include_directories}
  MODULE_DEPENDENCIES
    hpx_config
    hpx_assertion
    hpx_datastructures
    hpx_filesystem
    hpx_functional
    hpx_iterator_support
    hpx_type_support
  CMAKE_SUBDIRS tests
)
