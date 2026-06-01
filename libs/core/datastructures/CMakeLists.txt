# Copyright (c) 2019-2023 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Compatibility with std::tuple (i.e. using hpx::get() with std::tuple)
hpx_option(
  HPX_DATASTRUCTURES_WITH_ADAPT_STD_TUPLE BOOL
  "Enable compatibility of hpx::get with std::tuple. (default: ON)" ON ADVANCED
  CATEGORY "Modules"
  MODULE DATASTRUCTURES
)

if(HPX_DATASTRUCTURES_WITH_ADAPT_STD_TUPLE)
  hpx_add_config_define_namespace(
    DEFINE HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE NAMESPACE DATASTRUCTURES
  )
endif()

# Compatibility with std::variant (i.e. using hpx::get() with std::variant)
set(HPX_DATASTRUCTURES_WITH_ADAPT_STD_VARIANT_DEFAULT ON)
if(HPX_WITH_CXX17_COPY_ELISION)
  # we shouldn't implement hpx::get for variant if hpx::variant is an alias for
  # std::variant
  set(HPX_DATASTRUCTURES_WITH_ADAPT_STD_VARIANT_DEFAULT OFF)
endif()

hpx_option(
  HPX_DATASTRUCTURES_WITH_ADAPT_STD_VARIANT
  BOOL
  "Enable compatibility of hpx::get with std::variant.
 (default: ${HPX_DATASTRUCTURES_WITH_ADAPT_STD_VARIANT_DEFAULT})"
  ${HPX_DATASTRUCTURES_WITH_ADAPT_STD_VARIANT_DEFAULT}
  ADVANCED
  CATEGORY "Modules"
  MODULE DATASTRUCTURES
)

if(HPX_DATASTRUCTURES_WITH_ADAPT_STD_VARIANT)
  hpx_add_config_define_namespace(
    DEFINE HPX_DATASTRUCTURES_HAVE_ADAPT_STD_VARIANT NAMESPACE DATASTRUCTURES
  )
endif()

set(datastructures_headers
    hpx/datastructures/any.hpp
    hpx/datastructures/detail/dynamic_bitset.hpp
    hpx/datastructures/detail/flat_map.hpp
    hpx/datastructures/detail/flat_set.hpp
    hpx/datastructures/detail/intrusive_list.hpp
    hpx/datastructures/detail/small_vector.hpp
    hpx/datastructures/detail/optional.hpp
    hpx/datastructures/detail/variant.hpp
    hpx/datastructures/member_pack.hpp
    hpx/datastructures/optional.hpp
    hpx/datastructures/tuple.hpp
    hpx/datastructures/traits/supports_streaming_with_any.hpp
    hpx/datastructures/traits/is_tuple_like.hpp
    hpx/datastructures/variant.hpp
    hpx/datastructures/variant_helper.hpp
    hpx/datastructures/serialization/dynamic_bitset.hpp
    hpx/datastructures/serialization/optional.hpp
    hpx/datastructures/serialization/serializable_any.hpp
    hpx/datastructures/serialization/tuple.hpp
)

# cmake-format: off
set(datastructures_compat_headers
    hpx/datastructures.hpp => hpx/modules/datastructures.hpp
    hpx/runtime/serialization/dynamic_bitset.hpp => hpx/datastructures/serialization/dynamic_bitset.hpp
    hpx/runtime/serialization/optional.hpp => hpx/datastructures/serialization/optional.hpp
    hpx/runtime/serialization/tuple.hpp => hpx/datastructures/serialization/tuple.hpp
    hpx/serialization/dynamic_bitset.hpp => hpx/datastructures/serialization/dynamic_bitset.hpp
    hpx/serialization/optional.hpp => hpx/datastructures/serialization/optional.hpp
    hpx/serialization/serializable_any.hpp => hpx/datastructures/serialization/serializable_any.hpp
    hpx/serialization/tuple.hpp => hpx/datastructures/serialization/tuple.hpp
    hpx/traits/supports_streaming_with_any.hpp => hpx/modules/datastructures.hpp
    hpx/traits/is_tuple_like.hpp => hpx/modules/datastructures.hpp
    hpx/util/any.hpp => hpx/any.hpp
    hpx/util/optional.hpp => hpx/optional.hpp
    hpx/util/tuple.hpp => hpx/tuple.hpp
    hpx/util/serializable_any.hpp => hpx/datastructures/serialization/serializable_any.hpp
)
# cmake-format: on

set(datastructures_sources serializable_any.cpp)

include(HPX_AddModule)
add_hpx_module(
  core datastructures
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${datastructures_sources}
  HEADERS ${datastructures_headers}
  COMPAT_HEADERS ${datastructures_compat_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/datastructures/variant_helper.hpp"
  ADD_TO_GLOBAL_HEADER
    "hpx/datastructures/detail/dynamic_bitset.hpp"
    "hpx/datastructures/detail/flat_map.hpp"
    "hpx/datastructures/detail/flat_set.hpp"
    "hpx/datastructures/detail/intrusive_list.hpp"
    "hpx/datastructures/detail/small_vector.hpp"
  MODULE_DEPENDENCIES
    hpx_assertion
    hpx_config
    hpx_concepts
    hpx_contracts
    hpx_errors
    hpx_serialization
    hpx_type_support
  CMAKE_SUBDIRS examples tests
)
