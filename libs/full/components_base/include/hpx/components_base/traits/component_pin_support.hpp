//  Copyright (c) 2018-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstdint>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for component pinning
    namespace detail {

        struct pin_helper
        {
            template <typename Component>
            static constexpr bool call(wrap_int, Component*) noexcept
            {
                return true;
            }

            // forward the call if the component implements the function
            template <typename Component>
            static constexpr auto call(int, Component* p) noexcept
                -> decltype(p->pin())
            {
                return p->pin();
            }
        };

        struct unpin_helper
        {
            template <typename Component>
            static constexpr bool call(wrap_int, Component*) noexcept
            {
                return false;
            }

            // forward the call if the component implements the function
            template <typename Component>
            static constexpr auto call(int, Component* p) noexcept
                -> decltype(p->unpin())
            {
                return p->unpin();
            }
        };

        struct pin_count_helper
        {
            template <typename Component>
            static constexpr std::uint32_t call(wrap_int, Component*) noexcept
            {
                return 0;
            }

            // forward the call if the component implements the function
            template <typename Component>
            static constexpr auto call(int, Component* p) noexcept
                -> decltype(p->pin_count())
            {
                return p->pin_count();
            }
        };
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_pin_support
    {
        static constexpr bool pin(Component* p) noexcept
        {
            return detail::pin_helper::call(0, p);
        }

        static constexpr bool unpin(Component* p) noexcept
        {
            return detail::unpin_helper::call(0, p);
        }

        static constexpr std::uint32_t pin_count(Component* p) noexcept
        {
            return detail::pin_count_helper::call(0, p);
        }
    };
}    // namespace hpx::traits
