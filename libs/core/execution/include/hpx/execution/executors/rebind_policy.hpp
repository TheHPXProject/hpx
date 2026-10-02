//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/executors/rebind_policy.hpp
///
/// \brief Customization points for rebinding an execution policy's
///        executor and executor parameters independently of one another.
///
/// rebind_executor_t rebinds both at once and requires the policy to be
/// shaped as Derived<Executor, Parameters>. The two customization points
/// below can be specialized per axis, so a policy that carries extra state
/// or has a different shape can still be rebound. Their defaults forward to
/// rebind_executor_t. The matching construction-side customization points
/// are in create_rebound_policy.hpp.

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/executors/rebind_executor.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>

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
}    // namespace hpx::execution::detail

namespace hpx::execution::experimental {

    /// \brief Customization point for rebinding an execution policy to a
    ///        new executor, keeping its executor parameters.
    ///
    /// The default needs a nested \c rebind<Executor_, Parameters_>::type,
    /// as hpx::execution::detail::execution_policy provides. Specialize it
    /// for policies that don't fit that shape. Use it through
    /// rebind_policy_executor_t, which also checks that Executor's
    /// execution category is not weaker than Policy's.
    ///
    /// \tparam Policy   The execution policy type being rebound.
    /// \tparam Executor The executor type Policy should be rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    struct rebind_policy_executor
    {
        /// The unchanged parameters type comes from
        /// extract_executor_parameters_t, so its sequential fallback and
        /// explicit specializations are honored.
        using type =
            rebind_executor_t<std::decay_t<Policy>, std::decay_t<Executor>,
                extract_executor_parameters_t<std::decay_t<Policy>>>;
    };

    /// \brief Customization point for rebinding an execution policy to new
    ///        executor parameters, keeping its executor.
    ///
    /// The default needs a nested \c executor_type and
    /// \c rebind<Executor_, Parameters_>::type. Specialize it for policies
    /// that don't fit that shape. Use it through rebind_policy_parameters_t.
    ///
    /// \tparam Policy     The execution policy type being rebound.
    /// \tparam Parameters The executor parameters type Policy should be
    ///                    rebound to.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    struct rebind_policy_parameters
    {
        using type = rebind_executor_t<std::decay_t<Policy>,
            typename std::decay_t<Policy>::executor_type,
            std::decay_t<Parameters>>;
    };
}    // namespace hpx::execution::experimental

namespace hpx::execution::detail {

    /// \brief Applies the category check outside rebind_policy_executor, so it
    ///        also covers direct specializations.
    ///
    /// Has no nested \c type if the execution category of Executor is weaker
    /// than that of Policy, so the check can be detected with a requires
    /// expression instead of failing hard.
    template <typename Policy, typename Executor>
    struct validated_rebind_policy_executor
    {
    };

    template <typename Policy, typename Executor>
        requires(experimental::detail::is_not_weaker_v<
            hpx::traits::executor_execution_category_t<Executor>,
            rebind_policy_executor_category_t<Policy>>)
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
        requires(experimental::detail::is_not_weaker_v<
            rebind_policy_current_executor_category_t<Policy>,
            rebind_policy_executor_category_t<Policy>>)
    struct validated_rebind_policy_parameters<Policy, Parameters>
    {
        using type =
            hpx::execution::experimental::rebind_policy_parameters<Policy,
                Parameters>::type;
    };
}    // namespace hpx::execution::detail

namespace hpx::execution::experimental {

    /// \brief Policy rebound to Executor, keeping its executor parameters.
    ///
    /// Applies rebind_policy_executor and checks that Executor's execution
    /// category is not weaker than Policy's, also for direct
    /// specializations of rebind_policy_executor. If the check fails, the
    /// alias does not name a type.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor>
    using rebind_policy_executor_t =
        hpx::execution::detail::validated_rebind_policy_executor<
            std::decay_t<Policy>, std::decay_t<Executor>>::type;

    /// \brief Policy rebound to Parameters, keeping its executor.
    ///
    /// Applies rebind_policy_parameters and checks that the execution
    /// category of Policy's current executor is not weaker than Policy's,
    /// also for direct specializations of rebind_policy_parameters. If the
    /// check fails, the alias does not name a type.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Parameters>
    using rebind_policy_parameters_t =
        hpx::execution::detail::validated_rebind_policy_parameters<
            std::decay_t<Policy>, std::decay_t<Parameters>>::type;

    /// \brief Whether rebinding the executor then the parameters gives the
    ///        same type as the other order.
    ///
    /// Always true for the defaults. A policy that specializes either
    /// customization point should static_assert it.
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Executor,
        typename Parameters>
    inline constexpr bool rebind_policy_order_independent_v = std::is_same_v<
        rebind_policy_parameters_t<rebind_policy_executor_t<Policy, Executor>,
            Parameters>,
        rebind_policy_executor_t<rebind_policy_parameters_t<Policy, Parameters>,
            Executor>>;
}    // namespace hpx::execution::experimental
