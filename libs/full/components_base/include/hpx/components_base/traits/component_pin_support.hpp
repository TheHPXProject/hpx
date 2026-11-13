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

        struct defer_destroy_helper
        {
            template <typename Component, typename F>
            static constexpr bool call(wrap_int, Component*, F&&) noexcept
            {
                return false;
            }

            // forward the call if the component implements the function
            template <typename Component, typename F>
            static auto call(int, Component* p, F&& f)
                -> decltype(p->defer_destroy(HPX_FORWARD(F, f)))
            {
                return p->defer_destroy(HPX_FORWARD(F, f));
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

        // Called when the component is about to be destroyed (its global id
        // has been released). Returns true if the component took over the
        // given function, which destroys it, to call it once it is no longer
        // pinned. Returns false if the component can be destroyed right away.
        template <typename F>
        static bool defer_destroy(Component* p, F&& f)
        {
            return detail::defer_destroy_helper::call(0, p, HPX_FORWARD(F, f));
        }
    };
}    // namespace hpx::traits
