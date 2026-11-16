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

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
#include <meta>
#endif

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

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        // A member with an explicit object parameter can have the same type as
        // a member function, but no member function pointer can be extracted
        // from it.
        consteval bool has_explicit_object_parameter(std::meta::info const m)
        {
            auto const params = std::meta::parameters_of(m);
            return !params.empty() &&
                std::meta::is_explicit_object_parameter(params[0]);
        }

        // reflect_function does not take a member function pointer, so the
        // member F points to is looked up in Component. Comparing reflected
        // values instead of the pointers keeps this a constant expression for
        // virtual members as well.
        template <typename Component, typename Fn, auto F>
        consteval std::meta::info member_function_of()
        {
            for (std::meta::info const m : std::meta::members_of(
                     ^^Component, std::meta::access_context::unchecked()))
            {
                // Constructors and destructors have no type to compare, the
                // other checks keep extract from failing.
                if (std::meta::is_function(m) &&
                    !std::meta::is_static_member(m) &&
                    !std::meta::is_constructor(m) &&
                    !std::meta::is_destructor(m) && !std::meta::is_deleted(m) &&
                    std::meta::is_same_type(std::meta::type_of(m), ^^Fn) &&
                    !has_explicit_object_parameter(m) &&
                    std::meta::reflect_constant(std::meta::extract<decltype(F)>(
                        m)) == std::meta::reflect_constant(F))
                {
                    return m;
                }
            }
            return {};
        }

        // Specializations of member function templates are not members of
        // Component, so they are not found and stay unknown.
        template <typename Component, typename Fn, auto F>
        consteval threads::task_size member_function_task_size()
        {
            constexpr std::meta::info m =
                member_function_of<Component, Fn, F>();
            if constexpr (m == std::meta::info{})
            {
                return threads::task_size::unknown;
            }
            else
            {
                return hpx::traits::detail::annotated_function_task_size<m>();
            }
        }
#endif
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //  The specializations below pass Derived to basic_action as is, the
    //  specializations for Derived == void at the end of this file make the
    //  action its own derived type. The base classes don't name F: MSVC 19.52
    //  fails with an internal compiler error (C1001) if the base class of an
    //  action names the member function pointer of a component with more than
    //  one base class, when the action is instantiated in a translation unit
    //  importing the C++ module the component was exported from.

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic non-const component action types allowing to hold an
    //  arbitrary number of arguments
    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...), typename Derived>
    struct action<R (Component::*)(Ps...), F, Derived>
      : basic_action<Component, R(Ps...), Derived>
    {
        using derived_type = Derived;

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        // The member function can give the action a size with an annotation.
        // A sized function bound to the action hides this with its own size.
        static constexpr threads::task_size function_task_size() noexcept
        {
            return detail::member_function_task_size<Component, R(Ps...), F>();
        }
#endif

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
      : basic_action<Component, R(Ps...), Derived>
    {
        using derived_type = Derived;

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        // The member function can give the action a size with an annotation.
        // A sized function bound to the action hides this with its own size.
        static constexpr threads::task_size function_task_size() noexcept
        {
            return detail::member_function_task_size<Component, R(Ps...) const,
                F>();
        }
#endif

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
      : basic_action<Component, R(Ps...), Derived>
    {
        using derived_type = Derived;

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        // The member function can give the action a size with an annotation.
        // A sized function bound to the action hides this with its own size.
        static constexpr threads::task_size function_task_size() noexcept
        {
            return detail::member_function_task_size<Component,
                R(Ps...) noexcept, F>();
        }
#endif

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
      : basic_action<Component, R(Ps...), Derived>
    {
        using derived_type = Derived;

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        // The member function can give the action a size with an annotation.
        // A sized function bound to the action hides this with its own size.
        static constexpr threads::task_size function_task_size() noexcept
        {
            return detail::member_function_task_size<Component,
                R(Ps...) const noexcept, F>();
        }
#endif

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
    //  Component actions without a derived type are their own derived type
    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...)>
    struct action<R (Component::*)(Ps...), F, void>
      : action<R (Component::*)(Ps...), F,
            action<R (Component::*)(Ps...), F, void>>
    {
    };

    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...) const>
    struct action<R (Component::*)(Ps...) const, F, void>
      : action<R (Component::*)(Ps...) const, F,
            action<R (Component::*)(Ps...) const, F, void>>
    {
    };

    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...) noexcept>
    struct action<R (Component::*)(Ps...) noexcept, F, void>
      : action<R (Component::*)(Ps...) noexcept, F,
            action<R (Component::*)(Ps...) noexcept, F, void>>
    {
    };

    template <typename Component, typename R, typename... Ps,
        R (Component::*F)(Ps...) const noexcept>
    struct action<R (Component::*)(Ps...) const noexcept, F, void>
      : action<R (Component::*)(Ps...) const noexcept, F,
            action<R (Component::*)(Ps...) const noexcept, F, void>>
    {
    };
    /// \endcond
}    // namespace hpx::actions

#include <hpx/config/warnings_suffix.hpp>
