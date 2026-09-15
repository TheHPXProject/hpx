//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file post.hpp
/// \page hpx::post_distributed hpx::post (distributed)
/// \headerfile hpx/async.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    /// \brief The distributed implementation of \c hpx::post can be used by
    ///        giving an action instance as argument instead of a function,
    ///        and also by providing another argument with the locality ID or
    ///        the target ID
    ///
    /// \tparam Action The type of action instance
    /// \tparam Target The type of target where the action should be executed
    /// \tparam Ts     The type of any additional arguments
    ///
    /// \param action  The action instance to be executed
    /// \param target  The target where the action should be executed
    /// \param ts      Additional arguments
    ///
    /// \returns \c true if the action was successfully posted,
    ///          \c false otherwise.
    template <typename Action, typename Target, typename... Ts>
    bool post(Action&& action, Target&& target, Ts&&... ts);
    // clang-format on
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/async_distributed/bind_action.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/modules/async_local.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    // bound action
    template <typename Bound>
    struct post_dispatch<Bound, std::enable_if_t<hpx::is_bound_action_v<Bound>>>
    {
        template <typename Action, typename Is, typename... Ts, typename... Us>
        HPX_FORCEINLINE static bool call(
            hpx::detail::bound_action<Action, Is, Ts...> const& bound,
            Us&&... vs)
        {
            return bound.post(HPX_FORWARD(Us, vs)...);
        }
    };
}    // namespace hpx::detail

#endif

#if defined(HPX_HAVE_CXX26_REFLECTION)
#include <hpx/actions_base/reflect_action.hpp>

namespace hpx {

    /// \brief Reflection-based post overload.
    ///
    /// Allows calling hpx::post<^^func>(target, ...) directly without
    /// defining an explicit action type. Internally constructs
    /// reflect_action<F> and delegates to the existing post machinery.
    ///
    /// \tparam F      A std::meta::info reflection of a free function.
    /// \tparam Target id_type, client, or distribution policy.
    /// \tparam Ts     Additional arguments to pass to the function.
    // clang-format off
    HPX_CXX_EXPORT template <std::meta::info F, typename Target, typename... Ts>
        requires(std::meta::is_namespace_member(F) && std::meta::is_function(F) &&
            (std::is_same_v<std::decay_t<Target>, hpx::id_type> ||
                hpx::traits::is_client_v<std::decay_t<Target>> ||
                hpx::traits::is_distribution_policy_v<std::decay_t<Target>>))
    HPX_FORCEINLINE bool post(Target&& target, Ts&&... ts)
    // clang-format on
    {
        return hpx::post(hpx::actions::reflect_action<F>{},
            HPX_FORWARD(Target, target), HPX_FORWARD(Ts, ts)...);
    }

}    // namespace hpx
#endif    // HPX_HAVE_CXX26_REFLECTION
