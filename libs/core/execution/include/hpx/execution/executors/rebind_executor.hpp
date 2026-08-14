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

        using category1 = typename policy_type::execution_category;
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

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct create_rebound_policy_t final
    {
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

        template <typename ExPolicy, typename Executor>
            requires(hpx::executor_any<Executor>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Executor&& exec) const
        {
            using parameters_type =
                extract_executor_parameters_t<std::decay_t<ExPolicy>>;
            using rebound_type =
                rebind_executor_t<ExPolicy, Executor, parameters_type>;

            return rebound_type(
                HPX_FORWARD(Executor, exec), policy.parameters());
        }

        template <typename ExPolicy, typename Parameters>
            requires(hpx::executor_parameters<Parameters>)
        constexpr decltype(auto) operator()(
            ExPolicy&& policy, Parameters&& parameters) const
        {
            using executor_type = std::decay_t<ExPolicy>::executor_type;
            using rebound_type =
                rebind_executor_t<ExPolicy, executor_type, Parameters>;

            return rebound_type(
                policy.executor(), HPX_FORWARD(Parameters, parameters));
        }
    } create_rebound_policy{};
}    // namespace hpx::execution::experimental
