//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file component_action.hpp
/// \page HPX_DEFINE_COMPONENT_ACTION
/// \headerfile hpx/components.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/basic_action.hpp>
#include <hpx/actions_base/macros.hpp>
#include <hpx/actions_base/traits/is_client.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::actions {

    /// \cond NOINTERNAL
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        inline std::string make_component_action_name(
            std::string_view action_name, void const* lva)
        {
            return hpx::util::format(
                "component action({}) lva({})", action_name, lva);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Component, typename R, typename F, typename... Ts>
        R component_invoke(naming::address_type lva,
            naming::component_type /*comptype*/, F Component::* f, Ts&&... vs)
        {
            Component* component = get_lva<Component>::call(lva);
            if constexpr (traits::is_future_v<R> || traits::is_client_v<R>)
            {
                // additional pinning is required such that the object becomes
                // unpinned only after the returned future has become ready
                components::pinned_ptr p =
                    components::pinned_ptr::create<Component>(lva);

                R result = (component->*f)(HPX_FORWARD(Ts, vs)...);

                if (!result.is_ready())
                {
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [p = HPX_MOVE(p)]() {});
                }
                return result;
            }
            else
            {
                return (component->*f)(HPX_FORWARD(Ts, vs)...);
            }
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic non-const component action types allowing to hold an
    //  arbitrary number of arguments
    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...), typename Derived>
    struct action<R (Component::*)(Ps...), F, Derived>
      : basic_action<Component, R(Ps...),
            detail::action_type_t<action<R (Component::*)(Ps...), F, Derived>,
                Derived>>
    {
        using derived_type = detail::action_type_t<action, Derived>;

        static std::string get_action_name(naming::address_type lva)
        {
            return detail::make_component_action_name(
                detail::get_action_name<derived_type>(),
                get_lva<Component>::call(lva));
        }

        template <typename... Ts>
        static R invoke(naming::address_type lva,
            naming::component_type comptype, Ts&&... vs)
        {
            basic_action<Component, R(Ps...),
                derived_type>::increment_invocation_count();

            return detail::component_invoke<Component, R>(
                lva, comptype, F, HPX_FORWARD(Ts, vs)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic const component action types allowing to hold an
    //  arbitrary number of arguments
    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...) const, typename Derived>
    struct action<R (Component::*)(Ps...) const, F, Derived>
      : basic_action<Component, R(Ps...),
            detail::action_type_t<
                action<R (Component::*)(Ps...) const, F, Derived>, Derived>>
    {
        using derived_type = detail::action_type_t<action, Derived>;

        static std::string get_action_name(naming::address_type lva)
        {
            return detail::make_component_action_name(
                detail::get_action_name<derived_type>(),
                get_lva<Component>::call(lva));
        }

        template <typename... Ts>
        static R invoke(naming::address_type lva,
            naming::component_type comptype, Ts&&... vs)
        {
            basic_action<Component, R(Ps...),
                derived_type>::increment_invocation_count();

            return detail::component_invoke<Component, R>(
                lva, comptype, F, HPX_FORWARD(Ts, vs)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic non-const noexcept component action types allowing
    //  to hold an arbitrary number of arguments
    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...) noexcept, typename Derived>
    struct action<R (Component::*)(Ps...) noexcept, F, Derived>
      : basic_action<Component, R(Ps...),
            detail::action_type_t<
                action<R (Component::*)(Ps...) noexcept, F, Derived>, Derived>>
    {
        using derived_type = detail::action_type_t<action, Derived>;

        static std::string get_action_name(naming::address_type lva)
        {
            return detail::make_component_action_name(
                detail::get_action_name<derived_type>(),
                get_lva<Component>::call(lva));
        }

        template <typename... Ts>
        static R invoke(naming::address_type lva,
            naming::component_type comptype, Ts&&... vs)
        {
            basic_action<Component, R(Ps...),
                derived_type>::increment_invocation_count();

            return detail::component_invoke<Component, R>(
                lva, comptype, F, HPX_FORWARD(Ts, vs)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic const noexcept component action types allowing to
    //  hold an arbitrary number of arguments
    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...) const noexcept, typename Derived>
    struct action<R (Component::*)(Ps...) const noexcept, F, Derived>
      : basic_action<Component, R(Ps...),
            detail::action_type_t<
                action<R (Component::*)(Ps...) const noexcept, F, Derived>,
                Derived>>
    {
        using derived_type = detail::action_type_t<action, Derived>;

        static std::string get_action_name(naming::address_type lva)
        {
            return detail::make_component_action_name(
                detail::get_action_name<derived_type>(),
                get_lva<Component>::call(lva));
        }

        template <typename... Ts>
        static R invoke(naming::address_type lva,
            naming::component_type comptype, Ts&&... vs)
        {
            basic_action<Component, R(Ps...),
                derived_type>::increment_invocation_count();

            return detail::component_invoke<Component, R>(
                lva, comptype, F, HPX_FORWARD(Ts, vs)...);
        }
    };
    /// \endcond
}    // namespace hpx::actions

#include <hpx/config/warnings_suffix.hpp>
