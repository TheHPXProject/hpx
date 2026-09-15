//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/executors/rebind_policy.hpp
///
/// \brief Orthogonal customization points for rebinding an execution
///        policy's executor and executor parameters independently of one
///        another.
///
/// hpx::execution::detail::execution_policy is the CRTP base class that
/// CRTP-based built-in execution policies derive from; not every
/// built-in policy uses it (thrust_task_policy is one example that does
/// not). It exposes a single, combined rebind operation: given a new
/// Executor and a new Parameters type, it produces
/// Derived<Executor, Parameters>, where Derived is the
/// template <typename, typename> class the concrete policy is written as.
/// hpx::execution::experimental::rebind_executor_t builds on top of that
/// combined operation and is what hpx::execution::detail::execution_policy
/// itself uses internally for on(), with(), and query().
///
/// That combined operation requires every execution policy to be shaped
/// exactly as Derived<Executor, Parameters>, which is awkward for a policy
/// that needs to carry additional state, expose additional template
/// parameters, or simply is not naturally expressed as a two-parameter
/// class template. This header adds two smaller, independently
/// specializable customization points, one per axis, so a policy can
/// opt in to rebinding along either axis without being forced into that
/// shape at all. This is purely additive: nothing here replaces or is
/// called by the existing combined rebind_executor_t, on(), with(), or
/// query() machinery.

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::execution::detail {

    /// \brief Customization point controlling how an execution policy is
    ///        rebound to a new executor, independently of its executor
    ///        parameters.
    ///
    /// To make a policy type participate, either rely on the default
    /// below (which requires nothing more than what
    /// hpx::execution::detail::execution_policy already provides: a
    /// nested \c executor_parameters_type member and a nested
    /// \c rebind<Executor_, Parameters_>::type member template), or
    /// specialize rebind_policy_executor for the policy type directly to
    /// define bespoke behavior. A direct specialization does not need to
    /// derive from hpx::execution::detail::execution_policy, or from
    /// anything else, at all.
    ///
    /// \tparam Policy   The execution policy type being rebound. Passed
    ///                  through std::decay_t before use, so cv- and
    ///                  reference-qualified Policy types are handled the
    ///                  same as their unqualified form.
    /// \tparam Executor The executor type Policy should be rebound to.
    ///                  Passed through std::decay_t before use.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    struct rebind_policy_executor
    {
    private:
        using decayed_policy_type = std::decay_t<Policy>;

    public:
        /// \brief The type of Policy rebound to Executor, with its
        ///        executor parameters left unchanged.
        ///
        /// The default implementation forwards to Policy's own
        /// \c rebind<Executor_, Parameters_>::type member template,
        /// supplying Policy's current \c executor_parameters_type as the
        /// Parameters_ argument so that only the executor changes.
        using type = typename decayed_policy_type::template rebind<
            std::decay_t<Executor>,
            typename decayed_policy_type::executor_parameters_type>::type;
    };

    /// \brief Convenience alias for
    ///        \c rebind_policy_executor<Policy, Executor>::type.
    ///
    /// Rebinds the executor of Policy to Executor, keeping its executor
    /// parameters unchanged.
    ///
    /// \tparam Policy   The execution policy type being rebound.
    /// \tparam Executor The executor type Policy should be rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    using rebind_policy_executor_t =
        typename rebind_policy_executor<std::decay_t<Policy>,
            std::decay_t<Executor>>::type;

    /// \brief Customization point controlling how an execution policy is
    ///        rebound to a new set of executor parameters, independently
    ///        of its executor.
    ///
    /// Mirrors rebind_policy_executor along the other axis: to make a
    /// policy type participate, either rely on the default below (which
    /// requires a nested \c executor_type member and a nested
    /// \c rebind<Executor_, Parameters_>::type member template), or
    /// specialize rebind_policy_parameters for the policy type directly
    /// to define bespoke behavior.
    ///
    /// \tparam Policy     The execution policy type being rebound. Passed
    ///                    through std::decay_t before use, so cv- and
    ///                    reference-qualified Policy types are handled the
    ///                    same as their unqualified form.
    /// \tparam Parameters The executor parameters type Policy should be
    ///                    rebound to. Passed through std::decay_t before
    ///                    use.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    struct rebind_policy_parameters
    {
    private:
        using decayed_policy_type = std::decay_t<Policy>;

    public:
        /// \brief The type of Policy rebound to Parameters, with its
        ///        executor left unchanged.
        ///
        /// The default implementation forwards to Policy's own
        /// \c rebind<Executor_, Parameters_>::type member template,
        /// supplying Policy's current \c executor_type as the Executor_
        /// argument so that only the executor parameters change.
        using type = typename decayed_policy_type::template rebind<
            typename decayed_policy_type::executor_type,
            std::decay_t<Parameters>>::type;
    };

    /// \brief Convenience alias for
    ///        \c rebind_policy_parameters<Policy, Parameters>::type.
    ///
    /// Rebinds the executor parameters of Policy to Parameters, keeping
    /// its executor unchanged.
    ///
    /// \tparam Policy     The execution policy type being rebound.
    /// \tparam Parameters The executor parameters type Policy should be
    ///                    rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    using rebind_policy_parameters_t =
        typename rebind_policy_parameters<std::decay_t<Policy>,
            std::decay_t<Parameters>>::type;
}    // namespace hpx::execution::detail
