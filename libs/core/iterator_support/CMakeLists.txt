# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(iterator_support_headers
    hpx/iterator_support/detail/minimum_category.hpp
    hpx/iterator_support/traits/is_foldable.hpp
    hpx/iterator_support/traits/is_iterator.hpp
    hpx/iterator_support/traits/is_range.hpp
    hpx/iterator_support/traits/is_segmented_iterator.hpp
    hpx/iterator_support/boost_iterator_categories.hpp
    hpx/iterator_support/counting_iterator.hpp
    hpx/iterator_support/counting_shape.hpp
    hpx/iterator_support/generator_iterator.hpp
    hpx/iterator_support/iterator_adaptor.hpp
    hpx/iterator_support/iterator_facade.hpp
    hpx/iterator_support/iterator_range.hpp
    hpx/iterator_support/range.hpp
    hpx/iterator_support/transform_iterator.hpp
    hpx/iterator_support/unwrap_iterator.hpp
    hpx/iterator_support/zip_iterator.hpp
)

# cmake-format: off
set(iterator_support_compat_headers
    hpx/iterator_support.hpp => hpx/modules/iterator_support.hpp
    hpx/traits/is_iterator.hpp => hpx/modules/iterator_support.hpp
    hpx/traits/is_range.hpp => hpx/modules/iterator_support.hpp
    hpx/util/iterator_adaptor.hpp => hpx/modules/iterator_support.hpp
    hpx/util/iterator_facade.hpp => hpx/modules/iterator_support.hpp
    hpx/util/iterator_range.hpp => hpx/modules/iterator_support.hpp
    hpx/util/range.hpp => hpx/modules/iterator_support.hpp
    hpx/util/transform_iterator.hpp => hpx/modules/iterator_support.hpp
)
# cmake-format: on

set(iterator_support_sources)

include(HPX_AddModule)

add_hpx_module(
  core iterator_support
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${iterator_support_sources}
  HEADERS ${iterator_support_headers}
  COMPAT_HEADERS ${iterator_support_compat_headers}
  MODULE_DEPENDENCIES hpx_concepts hpx_config hpx_datastructures hpx_functional
                      hpx_serialization hpx_type_support
  CMAKE_SUBDIRS examples tests
)
