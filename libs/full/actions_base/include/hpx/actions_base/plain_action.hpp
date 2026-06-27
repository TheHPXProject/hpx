//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file plain_action.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/basic_action.hpp>
#include <hpx/actions_base/macros.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#if defined(__NVCC__) || defined(__CUDACC__)
#include <type_traits>
#endif
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::actions {

    /// \cond NOINTERNAL
    namespace detail {

        struct plain_function
        {
            // Only localities are valid targets for a plain action
            static bool is_target_valid(hpx::id_type const& id) noexcept
            {
                return naming::is_locality(id);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        inline std::string make_plain_action_name(std::string_view action_name)
        {
            return hpx::util::format("plain action({})", action_name);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic plain (free) action types allowing to hold a
    //  different number of arguments
    ///////////////////////////////////////////////////////////////////////////
    template <typename R, typename... Ps, R (*F)(Ps...), typename Derived>
    struct action<R (*)(Ps...), F, Derived>
      : basic_action<detail::plain_function, R(Ps...),
            detail::action_type_t<action<R (*)(Ps...), F, Derived>, Derived>>
    {
        using derived_type = detail::action_type_t<action, Derived>;

        static std::string get_action_name(
            naming::address::address_type /*lva*/)
        {
            return detail::make_plain_action_name(
                detail::get_action_name<derived_type>());
        }

        template <typename... Ts>
        static R invoke(naming::address::address_type /*lva*/,
            naming::address::component_type /*comptype*/, Ts&&... vs)
        {
            basic_action<detail::plain_function, R(Ps...),
                derived_type>::increment_invocation_count();

            return F(HPX_FORWARD(Ts, vs)...);
        }
    };

    template <typename R, typename... Ps, R (*F)(Ps...) noexcept,
        typename Derived>
    struct action<R (*)(Ps...) noexcept, F, Derived>
      : basic_action<detail::plain_function, R(Ps...),
            detail::action_type_t<action<R (*)(Ps...) noexcept, F, Derived>,
                Derived>>
    {
        using derived_type = detail::action_type_t<action, Derived>;

        static std::string get_action_name(
            naming::address::address_type /*lva*/)
        {
            return detail::make_plain_action_name(
                detail::get_action_name<derived_type>());
        }

        template <typename... Ts>
        static R invoke(naming::address::address_type /*lva*/,
            naming::address::component_type /* comptype */, Ts&&... vs)
        {
            basic_action<detail::plain_function, R(Ps...),
                derived_type>::increment_invocation_count();

            return F(HPX_FORWARD(Ts, vs)...);
        }
    };
    /// \endcond
}    // namespace hpx::actions

namespace hpx::traits {

    /// \cond NOINTERNAL
    template <>
    HPX_ALWAYS_EXPORT inline components::component_type component_type_database<
        hpx::actions::detail::plain_function>::get() noexcept
    {
        return to_int(hpx::components::component_enum_type::plain_function);
    }

    // clang-format off
    template <>
    HPX_ALWAYS_EXPORT inline void
        component_type_database<hpx::actions::detail::plain_function>::set(
            components::component_type)
    {
        HPX_ASSERT(false);    // shouldn't be ever called
    }
    // clang-format on
    /// \endcond
}    // namespace hpx::traits

#include <hpx/config/warnings_suffix.hpp>
