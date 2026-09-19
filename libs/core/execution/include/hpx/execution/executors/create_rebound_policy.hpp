//  Copyright (c) 2016-2024 Hartmut Kaiser
//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/executors/create_rebound_policy.hpp
///
/// \brief hpx::execution::experimental::create_rebound_policy, the
///        combined customization point that rebinds an execution policy's
///        executor and/or its executor parameters and constructs the
///        result, in one call.
///
/// This lives in its own header, separate from both rebind_executor.hpp
/// and rebind_policy.hpp: the single-argument overloads below route
/// through hpx::execution::detail::rebind_policy_executor_t and
/// rebind_policy_parameters_t (see rebind_policy.hpp) so that a policy
/// which customizes either axis is honored here too, instead of the two
/// call-sites drifting apart. rebind_policy.hpp already includes
/// rebind_executor.hpp for the two-argument overload's rebind_executor_t,
/// so defining create_rebound_policy_t in either of those two headers
/// directly would make them include each other; this header depends on
/// both instead, so neither of them needs to depend on it.

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/executors/rebind_executor.hpp>
#include <hpx/execution/executors/rebind_policy.hpp>

#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct create_rebound_policy_t final
    {
        /// \brief Rebind both the executor and the executor parameters of
        ///        \a policy in one step, and construct the result from
        ///        \a exec and \a parameters.
        ///
        /// Goes through the combined hpx::execution::experimental::
        /// rebind_executor_t, unchanged from before: rebinding both axes
        /// at once is not yet routed through the per-axis customization
        /// points (that integration is tracked separately), since doing
        /// so requires the combined rebind to also support per-axis
        /// specialization first.
        template <typename ExPolicy, typename Executor, typename Parameters>
            requires(hpx::executor_any<Executor> &&
                hpx::executor_parameters<Parameters>)
        constexpr decltype(auto) operator()(
            ExPolicy&&, Executor&& exec, Parameters&& parameters) const
        {
            using rebound_type =
                rebind_executor_t<ExPolicy, Executor, Parameters>;

            return rebound_type(HPX_FORWARD(Executor, exec),
                HPX_FORWARD(Parameters, parameters));
        }

        /// \brief Rebind only the executor of \a policy, keeping its
        ///        executor parameters, and construct the result.
        ///
        /// Routes through hpx::execution::detail::rebind_policy_executor_t
        /// rather than computing rebind_executor_t inline with a
        /// separately-extracted parameters type, so a policy providing a
        /// bespoke rebind_policy_executor specialization is honored here
        /// too, and existing call sites of the (pre-existing)
        /// create_rebound_policy automatically pick up that per-axis
        /// customization point without having to be rewritten.
        template <typename ExPolicy, typename Executor>
            requires(hpx::executor_any<Executor>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Executor&& exec) const
        {
            using rebound_type =
                hpx::execution::detail::rebind_policy_executor_t<ExPolicy,
                    Executor>;

            return rebound_type(
                HPX_FORWARD(Executor, exec), policy.parameters());
        }

        /// \brief Rebind only the executor parameters of \a policy,
        ///        keeping its executor, and construct the result.
        ///
        /// Mirrors the executor-only overload above along the other axis,
        /// routing through hpx::execution::detail::
        /// rebind_policy_parameters_t instead of computing
        /// rebind_executor_t inline.
        template <typename ExPolicy, typename Parameters>
            requires(hpx::executor_parameters<Parameters>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Parameters&& parameters) const
        {
            using rebound_type =
                hpx::execution::detail::rebind_policy_parameters_t<ExPolicy,
                    Parameters>;

            return rebound_type(
                policy.executor(), HPX_FORWARD(Parameters, parameters));
        }
    } create_rebound_policy{};
}    // namespace hpx::execution::experimental
