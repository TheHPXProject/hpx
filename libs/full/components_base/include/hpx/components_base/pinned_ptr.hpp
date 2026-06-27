//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components_base/get_lva.hpp>
#include <hpx/components_base/traits/action_decorate_function.hpp>
#include <hpx/components_base/traits/component_pin_support.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>

namespace hpx::components {

    HPX_CXX_EXPORT class pinned_ptr
    {
        template <typename T>
        struct id
        {
        };

        using unpin_type = void (*)(naming::address_type);

        template <typename Component>
        static void unpin_impl(naming::address_type lva)
        {
            if (lva != nullptr)
            {
                traits::component_pin_support<Component>::unpin(
                    get_lva<Component>::call(lva));
            }
        }

        template <typename Component>
        pinned_ptr(naming::address_type lva, id<Component>)
          : lva_(lva)
          , unpin_(&unpin_impl<Component>)
        {
            HPX_ASSERT(nullptr != this->lva_);

            // pin associated component instance
            if (!traits::component_pin_support<Component>::pin(
                    get_lva<Component>::call(this->lva_)))
            {
                this->lva_ = nullptr;
                this->unpin_ = nullptr;
            }
        }

    public:
        constexpr pinned_ptr() noexcept = default;

        ~pinned_ptr()
        {
            if (unpin_ != nullptr && lva_ != nullptr)
            {
                unpin_(lva_);
            }
        }

        pinned_ptr(pinned_ptr const&) = delete;
        pinned_ptr(pinned_ptr&& rhs) noexcept
          : lva_(rhs.lva_)
          , unpin_(rhs.unpin_)
        {
            rhs.lva_ = nullptr;
            rhs.unpin_ = nullptr;
        }

        pinned_ptr& operator=(pinned_ptr const&) = delete;
        pinned_ptr& operator=(pinned_ptr&& rhs) noexcept
        {
            if (this != &rhs)
            {
                if (unpin_ != nullptr && lva_ != nullptr)
                {
                    unpin_(lva_);
                }
                lva_ = rhs.lva_;
                unpin_ = rhs.unpin_;
                rhs.lva_ = nullptr;
                rhs.unpin_ = nullptr;
            }
            return *this;
        }

        explicit constexpr operator bool() const noexcept
        {
            return lva_ != nullptr;
        }

        template <typename Component>
        static pinned_ptr create([[maybe_unused]] naming::address_type lva)
        {
            using component_type = std::remove_cv_t<Component>;
            if constexpr (traits::component_decorates_action_v<component_type>)
            {
                // created pinned_ptr actually pins object it refers to
                return pinned_ptr(lva, id<component_type>{});
            }
            else
            {
                // created pinned_ptr does not pin object it refers to
                return pinned_ptr{};
            }
        }

    private:
        naming::address_type lva_ = nullptr;
        unpin_type unpin_ = nullptr;
    };
}    // namespace hpx::components
