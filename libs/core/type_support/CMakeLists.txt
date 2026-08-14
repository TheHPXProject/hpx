# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(type_support_headers
    hpx/type_support/detail/with_result_of.hpp
    hpx/type_support/detail/wrap_int.hpp
    hpx/type_support/aligned_storage.hpp
    hpx/type_support/assert_owns_lock.hpp
    hpx/type_support/base_from_member.hpp
    hpx/type_support/bit_cast.hpp
    hpx/type_support/construct_at.hpp
    hpx/type_support/decay.hpp
    hpx/type_support/default_sentinel.hpp
    hpx/type_support/detected.hpp
    hpx/type_support/empty_function.hpp
    hpx/type_support/equality.hpp
    hpx/type_support/extra_data.hpp
    hpx/type_support/identity.hpp
    hpx/type_support/is_relocatable.hpp
    hpx/type_support/is_replaceable.hpp
    hpx/type_support/is_trivially_relocatable.hpp
    hpx/type_support/is_contiguous_iterator.hpp
    hpx/type_support/lazy_conditional.hpp
    hpx/type_support/lazy_enable_if.hpp
    hpx/type_support/pack.hpp
    hpx/type_support/macros.hpp
    hpx/type_support/meta.hpp
    hpx/type_support/relocate_at.hpp
    hpx/type_support/select.hpp
    hpx/type_support/static.hpp
    hpx/type_support/uninitialized_relocation_primitives.hpp
    hpx/type_support/unwrap_ref.hpp
    hpx/type_support/unused.hpp
    hpx/type_support/void_guard.hpp
)
set(type_support_macro_headers hpx/type_support/macros.hpp)

if(HPX_WITH_CXX20_COROUTINES)
  set(type_support_headers
      ${type_support_headers} hpx/type_support/coroutines_support.hpp
      hpx/type_support/generator.hpp
  )
endif()

# cmake-format: off
set(type_support_compat_headers
    hpx/type_support.hpp => hpx/modules/type_support.hpp
    hpx/util/always_void.hpp => hpx/modules/type_support.hpp
    hpx/util/assert_owns_lock.hpp => hpx/modules/type_support.hpp
    hpx/util/decay.hpp => hpx/modules/type_support.hpp
    hpx/util/detected.hpp => hpx/modules/type_support.hpp
    hpx/util/identity.hpp => hpx/modules/type_support.hpp
    hpx/util/lazy_conditional.hpp => hpx/modules/type_support.hpp
    hpx/util/lazy_enable_if.hpp => hpx/modules/type_support.hpp
    hpx/util/static.hpp => hpx/modules/type_support.hpp
    hpx/util/unwrap_ref.hpp => hpx/modules/type_support.hpp
    hpx/util/unused.hpp => hpx/modules/type_support.hpp
    hpx/util/void_guard.hpp => hpx/modules/type_support.hpp
)
# cmake-format: on

set(type_support_sources)

include(HPX_AddModule)
add_hpx_module(
  core type_support
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${type_support_sources}
  HEADERS ${type_support_headers}
  MACRO_HEADERS ${type_support_macro_headers}
  COMPAT_HEADERS ${type_support_compat_headers}
  ADD_TO_GLOBAL_HEADER hpx/type_support/detail/with_result_of.hpp
                       hpx/type_support/detail/wrap_int.hpp
  MODULE_DEPENDENCIES hpx_assertion hpx_concepts hpx_config
  CMAKE_SUBDIRS examples tests
)
