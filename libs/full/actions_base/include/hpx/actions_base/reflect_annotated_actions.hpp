//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file reflect_annotated_actions.hpp
/// \brief Annotation-driven HPX action registration via C++26 reflection.
///
/// Functions decorated with [[=hpx::actions::detail::remote_function{}]] are
/// automatically discovered and registered as HPX plain actions.
///
/// Usage:
/// \code
/// namespace my_app {
///     [[=hpx::actions::detail::remote_function{}]]
///     int compute(int x) { return x + 1; }
/// }
/// HPX_REGISTER_ANNOTATED_ACTIONS(my_app)
///
/// \warning This macro must be invoked after all annotated function
/// definitions in \a Ns are complete. Since C++ namespaces are open,
/// functions added to \a Ns after this macro is expanded will NOT be
/// discovered or registered.
///
/// // Then dispatch via:
/// hpx::async<hpx::actions::reflect_action<^^my_app::compute>>(id, 42);
/// \endcode
#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)

#include <hpx/actions_base/reflect_action.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <array>
#include <cstddef>
#include <meta>

namespace hpx::actions::detail {

    /// Marker annotation for HPX remote function registration.
    /// Decorate functions with [[=hpx::actions::detail::remote_function{}]]
    /// then call HPX_REGISTER_ANNOTATED_ACTIONS(namespace) to register them.
    struct remote_function
    {
        int priority = 0;
    };

    /// Returns true if fn carries [[=hpx::actions::detail::remote_function{}]].
    consteval bool is_annotated_remote_fn(std::meta::info fn) noexcept
    {
        if (!std::meta::is_function(fn))
            return false;
        for (auto a : std::meta::annotations_of(fn))
            if (std::meta::remove_const(std::meta::type_of(a)) ==
                ^^hpx::actions::detail::remote_function)
                return true;
        return false;
    }

    /// Count annotated functions in a namespace.
    consteval std::size_t count_annotated_fns(std::meta::info ns) noexcept
    {
        std::size_t n = 0;
        for (auto m :
            std::meta::members_of(ns, std::meta::access_context::unchecked()))
            if (is_annotated_remote_fn(m))
                ++n;
        return n;
    }

    /// Collect all annotated functions in a namespace.
    template <std::meta::info Ns>
    consteval auto get_annotated_fns() noexcept
    {
        constexpr std::size_t N = count_annotated_fns(Ns);
        std::array<std::meta::info, N> result{};
        std::size_t i = 0;
        for (auto m :
            std::meta::members_of(Ns, std::meta::access_context::unchecked()))
            if (is_annotated_remote_fn(m))
                result[i++] = m;
        return result;
    }

    /// Force-instantiate reflect_action<^^fn> for each annotated function.
    /// Uses static constexpr + template for in function body (GCC trunk
    /// constraint: template for only works in function bodies, not at
    /// namespace scope or in class bodies).
    template <std::meta::info Ns>
    void register_annotated_actions() noexcept
    {
        static constexpr auto fns = get_annotated_fns<Ns>();
        template for (constexpr auto fn : fns)
        {
            // Touching the static registrar forces ODR-use and
            // triggers HPX action registration at program startup.
            using action_t = ::hpx::actions::reflect_action<fn>;
            (void) action_t::invocation_count_registrar_;
        }
    }

}    // namespace hpx::actions::detail

/// \brief Register all [[=hpx::actions::detail::remote_function{}]] annotated
/// functions in namespace \a Ns as HPX plain actions.
///
/// Place at namespace scope after all annotated function definitions.
#define HPX_REGISTER_ANNOTATED_ACTIONS(Ns)                                     \
    inline bool HPX_PP_CAT(Ns, _annotated_actions_registered_) =               \
        (::hpx::actions::detail::register_annotated_actions<^^Ns>(),           \
            true); /**/

#endif    // HPX_HAVE_CXX26_REFLECTION
