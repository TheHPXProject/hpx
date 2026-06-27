//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/traits/is_component.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// The \a get_lva template is a helper structure allowing to convert a
    /// local virtual address as stored in a local address (returned from
    /// the function \a agas::addressing_service#resolve) to the address of the
    /// component implementing the action.
    ///
    /// The default implementation uses the template argument \a Component
    /// to deduce the type wrapping the component implementing the action. This
    /// is used to get the needed address.
    ///
    /// \tparam Component  This is the type of the component implementing the
    ///                    action to execute.
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct get_lva
    {
        constexpr static Component* call(naming::address_type lva) noexcept
        {
            if constexpr (traits::is_managed_component_v<Component>)
            {
                if constexpr (std::is_const_v<Component>)
                {
                    using wrapping_type =
                        std::add_const_t<typename Component::wrapping_type>;
                    return static_cast<wrapping_type*>(lva)->get();
                }
                else
                {
                    using wrapping_type = typename Component::wrapping_type;
                    return static_cast<wrapping_type*>(lva)->get();
                }
            }
            else
            {
                return static_cast<Component*>(lva);
            }
        }
    };
}    // namespace hpx
