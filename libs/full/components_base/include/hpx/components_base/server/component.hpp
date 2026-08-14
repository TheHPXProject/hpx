//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2015-2017 Thomas Heller
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components_base/components_base_fwd.hpp>
#include <hpx/components_base/server/component_base.hpp>
#include <hpx/components_base/traits/component_heap_type.hpp>
#include <hpx/modules/allocator_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::components::detail {

    ///////////////////////////////////////////////////////////////////////
    template <typename Component>
    struct simple_heap
    {
        static void* alloc(std::size_t count)
        {
            HPX_ASSERT(1 == count);
            return alloc_.allocate(count);
        }
        static void free(void* p, std::size_t count) noexcept
        {
            HPX_ASSERT(1 == count);
            alloc_.deallocate(static_cast<Component*>(p), count);
        }

        static util::internal_allocator<Component> alloc_;
    };

    template <typename Component>
    util::internal_allocator<Component> simple_heap<Component>::alloc_;
}    // namespace hpx::components::detail

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component, typename Enable>
    struct component_heap_type
    {
        using type = hpx::components::detail::simple_heap<Component>;
    };
}    // namespace hpx::traits

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    class component : public Component
    {
    public:
        using type_holder = Component;
        using component_type = component<Component>;
        using derived_type = component_type;
        using heap_type = typename traits::component_heap_type<Component>::type;

        constexpr component() = default;

        // Construct a component instance holding a new wrapped instance
        template <typename T, typename... Ts,
            typename Enable =
                std::enable_if_t<!std::is_same_v<std::decay_t<T>, component>>>
        explicit component(T&& t, Ts&&... ts)
          : Component(HPX_FORWARD(T, t), HPX_FORWARD(Ts, ts)...)
        {
        }
    };
}    // namespace hpx::components
