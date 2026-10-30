//  Copyright (c) 2016-2024 Hartmut Kaiser
//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/executors/create_rebound_policy.hpp
///
/// \brief create_rebound_policy and the per-axis construction
///        customization points it dispatches to.
///
/// rebind_executor.hpp includes this header, so code that includes
/// rebind_executor.hpp directly still gets create_rebound_policy.

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/executors/rebind_policy.hpp>
#include <hpx/modules/execution_base.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    /// \brief Customization point controlling how an execution policy
    ///        rebound to a new executor is constructed from the original
    ///        policy.
    ///
    /// rebind_policy_executor only computes the rebound type. A policy that
    /// carries additional state, or that cannot be constructed from just
    /// (executor, parameters), specializes this template to build the
    /// rebound policy itself. \c call receives the original policy, forwarded
    /// with the value category it was passed with, so any such state can be
    /// carried over, and the new executor.
    ///
    /// \c call must return exactly rebind_policy_executor_t<Policy, Executor>.
    /// It must also be SFINAE-friendly: create_rebound_policy_executor
    /// checks the call in a requires expression, so a specialization that
    /// can't handle some arguments has to make the call expression itself
    /// ill-formed, for example by constraining \c call or by giving it a
    /// return type that depends on its own template parameters. A failure
    /// inside the body of \c call, or in a member declaration that depends
    /// only on Policy and Executor, is a hard error.
    ///
    /// The default constructs the rebound policy from the new executor and
    /// the original policy's parameters().
    ///
    /// \tparam Policy   The (decayed) execution policy type being rebound.
    /// \tparam Executor The (decayed) executor type Policy is rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    struct construct_rebound_policy_executor
    {
        template <typename Policy_, typename Executor_>
        static constexpr rebind_policy_executor_t<Policy, Executor_> call(
            Policy_&& policy, Executor_&& exec)
        {
            return rebind_policy_executor_t<Policy, Executor_>(
                HPX_FORWARD(Executor_, exec),
                HPX_FORWARD(Policy_, policy).parameters());
        }
    };

    /// \brief Customization point controlling how an execution policy
    ///        rebound to new executor parameters is constructed from the
    ///        original policy.
    ///
    /// Mirrors construct_rebound_policy_executor along the parameters axis,
    /// with the same requirements: \c call must return exactly
    /// rebind_policy_parameters_t<Policy, Parameters> and be
    /// SFINAE-friendly.
    ///
    /// The default constructs the rebound policy from the original
    /// policy's executor() and the new parameters.
    ///
    /// \tparam Policy     The (decayed) execution policy type being rebound.
    /// \tparam Parameters The (decayed) executor parameters type Policy is
    ///                    rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    struct construct_rebound_policy_parameters
    {
        template <typename Policy_, typename Parameters_>
        static constexpr rebind_policy_parameters_t<Policy, Parameters_> call(
            Policy_&& policy, Parameters_&& parameters)
        {
            return rebind_policy_parameters_t<Policy, Parameters_>(
                HPX_FORWARD(Policy_, policy).executor(),
                HPX_FORWARD(Parameters_, parameters));
        }
    };

    /// \brief Whether a policy of type Policy can be rebound to an executor
    ///        of type Executor through create_rebound_policy_executor.
    ///
    /// Satisfied if Executor is an executor and
    /// construct_rebound_policy_executor accepts the forwarded policy and
    /// executor and returns rebind_policy_executor_t<Policy, Executor>, so
    /// in particular only if that type exists.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    concept rebound_policy_executor_constructible =
        hpx::executor_any<Executor> &&
        requires(Policy&& policy, Executor&& exec) {
            {
                construct_rebound_policy_executor<std::decay_t<Policy>,
                    std::decay_t<Executor>>::call(HPX_FORWARD(Policy, policy),
                    HPX_FORWARD(Executor, exec))
            } -> std::same_as<rebind_policy_executor_t<Policy, Executor>>;
        };

    /// \brief Whether a policy of type Policy can be rebound to executor
    ///        parameters of type Parameters through
    ///        create_rebound_policy_parameters.
    ///
    /// Mirrors rebound_policy_executor_constructible along the parameters
    /// axis.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    concept rebound_policy_parameters_constructible =
        hpx::executor_parameters<Parameters> &&
        requires(Policy&& policy, Parameters&& params) {
            {
                construct_rebound_policy_parameters<std::decay_t<Policy>,
                    std::decay_t<Parameters>>::call(HPX_FORWARD(Policy, policy),
                    HPX_FORWARD(Parameters, params))
            } -> std::same_as<rebind_policy_parameters_t<Policy, Parameters>>;
        };

    /// \brief Rebind the executor of \a policy to \a exec and construct the
    ///        result through construct_rebound_policy_executor.
    ///
    /// Only participates in overload resolution if
    /// rebound_policy_executor_constructible<Policy, Executor> is satisfied.
    HPX_CXX_CORE_EXPORT inline constexpr struct
        create_rebound_policy_executor_t final
    {
        template <typename Policy, typename Executor>
            requires(rebound_policy_executor_constructible<Policy, Executor>)
        constexpr rebind_policy_executor_t<Policy, Executor> operator()(
            Policy&& policy, Executor&& exec) const
        {
            return construct_rebound_policy_executor<std::decay_t<Policy>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Policy, policy),
                HPX_FORWARD(Executor, exec));
        }
    } create_rebound_policy_executor{};

    /// \brief Rebind the executor parameters of \a policy to \a parameters
    ///        and construct the result through
    ///        construct_rebound_policy_parameters.
    ///
    /// Only participates in overload resolution if
    /// rebound_policy_parameters_constructible<Policy, Parameters> is
    /// satisfied.
    HPX_CXX_CORE_EXPORT inline constexpr struct
        create_rebound_policy_parameters_t final
    {
        template <typename Policy, typename Parameters>
            requires(
                rebound_policy_parameters_constructible<Policy, Parameters>)
        constexpr rebind_policy_parameters_t<Policy, Parameters> operator()(
            Policy&& policy, Parameters&& parameters) const
        {
            return construct_rebound_policy_parameters<std::decay_t<Policy>,
                std::decay_t<Parameters>>::call(HPX_FORWARD(Policy, policy),
                HPX_FORWARD(Parameters, parameters));
        }
    } create_rebound_policy_parameters{};

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct create_rebound_policy_t final
    {
        /// \brief Rebind both the executor and the executor parameters of
        ///        \a policy, one axis at a time through
        ///        create_rebound_policy_executor and
        ///        create_rebound_policy_parameters.
        ///
        /// The executor is rebound first, so the specializations of both
        /// axes are honored and state that \a policy carries is kept, the
        /// same as when rebinding a single axis.
        template <typename ExPolicy, typename Executor, typename Parameters>
            requires(
                rebound_policy_executor_constructible<ExPolicy, Executor> &&
                rebound_policy_parameters_constructible<
                    rebind_policy_executor_t<ExPolicy, Executor>, Parameters>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Executor&& exec, Parameters&& parameters) const
        {
            return create_rebound_policy_parameters(
                create_rebound_policy_executor(
                    HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Executor, exec)),
                HPX_FORWARD(Parameters, parameters));
        }

        /// \brief Rebind only the executor of \a policy, through
        ///        create_rebound_policy_executor.
        template <typename ExPolicy, typename Executor>
            requires(rebound_policy_executor_constructible<ExPolicy, Executor>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Executor&& exec) const
        {
            return create_rebound_policy_executor(
                HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Executor, exec));
        }

        /// \brief Rebind only the executor parameters of \a policy,
        ///        through create_rebound_policy_parameters.
        template <typename ExPolicy, typename Parameters>
            requires(
                rebound_policy_parameters_constructible<ExPolicy, Parameters>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Parameters&& parameters) const
        {
            return create_rebound_policy_parameters(
                HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(Parameters, parameters));
        }
    } create_rebound_policy{};
}    // namespace hpx::execution::experimental
