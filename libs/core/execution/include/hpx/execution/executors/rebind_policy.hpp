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
#include <hpx/execution/executors/rebind_executor.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>

namespace hpx::execution::detail {

    /// \cond NOINTERNAL
    /// \brief The execution category of Policy, or
    ///        hpx::execution::unsequenced_execution_tag (the weakest
    ///        category) if Policy has no nested \c execution_category
    ///        member.
    ///
    /// This is the same fallback rebind_executor itself now applies to its
    /// own category1 (see hpx::execution::experimental::detail::
    /// policy_execution_category_or_unsequenced in rebind_executor.hpp), so
    /// a policy that predates this check compiles and is simply not
    /// constrained by it, consistently whether it goes through
    /// rebind_executor_t directly or through the per-axis customization
    /// points below.
    template <typename Policy>
    using rebind_policy_executor_category_t = hpx::execution::experimental::
        detail::policy_execution_category_or_unsequenced_t<Policy>;

    /// \brief The execution category of Policy's own, current executor
    ///        (i.e. Policy::executor_type), or Policy's own execution
    ///        category (see rebind_policy_executor_category_t above) if
    ///        Policy has no nested \c executor_type member.
    ///
    /// A Policy that participates in rebind_policy_parameters only through
    /// a direct specialization is not required to expose an
    /// \c executor_type member at all (that member only exists to support
    /// the default implementation's forwarding to rebind_executor_t), so
    /// there may be no "current executor" for validated_rebind_policy_
    /// parameters below to inspect. Falling back to Policy's own execution
    /// category in that case makes the check trivially satisfied through
    /// the reflexive hpx::execution::experimental::detail::is_not_weaker_v
    /// specialization, rather than hard-failing to compile a Policy the
    /// default implementation was never going to be used for anyway.
    template <typename Policy>
    struct rebind_policy_own_executor_category
    {
    private:
        template <typename T>
        using executor_type_of = T::executor_type;

        using own_executor_type =
            hpx::util::detected_or_t<void, executor_type_of, Policy>;

    public:
        using type = std::conditional_t<std::is_void_v<own_executor_type>,
            rebind_policy_executor_category_t<Policy>,
            hpx::traits::executor_execution_category_t<own_executor_type>>;
    };

    template <typename Policy>
    using rebind_policy_own_executor_category_t =
        rebind_policy_own_executor_category<Policy>::type;
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
    /// rebind_policy_executor itself performs no validation; the safety
    /// guarantee hpx::execution::experimental::rebind_executor enforces
    /// (Executor's execution category must not be weaker than Policy's)
    /// is applied uniformly to both the default implementation below and
    /// to any direct specialization by rebind_policy_executor_t, through
    /// detail::validated_rebind_policy_executor. A Policy without a
    /// nested \c execution_category member is treated as
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

    public:
        /// \brief The type of Policy rebound to Executor, with its
        ///        executor parameters left unchanged.
        ///
        /// The default implementation forwards to
        /// hpx::execution::experimental::rebind_executor_t, supplying
        /// Policy's current \c executor_parameters_type as the
        /// Parameters_ argument so that only the executor changes.
        using type =
            hpx::execution::experimental::rebind_executor_t<decayed_policy_type,
                decayed_executor_type,
                typename decayed_policy_type::executor_parameters_type>;
    };

    namespace detail {

        /// \brief Applies the execution category safety check to
        ///        rebind_policy_executor's result, so the check runs
        ///        uniformly whether rebind_policy_executor is used
        ///        through its default implementation or through a
        ///        direct specialization: a specialization replaces the
        ///        primary template entirely, so the check cannot live
        ///        inside rebind_policy_executor itself if it is to apply
        ///        to every specialization. rebind_policy_executor_t
        ///        below is the public entry point that goes through
        ///        this wrapper.
        template <typename Policy, typename Executor>
        struct validated_rebind_policy_executor
        {
        private:
            using decayed_policy_type = std::decay_t<Policy>;
            using decayed_executor_type = std::decay_t<Executor>;

            using category1 =
                rebind_policy_executor_category_t<decayed_policy_type>;
            using category2 = hpx::traits::executor_execution_category_t<
                decayed_executor_type>;

            static_assert(
                hpx::execution::experimental::detail::is_not_weaker_v<category2,
                    category1>,
                "the execution category of Executor must not be weaker than "
                "that of Policy; see hpx::execution::experimental::"
                "rebind_executor");

        public:
            using type = rebind_policy_executor<decayed_policy_type,
                decayed_executor_type>::type;
        };

    }    // namespace detail

    /// \brief Convenience alias for
    ///        \c detail::validated_rebind_policy_executor<Policy,
    ///        Executor>::type, i.e. \c rebind_policy_executor<Policy,
    ///        Executor>::type with the execution category safety check
    ///        applied first.
    ///
    /// Rebinds the executor of Policy to Executor, keeping its executor
    /// parameters unchanged.
    ///
    /// \tparam Policy   The execution policy type being rebound.
    /// \tparam Executor The executor type Policy should be rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    using rebind_policy_executor_t =
        detail::validated_rebind_policy_executor<Policy, Executor>::type;

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
        /// The default implementation forwards to
        /// hpx::execution::experimental::rebind_executor_t, supplying
        /// Policy's current \c executor_type as the Executor argument so
        /// that only the executor parameters change. This mirrors
        /// rebind_policy_executor's default implementation, which forwards
        /// to the same rebind_executor_t along the other axis, instead of
        /// calling Policy's \c rebind<Executor_, Parameters_>::type member
        /// template directly; keeping both axes funneled through the one
        /// shared implementation means a future change there (e.g. to what
        /// is validated or computed) applies to both axes automatically
        /// instead of drifting apart.
        using type =
            hpx::execution::experimental::rebind_executor_t<decayed_policy_type,
                typename decayed_policy_type::executor_type,
                std::decay_t<Parameters>>;
    };

    namespace detail {

        /// \brief Mirrors validated_rebind_policy_executor along the
        ///        parameters axis, so a direct specialization of
        ///        rebind_policy_parameters is checked the same way a
        ///        specialization of rebind_policy_executor is: going
        ///        through the primary template's default implementation is
        ///        not the only way to get the safety check, since a
        ///        specialization replaces the primary template (and
        ///        whatever it forwards to) entirely.
        ///
        /// The check itself is trivially satisfied for this axis in the
        /// default case, since the executor does not change here, but
        /// applying it uniformly keeps rebind_policy_executor_t and
        /// rebind_policy_parameters_t symmetric customization points with
        /// the same contract, rather than only one of the two axes being
        /// independently guarded.
        ///
        /// category2 goes through rebind_policy_own_executor_category_t
        /// rather than accessing Policy::executor_type directly, since a
        /// Policy participating only through a direct specialization of
        /// rebind_policy_parameters need not expose that member at all.
        template <typename Policy, typename Parameters>
        struct validated_rebind_policy_parameters
        {
        private:
            using decayed_policy_type = std::decay_t<Policy>;

            using category1 =
                rebind_policy_executor_category_t<decayed_policy_type>;
            using category2 =
                rebind_policy_own_executor_category_t<decayed_policy_type>;

            static_assert(
                hpx::execution::experimental::detail::is_not_weaker_v<category2,
                    category1>,
                "the execution category of Policy's own executor must not "
                "be weaker than that of Policy; see hpx::execution::"
                "experimental::rebind_executor");

        public:
            using type = rebind_policy_parameters<decayed_policy_type,
                std::decay_t<Parameters>>::type;
        };

    }    // namespace detail

    /// \brief Convenience alias for
    ///        \c detail::validated_rebind_policy_parameters<Policy,
    ///        Parameters>::type, i.e. \c rebind_policy_parameters<Policy,
    ///        Parameters>::type with the execution category safety check
    ///        applied first.
    ///
    /// Rebinds the executor parameters of Policy to Parameters, keeping
    /// its executor unchanged.
    ///
    /// \tparam Policy     The execution policy type being rebound.
    /// \tparam Parameters The executor parameters type Policy should be
    ///                    rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    using rebind_policy_parameters_t =
        detail::validated_rebind_policy_parameters<Policy, Parameters>::type;

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
