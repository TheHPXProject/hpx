# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Compatibility with Boost placeholder types, introduced in V2.0.0
hpx_option(
  HPX_FUNCTIONAL_WITH_BOOST_PLACEHOLDERS BOOL
  "Enable support for Boost placeholder types. (default: OFF)" OFF ADVANCED
  CATEGORY "Modules"
  MODULE FUNCTIONAL
)

if(HPX_FUNCTIONAL_WITH_BOOST_PLACEHOLDERS)
  hpx_add_config_define_namespace(
    DEFINE HPX_FUNCTIONAL_HAVE_BOOST_PLACEHOLDERS NAMESPACE FUNCTIONAL
  )
endif()

# Default location is $HPX_ROOT/libs/functional/include
set(functional_headers
    hpx/functional/deferred_call.hpp
    hpx/functional/detail/basic_function.hpp
    hpx/functional/detail/empty_function.hpp
    hpx/functional/detail/function_registration.hpp
    hpx/functional/detail/reset_function.hpp
    hpx/functional/detail/runtime_get.hpp
    hpx/functional/detail/vtable/callable_vtable.hpp
    hpx/functional/detail/vtable/copyable_vtable.hpp
    hpx/functional/detail/vtable/function_vtable.hpp
    hpx/functional/detail/vtable/vtable.hpp
    hpx/functional/bind.hpp
    hpx/functional/bind_back.hpp
    hpx/functional/bind_front.hpp
    hpx/functional/first_argument.hpp
    hpx/functional/function.hpp
    hpx/functional/function_ref.hpp
    hpx/functional/invoke_fused.hpp
    hpx/functional/macros.hpp
    hpx/functional/mem_fn.hpp
    hpx/functional/move_only_function.hpp
    hpx/functional/one_shot.hpp
    hpx/functional/protect.hpp
    hpx/functional/experimental/scope_exit.hpp
    hpx/functional/experimental/scope_fail.hpp
    hpx/functional/experimental/scope_success.hpp
    hpx/functional/reference_wrapper.hpp
    hpx/functional/serialization/detail/serializable_basic_function.hpp
    hpx/functional/serialization/detail/vtable/serializable_function_vtable.hpp
    hpx/functional/serialization/detail/vtable/serializable_vtable.hpp
    hpx/functional/serialization/reference_wrapper.hpp
    hpx/functional/serialization/serializable_function.hpp
    hpx/functional/serialization/serializable_move_only_function.hpp
    hpx/functional/traits/get_action_name.hpp
    hpx/functional/traits/get_function_address.hpp
    hpx/functional/traits/get_function_annotation.hpp
    hpx/functional/traits/is_action.hpp
    hpx/functional/traits/is_bind_expression.hpp
    hpx/functional/traits/is_placeholder.hpp
)
set(functional_macro_headers hpx/functional/macros.hpp)

# Default location is $HPX_ROOT/libs/functional/include_compatibility
# cmake-format: off
set(functional_compat_headers
    hpx/util/bind.hpp => hpx/functional.hpp
    hpx/util/bind_back.hpp => hpx/functional.hpp
    hpx/util/bind_front.hpp => hpx/functional.hpp
    hpx/util/deferred_call.hpp => hpx/functional.hpp
    hpx/util/first_argument.hpp => hpx/functional.hpp
    hpx/util/function.hpp => hpx/functional.hpp
    hpx/util/function_ref.hpp => hpx/functional.hpp
    hpx/util/invoke.hpp => hpx/functional.hpp
    hpx/util/invoke_fused.hpp => hpx/functional.hpp
    hpx/util/mem_fn.hpp => hpx/functional.hpp
    hpx/util/one_shot.hpp => hpx/functional.hpp
    hpx/util/protect.hpp => hpx/functional.hpp
    hpx/util/result_of.hpp => hpx/functional.hpp
    hpx/util/serializable_function.hpp => hpx/functional.hpp
    hpx/util/serializable_unique_function.hpp => hpx/functional.hpp
    hpx/util/unique_function.hpp => hpx/functional.hpp
)
# cmake-format: on

# Default location is $HPX_ROOT/libs/functional/src
set(functional_sources basic_function.cpp empty_function.cpp)

include(HPX_AddModule)
add_hpx_module(
  core functional
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${functional_sources}
  HEADERS ${functional_headers}
  MACRO_HEADERS ${functional_macro_headers}
  COMPAT_HEADERS ${functional_compat_headers}
  ADD_TO_GLOBAL_HEADER "hpx/functional/detail/reset_function.hpp"
                       "hpx/functional/detail/runtime_get.hpp"
  MODULE_DEPENDENCIES
    hpx_assertion
    hpx_config
    hpx_datastructures
    hpx_debugging
    hpx_errors
    hpx_tracing
    hpx_preprocessor
    hpx_serialization
    hpx_tag_invoke
    hpx_type_support
  CMAKE_SUBDIRS examples tests
)
