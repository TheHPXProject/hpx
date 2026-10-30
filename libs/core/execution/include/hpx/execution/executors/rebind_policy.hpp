//  Copyright (c) 2016-2024 Hartmut Kaiser
//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/executors/rebind_policy.hpp
///
/// \brief rebind_executor and the customization points for rebinding an
///        execution policy's executor and executor parameters independently
///        of one another.
///
/// rebind_executor_t rebinds both at once and requires the policy to be
/// shaped as Derived<Executor, Parameters>. rebind_policy_executor and
/// rebind_policy_parameters can be specialized per axis, so a policy that
/// carries extra state or has a different shape can still be rebound. Their
/// defaults forward to rebind_executor_t. The matching construction-side
/// customization points are in create_rebound_policy.hpp.

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/traits/executor_traits.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        /// \cond NOINTERNAL
        template <typename Category1, typename Category2>
        struct is_not_weaker : std::false_type
        {
        };

        template <typename Category>
        struct is_not_weaker<Category, Category> : std::true_type
        {
        };

        template <>
        struct is_not_weaker<hpx::execution::parallel_execution_tag,
            hpx::execution::unsequenced_execution_tag> : std::true_type
        {
        };

        template <>
        struct is_not_weaker<hpx::execution::sequenced_execution_tag,
            hpx::execution::unsequenced_execution_tag> : std::true_type
        {
        };

        template <>
        struct is_not_weaker<hpx::execution::sequenced_execution_tag,
            hpx::execution::parallel_execution_tag> : std::true_type
        {
        };

        template <typename Category1, typename Category2>
        inline constexpr bool is_not_weaker_v =
            is_not_weaker<Category1, Category2>::value;

        /// \brief The execution category of Policy, or
        ///        hpx::execution::unsequenced_execution_tag (the weakest
        ///        category) if Policy has no nested \c execution_category
        ///        member. Shared by rebind_executor and by the per-axis
        ///        customization points below so both places apply the same
        ///        fallback and a Policy that predates this check keeps
        ///        compiling everywhere, not just in one place.
        template <typename Policy>
        struct policy_execution_category_or_unsequenced
        {
        private:
            template <typename T>
            using execution_category_of = T::execution_category;

        public:
            using type = hpx::util::detected_or_t<
                hpx::execution::unsequenced_execution_tag,
                execution_category_of, Policy>;
        };

        template <typename Policy>
        using policy_execution_category_or_unsequenced_t =
            policy_execution_category_or_unsequenced<Policy>::type;
        /// \endcond
    }    // namespace detail

    /// Rebind the type of executor used by an execution policy. The execution
    /// category of Executor shall not be weaker than that of ExecutionPolicy.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Executor,
        typename Parameters>
    struct rebind_executor
    {
        /// \cond NOINTERNAL
        using policy_type = std::decay_t<ExPolicy>;
        using executor_type = std::decay_t<Executor>;
        using parameters_type = std::decay_t<Parameters>;

        using category1 =
            detail::policy_execution_category_or_unsequenced_t<policy_type>;
        using category2 =
            hpx::traits::executor_execution_category_t<executor_type>;

        static_assert(detail::is_not_weaker_v<category2, category1>,
            "detail::is_not_weaker_v<category2, category1>");
        /// \endcond

        /// The type of the rebound execution policy
        using type = typename policy_type::template rebind<executor_type,
            parameters_type>::type;
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Executor,
        typename Parameters>
    using rebind_executor_t =
        typename rebind_executor<ExPolicy, Executor, Parameters>::type;
}    // namespace hpx::execution::experimental

namespace hpx::execution::detail {

    /// \brief Policy's execution category, or unsequenced_execution_tag if it
    ///        has none. Same fallback rebind_executor uses.
    template <typename Policy>
    using rebind_policy_executor_category_t =
        experimental::detail::policy_execution_category_or_unsequenced_t<
            Policy>;

    /// \brief Category of Policy's current executor. Falls back to Policy's
    ///        own category if it has no executor_type, which a policy that
    ///        only specializes rebind_policy_parameters doesn't need to have.
    template <typename Policy>
    struct rebind_policy_current_executor_category
    {
    private:
        template <typename T>
        using executor_type_of = T::executor_type;

        using current_executor_type =
            hpx::util::detected_or_t<void, executor_type_of, Policy>;

    public:
        using type = std::conditional_t<std::is_void_v<current_executor_type>,
            rebind_policy_executor_category_t<Policy>,
            hpx::traits::executor_execution_category_t<current_executor_type>>;
    };

    template <typename Policy>
    using rebind_policy_current_executor_category_t =
        rebind_policy_current_executor_category<Policy>::type;

    /// \brief Whether Policy has a nested rebind<Executor, Parameters>::type.
    template <typename Policy, typename Executor, typename Parameters>
    concept policy_has_rebind = requires {
        typename Policy::template rebind<Executor, Parameters>::type;
    };

    /// \brief Whether Policy has a nested executor_type.
    template <typename Policy>
    concept policy_has_executor_type =
        requires { typename Policy::executor_type; };

    /// \brief Default implementation of rebind_policy_executor.
    ///
    /// Has no nested \c type if Policy has no nested
    /// \c rebind<Executor, Parameters_>::type, so the default fails in a
    /// SFINAE-friendly way. The unchanged parameters type comes from
    /// extract_executor_parameters_t, so its sequential fallback and explicit
    /// specializations are honored.
    template <typename Policy, typename Executor>
    struct default_rebind_policy_executor
    {
    };

    template <typename Policy, typename Executor>
        requires(policy_has_rebind<Policy, Executor,
            experimental::extract_executor_parameters_t<Policy>>)
    struct default_rebind_policy_executor<Policy, Executor>
    {
        using type = experimental::rebind_executor_t<Policy, Executor,
            experimental::extract_executor_parameters_t<Policy>>;
    };

    /// \brief Default implementation of rebind_policy_parameters.
    ///
    /// Has no nested \c type if Policy has no nested \c executor_type or
    /// \c rebind<Executor_, Parameters>::type, so the default fails in a
    /// SFINAE-friendly way.
    template <typename Policy, typename Parameters>
    struct default_rebind_policy_parameters
    {
    };

    template <typename Policy, typename Parameters>
        requires(policy_has_executor_type<Policy> &&
            policy_has_rebind<Policy, typename Policy::executor_type,
                Parameters>)
    struct default_rebind_policy_parameters<Policy, Parameters>
    {
        using type = experimental::rebind_executor_t<Policy,
            typename Policy::executor_type, Parameters>;
    };
}    // namespace hpx::execution::detail

namespace hpx::execution::experimental {

    /// \brief Customization point for rebinding an execution policy to a
    ///        new executor, keeping its executor parameters.
    ///
    /// The default needs a nested \c rebind<Executor_, Parameters_>::type,
    /// as hpx::execution::detail::execution_policy provides, and has no
    /// nested \c type otherwise. Specialize it for policies that don't fit
    /// that shape. Use it through rebind_policy_executor_t, which also checks
    /// that Executor's execution category is not weaker than Policy's.
    ///
    /// A specialization that can't rebind Policy to a given Executor should
    /// leave out the nested \c type instead of failing to compile, so that
    /// rebind_policy_executor_t, create_rebound_policy_executor and
    /// rebind_policy_order_independent_v can detect it.
    ///
    /// \tparam Policy   The execution policy type being rebound.
    /// \tparam Executor The executor type Policy should be rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    struct rebind_policy_executor
      : hpx::execution::detail::default_rebind_policy_executor<
            std::decay_t<Policy>, std::decay_t<Executor>>
    {
    };

    /// \brief Customization point for rebinding an execution policy to new
    ///        executor parameters, keeping its executor.
    ///
    /// The default needs a nested \c executor_type and
    /// \c rebind<Executor_, Parameters_>::type, and has no nested \c type
    /// otherwise. Specialize it for policies that don't fit that shape. Use
    /// it through rebind_policy_parameters_t.
    ///
    /// A specialization that can't rebind Policy to given Parameters should
    /// leave out the nested \c type instead of failing to compile, so that
    /// rebind_policy_parameters_t, create_rebound_policy_parameters and
    /// rebind_policy_order_independent_v can detect it.
    ///
    /// \tparam Policy     The execution policy type being rebound.
    /// \tparam Parameters The executor parameters type Policy should be
    ///                    rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    struct rebind_policy_parameters
      : hpx::execution::detail::default_rebind_policy_parameters<
            std::decay_t<Policy>, std::decay_t<Parameters>>
    {
    };
}    // namespace hpx::execution::experimental

namespace hpx::execution::detail {

    /// \brief Whether the execution category of Executor is not weaker than
    ///        that of Policy.
    template <typename Policy, typename Executor>
    concept executor_category_not_weaker =
        experimental::detail::is_not_weaker_v<
            hpx::traits::executor_execution_category_t<Executor>,
            rebind_policy_executor_category_t<Policy>>;

    /// \brief Whether the execution category of Policy's current executor is
    ///        not weaker than that of Policy.
    template <typename Policy>
    concept current_executor_category_not_weaker =
        experimental::detail::is_not_weaker_v<
            rebind_policy_current_executor_category_t<Policy>,
            rebind_policy_executor_category_t<Policy>>;

    /// \brief Whether rebind_policy_executor has a nested type for Policy and
    ///        Executor.
    template <typename Policy, typename Executor>
    concept executor_can_be_rebound = requires {
        typename experimental::rebind_policy_executor<Policy, Executor>::type;
    };

    /// \brief Whether rebind_policy_parameters has a nested type for Policy
    ///        and Parameters.
    template <typename Policy, typename Parameters>
    concept parameters_can_be_rebound = requires {
        typename experimental::rebind_policy_parameters<Policy,
            Parameters>::type;
    };

    /// \brief Applies the category check outside rebind_policy_executor, so it
    ///        also covers direct specializations.
    ///
    /// Has no nested \c type if the execution category of Executor is weaker
    /// than that of Policy, or if rebind_policy_executor has no nested
    /// \c type, so both can be detected with a requires expression instead
    /// of failing hard.
    template <typename Policy, typename Executor>
    struct validated_rebind_policy_executor
    {
    };

    template <typename Policy, typename Executor>
        requires(executor_category_not_weaker<Policy, Executor> &&
            executor_can_be_rebound<Policy, Executor>)
    struct validated_rebind_policy_executor<Policy, Executor>
    {
        using type =
            experimental::rebind_policy_executor<Policy, Executor>::type;
    };

    /// \brief Same check as validated_rebind_policy_executor, against Policy's
    ///        current executor, so direct specializations are validated too.
    template <typename Policy, typename Parameters>
    struct validated_rebind_policy_parameters
    {
    };

    template <typename Policy, typename Parameters>
        requires(current_executor_category_not_weaker<Policy> &&
            parameters_can_be_rebound<Policy, Parameters>)
    struct validated_rebind_policy_parameters<Policy, Parameters>
    {
        using type =
            experimental::rebind_policy_parameters<Policy, Parameters>::type;
    };
}    // namespace hpx::execution::detail

namespace hpx::execution::experimental {

    /// \brief Policy rebound to Executor, keeping its executor parameters.
    ///
    /// Applies rebind_policy_executor and checks that Executor's execution
    /// category is not weaker than Policy's, also for direct
    /// specializations of rebind_policy_executor. If the check fails, or
    /// rebind_policy_executor has no nested \c type, the alias does not name
    /// a type.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    using rebind_policy_executor_t =
        hpx::execution::detail::validated_rebind_policy_executor<
            std::decay_t<Policy>, std::decay_t<Executor>>::type;

    /// \brief Policy rebound to Parameters, keeping its executor.
    ///
    /// Applies rebind_policy_parameters and checks that the execution
    /// category of Policy's current executor is not weaker than Policy's,
    /// also for direct specializations of rebind_policy_parameters. If the
    /// check fails, or rebind_policy_parameters has no nested \c type, the
    /// alias does not name a type.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    using rebind_policy_parameters_t =
        hpx::execution::detail::validated_rebind_policy_parameters<
            std::decay_t<Policy>, std::decay_t<Parameters>>::type;

    /// \brief Whether rebinding the executor then the parameters gives the
    ///        same type as the other order.
    ///
    /// Not satisfied if either order can't be rebound, so it can be used in a
    /// constraint without failing to compile. Satisfied for the defaults
    /// whenever both orders can be rebound. A policy that specializes either
    /// customization point should static_assert it.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor,
        typename Parameters>
    concept rebind_policy_order_independent = std::is_same_v<
        rebind_policy_parameters_t<rebind_policy_executor_t<Policy, Executor>,
            Parameters>,
        rebind_policy_executor_t<rebind_policy_parameters_t<Policy, Parameters>,
            Executor>>;

    /// \brief Value of rebind_policy_order_independent.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor,
        typename Parameters>
    inline constexpr bool rebind_policy_order_independent_v =
        rebind_policy_order_independent<Policy, Executor, Parameters>;
}    // namespace hpx::execution::experimental
