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
#include <hpx/modules/execution.hpp>

#include <type_traits>

namespace hpx::execution::detail {

    /// \cond NOINTERNAL
    /// \brief The execution category of Policy, or
    ///        hpx::execution::unsequenced_execution_tag (the weakest
    ///        category) if Policy has no nested \c execution_category
    ///        member. Mirrors how hpx::traits::executor_execution_category
    ///        falls back for executors, so a policy that predates this
    ///        check keeps compiling and is simply not constrained by it.
    template <typename Policy>
    struct rebind_policy_executor_category
    {
    private:
        template <typename T>
        using execution_category_of = T::execution_category;

    public:
        using type =
            hpx::util::detected_or_t<hpx::execution::unsequenced_execution_tag,
                execution_category_of, Policy>;
    };
    /// \endcond

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
    /// The default implementation enforces the same safety guarantee as
    /// hpx::execution::experimental::rebind_executor: Executor's
    /// execution category must not be weaker than Policy's. A Policy
    /// without a nested \c execution_category member is treated as
    /// hpx::execution::unsequenced_execution_tag, the weakest category,
    /// so the check never rejects a policy that simply does not track
    /// one.
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
        using decayed_executor_type = std::decay_t<Executor>;

        using category1 =
            rebind_policy_executor_category<decayed_policy_type>::type;
        using category2 =
            hpx::traits::executor_execution_category_t<decayed_executor_type>;

        static_assert(
            hpx::execution::experimental::detail::is_not_weaker_v<category2,
                category1>,
            "the execution category of Executor must not be weaker than "
            "that of Policy; see hpx::execution::experimental::"
            "rebind_executor");

    public:
        /// \brief The type of Policy rebound to Executor, with its
        ///        executor parameters left unchanged.
        ///
        /// The default implementation forwards to Policy's own
        /// \c rebind<Executor_, Parameters_>::type member template,
        /// supplying Policy's current \c executor_parameters_type as the
        /// Parameters_ argument so that only the executor changes.
        using type = decayed_policy_type::template rebind<decayed_executor_type,
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
        rebind_policy_executor<Policy, Executor>::type;

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
        using type = decayed_policy_type::template rebind<
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
        rebind_policy_parameters<Policy, Parameters>::type;

    /// \brief Whether rebinding Policy's executor and executor parameters
    ///        through rebind_policy_executor_t and
    ///        rebind_policy_parameters_t is order-independent for the
    ///        given Executor and Parameters, i.e. whether rebinding the
    ///        executor first and the parameters second yields the same
    ///        type as doing it in the opposite order.
    ///
    /// This holds structurally for the default implementations of both
    /// customization points, since they both funnel through the same
    /// combined \c Policy::rebind<Executor_, Parameters_>::type
    /// mechanism regardless of which axis is rebound first. It is not
    /// guaranteed automatically for a Policy that specializes
    /// rebind_policy_executor and/or rebind_policy_parameters directly;
    /// an author providing such a specialization should
    /// static_assert this trait to pin down the invariant that callers
    /// of both customization points rely on.
    ///
    /// \tparam Policy     The execution policy type being rebound.
    /// \tparam Executor   The executor type Policy should be rebound to.
    /// \tparam Parameters The executor parameters type Policy should be
    ///                    rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor,
        typename Parameters>
    inline constexpr bool rebind_policy_order_independent_v = std::is_same_v<
        rebind_policy_parameters_t<rebind_policy_executor_t<Policy, Executor>,
            Parameters>,
        rebind_policy_executor_t<rebind_policy_parameters_t<Policy, Parameters>,
            Executor>>;
}    // namespace hpx::execution::detail
