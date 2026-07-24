# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Compatibility with various Boost types, introduced in V1.4.0
hpx_option(
  HPX_SERIALIZATION_WITH_BOOST_TYPES BOOL
  "Enable serialization of certain Boost types. (default: OFF)" OFF ADVANCED
  CATEGORY "Modules"
  MODULE SERIALIZATION
)

if(HPX_SERIALIZATION_WITH_BOOST_TYPES)
  hpx_add_config_define_namespace(
    DEFINE HPX_SERIALIZATION_HAVE_BOOST_TYPES NAMESPACE SERIALIZATION
  )
endif()

# Support endian conversion on inout and output archives
hpx_option(
  HPX_SERIALIZATION_WITH_SUPPORTS_ENDIANESS BOOL
  "Support endian conversion on inout and output archives. (default: OFF)" OFF
  ADVANCED
  CATEGORY "Modules"
  MODULE SERIALIZATION
)

if(HPX_SERIALIZATION_WITH_SUPPORTS_ENDIANESS)
  hpx_add_config_define_namespace(
    DEFINE HPX_SERIALIZATION_HAVE_SUPPORTS_ENDIANESS NAMESPACE SERIALIZATION
  )
endif()

# cmake-format: off
#
# Important note: The following flags are specific for using HPX as a
# backend for SHAD (https://github.com/pnnl/SHAD):
#
# - HPX_SERIALIZATION_WITH_ALL_TYPES_ARE_BITWISE_SERIALIZABLE
# - HPX_SERIALIZATION_WITH_ALLOW_RAW_POINTER_SERIALIZATION
# - HPX_SERIALIZATION_WITH_ALLOW_CONST_TUPLE_MEMBERS
#
# They all enable questionable functionalities, partially they even enable
# undefined behavior. Please only use any of them if you know what you're
# doing.
#
# cmake-format: on

# This flag can be used in systems that assume that all types are bitwise
# serializable by default (like SHAD).
hpx_option(
  HPX_SERIALIZATION_WITH_ALL_TYPES_ARE_BITWISE_SERIALIZABLE BOOL
  "Assume all types are bitwise serializable. (default: OFF)" OFF ADVANCED
  CATEGORY "Modules"
  MODULE SERIALIZATION
)

if(HPX_SERIALIZATION_WITH_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
  hpx_add_config_define_namespace(
    DEFINE HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE
    NAMESPACE SERIALIZATION
  )
endif()

# This flag can be used in systems that rely on sending raw pointers to other
# localities (assuming those are being sent back to the originating locality
# before they are dereferenced (like SHAD).
hpx_option(
  HPX_SERIALIZATION_WITH_ALLOW_RAW_POINTER_SERIALIZATION BOOL
  "Enable serializing raw pointers. (default: OFF)" OFF ADVANCED
  CATEGORY "Modules"
  MODULE SERIALIZATION
)

if(HPX_SERIALIZATION_WITH_ALLOW_RAW_POINTER_SERIALIZATION)
  hpx_add_config_define_namespace(
    DEFINE HPX_SERIALIZATION_HAVE_ALLOW_RAW_POINTER_SERIALIZATION
    NAMESPACE SERIALIZATION
  )
endif()

# This flag can be used in systems that rely on sending constant values as part
# of std::tuple. This option essentially casts away constness for tuple members
# during de-serialization.
hpx_option(
  HPX_SERIALIZATION_WITH_ALLOW_CONST_TUPLE_MEMBERS BOOL
  "Enable serializing std::tuple with const members. (default: OFF)" OFF
  ADVANCED
  CATEGORY "Modules"
  MODULE SERIALIZATION
)

if(HPX_SERIALIZATION_WITH_ALLOW_CONST_TUPLE_MEMBERS)
  hpx_add_config_define_namespace(
    DEFINE HPX_SERIALIZATION_HAVE_ALLOW_CONST_TUPLE_MEMBERS
    NAMESPACE SERIALIZATION
  )
endif()

# Default location is $HPX_ROOT/libs/serialization/include
set(serialization_headers
    hpx/serialization.hpp
    hpx/serialization/detail/allow_zero_copy_receive.hpp
    hpx/serialization/detail/constructor_selector.hpp
    hpx/serialization/detail/non_default_constructible.hpp
    hpx/serialization/detail/pointer.hpp
    hpx/serialization/detail/polymorphic_id_factory.hpp
    hpx/serialization/detail/polymorphic_intrusive_factory.hpp
    hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp
    hpx/serialization/detail/polymorphic_nonintrusive_factory_impl.hpp
    hpx/serialization/detail/preprocess_container.hpp
    hpx/serialization/detail/raw_ptr.hpp
    hpx/serialization/detail/serialize_collection.hpp
    hpx/serialization/detail/vc.hpp
    hpx/serialization/detail/refl_qualified_name_of.hpp
    hpx/serialization/detail/refl_serialize_impl.hpp
    hpx/serialization/array.hpp
    hpx/serialization/bitset.hpp
    hpx/serialization/complex.hpp
    hpx/serialization/datapar.hpp
    hpx/serialization/deque.hpp
    hpx/serialization/error_code.hpp
    hpx/serialization/exception_ptr.hpp
    hpx/serialization/list.hpp
    hpx/serialization/macros.hpp
    hpx/serialization/map.hpp
    hpx/serialization/set.hpp
    hpx/serialization/serialize_buffer_fwd.hpp
    hpx/serialization/serialize_buffer.hpp
    hpx/serialization/string.hpp
    hpx/serialization/std_tuple.hpp
    hpx/serialization/unordered_set.hpp
    hpx/serialization/unordered_map.hpp
    hpx/serialization/vector.hpp
    hpx/serialization/variant.hpp
    hpx/serialization/valarray.hpp
    hpx/serialization/shared_ptr.hpp
    hpx/serialization/unique_ptr.hpp
    hpx/serialization/access.hpp
    hpx/serialization/base_object.hpp
    hpx/serialization/basic_archive.hpp
    hpx/serialization/binary_filter.hpp
    hpx/serialization/brace_initializable.hpp
    hpx/serialization/brace_initializable_fwd.hpp
    hpx/serialization/container.hpp
    hpx/serialization/input_archive.hpp
    hpx/serialization/input_container.hpp
    hpx/serialization/output_archive.hpp
    hpx/serialization/output_container.hpp
    hpx/serialization/serialization_chunk.hpp
    hpx/serialization/serialization_fwd.hpp
    hpx/serialization/serialize.hpp
    hpx/serialization/traits/brace_initializable_traits.hpp
    hpx/serialization/traits/is_bitwise_serializable.hpp
    hpx/serialization/traits/is_not_bitwise_serializable.hpp
    hpx/serialization/traits/is_serializable.hpp
    hpx/serialization/traits/is_serialization_supported.hpp
    hpx/serialization/traits/needs_automatic_registration.hpp
    hpx/serialization/traits/polymorphic_traits.hpp
    hpx/serialization/traits/serialization_access_data.hpp
)
set(serialization_macro_headers hpx/serialization/macros.hpp)

# Default location is $HPX_ROOT/libs/serialization/include_compatibility
# cmake-format: off
set(serialization_compat_headers
    hpx/runtime/serialization/array.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/bitset.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/complex.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/container.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/datapar.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/deque.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/list.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/map.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/multi_array.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/serialize_buffer.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/set.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/string.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/vector.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/variant.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/valarray.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/shared_ptr.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/unique_ptr.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/unordered_map.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/access.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/base_object.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/basic_archive.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/binary_filter.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/brace_initializable.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/brace_initializable_fwd.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/input_archive.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/input_container.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/output_archive.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/output_container.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/serialization_chunk.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/serialization_fwd.hpp => hpx/serialization.hpp
    hpx/runtime/serialization/serialize.hpp => hpx/serialization.hpp
    hpx/traits/brace_initializable_traits.hpp => hpx/serialization.hpp
    hpx/traits/is_bitwise_serializable.hpp => hpx/serialization.hpp
    hpx/serialization/traits/is_serialization_supported.hpp => hpx/serialization.hpp
    hpx/traits/needs_automatic_registration.hpp => hpx/serialization.hpp
    hpx/traits/polymorphic_traits.hpp => hpx/serialization.hpp
    hpx/traits/serialization_access_data.hpp => hpx/serialization.hpp
)
# cmake-format: on

if(HPX_SERIALIZATION_WITH_BOOST_TYPES)
  set(boost_serialization_headers
      hpx/serialization/boost_array.hpp
      hpx/serialization/boost_intrusive_ptr.hpp
      hpx/serialization/boost_multi_array.hpp
      hpx/serialization/boost_shared_ptr.hpp
      hpx/serialization/boost_variant.hpp
  )

  set(serialization_headers ${serialization_headers}
                            ${boost_serialization_headers}
  )

  # cmake-format: off
  set(serialization_compat_headers ${serialization_compat_headers}
    hpx/serialization/multi_array.hpp => hpx/serialization/boost_multi_array.hpp
    hpx/serialization/intrusive_ptr.hpp => hpx/serialization/boost_intrusive_ptr.hpp
  )
  # cmake-format: on
endif()

# Default location is $HPX_ROOT/libs/serialization/src
set(serialization_sources
    detail/allow_zero_copy_receive.cpp detail/pointer.cpp
    detail/polymorphic_id_factory.cpp detail/polymorphic_intrusive_factory.cpp
    detail/polymorphic_nonintrusive_factory.cpp exception_ptr.cpp
)

if(TARGET Vc::vc)
  set(serialization_optional_dependencies Vc::vc)
endif()

include(HPX_AddModule)
add_hpx_module(
  core serialization
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${serialization_sources}
  HEADERS ${serialization_headers}
  MACRO_HEADERS ${serialization_macro_headers}
  COMPAT_HEADERS ${serialization_compat_headers}
  MODULE_DEPENDENCIES
    hpx_assertion
    hpx_concepts
    hpx_config
    hpx_debugging
    hpx_errors
    hpx_format
    hpx_preprocessor
    hpx_type_support
  DEPENDENCIES ${serialization_optional_dependencies}
  ADD_TO_GLOBAL_HEADER
    hpx/serialization/detail/allow_zero_copy_receive.hpp
    hpx/serialization/detail/polymorphic_id_factory.hpp
    hpx/serialization/detail/polymorphic_intrusive_factory.hpp
    hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp
    hpx/serialization/detail/preprocess_container.hpp
  CMAKE_SUBDIRS examples tests
)
