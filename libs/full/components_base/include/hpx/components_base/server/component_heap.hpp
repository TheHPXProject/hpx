//  Copyright (c) 2011-2017 Thomas Heller
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/macros.hpp>
#include <hpx/modules/static_reinit.hpp>

namespace hpx::components {

    // This is a utility to ensure that there exists exactly one heap per
    // component.
    //
    // This is the customization point and will be defined by the registration
    // macros
    namespace detail {

        HPX_CXX_EXPORT template <typename Component>
        struct HPX_ALWAYS_EXPORT component_heap_impl;

        HPX_CXX_EXPORT template <typename Component>
        HPX_ALWAYS_EXPORT Component::heap_type& component_heap_helper();
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Component>
    Component::heap_type& component_heap()
    {
        if constexpr (requires {
                          typename detail::component_heap_impl<
                              Component>::valid;
                      })
        {
            return detail::component_heap_impl<Component>::call();
        }
        else
        {
            return detail::component_heap_helper<Component>();
        }
    }
}    // namespace hpx::components
