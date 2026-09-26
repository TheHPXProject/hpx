//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
        ///        customization points in rebind_policy.hpp so both places
        ///        apply the same fallback and a Policy that predates this
        ///        check keeps compiling everywhere, not just in one place.
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

    // create_rebound_policy_t (the combined rebind customization point) is
    // defined in hpx/execution/executors/create_rebound_policy.hpp, not
    // here: its single-argument overloads route through
    // hpx::execution::detail::rebind_policy_executor_t and
    // rebind_policy_parameters_t (see rebind_policy.hpp), and rebind_policy.hpp
    // itself includes this header, so defining it here would make the two
    // headers include each other. Code that wants create_rebound_policy
    // should include create_rebound_policy.hpp directly, or the umbrella
    // hpx/execution.hpp, which already pulls it in.
}    // namespace hpx::execution::experimental
