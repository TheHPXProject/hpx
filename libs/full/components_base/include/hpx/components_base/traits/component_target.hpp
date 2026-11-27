//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <type_traits>
#include <utility>

namespace hpx::traits {

    namespace detail {

        template <typename Component, typename Enable = void>
        struct has_get_target : std::false_type
        {
        };

        template <typename Component>
        struct has_get_target<Component,
            std::void_t<
                decltype(std::declval<Component const&>().get_target())>>
          : std::true_type
        {
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Customization point: the target (for instance an
    // hpx::cuda::experimental::target) a component's device data lives on.
    // Device data in the arguments of parcels sent to the component (for
    // instance a thrust::device_vector) is received on the GPU of that target
    // (see hpx::traits::target_device). By default, a component's
    // get_target() member function is used if it has one (it must not throw);
    // specializations need to provide has_target and call(), which returns
    // the target.
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_target
    {
        // false if the component never has a target, the receiving side then
        // doesn't look at the component at all
        static constexpr bool has_target =
            detail::has_get_target<Component>::value;

        // only used if has_target
        static decltype(auto) call(Component const& component) noexcept
        {
            return component.get_target();
        }
    };

    namespace detail {

        // The GPU device data sent to a component is received on, -1 if
        // none: the GPU of its target. Components that know the GPU only
        // (LCOs, see hpx::lcos::base_lco) specialize this instead.
        HPX_CXX_EXPORT template <typename Component, typename Enable = void>
        struct component_device
        {
            using target_trait = component_target<Component>;

            static constexpr bool has_device = target_trait::has_target;

            static int call(
                [[maybe_unused]] Component const& component) noexcept
            {
                if constexpr (has_device)
                {
                    decltype(auto) target = target_trait::call(component);
                    return target_device<std::decay_t<decltype(target)>>::call(
                        target);
                }
                else
                {
                    return -1;
                }
            }
        };
    }    // namespace detail
}    // namespace hpx::traits
