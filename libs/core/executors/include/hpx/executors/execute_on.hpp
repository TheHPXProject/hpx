//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/executors/execution_policy.hpp>
#include <hpx/executors/explicit_scheduler_executor.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Scheduler,
            typename Enable = void>
        struct exposes_policy_aware_scheduler_types : std::false_type
        {
        };

        template <typename Scheduler>
        struct exposes_policy_aware_scheduler_types<Scheduler,
            std::void_t<typename Scheduler::policy_type,
                typename Scheduler::base_scheduler_type>> : std::true_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename Scheduler,
            typename Enable = void>
        struct exposes_get_policy : std::false_type
        {
        };

        // clang-format off
        template <typename Scheduler>
        struct exposes_get_policy<Scheduler,
            std::enable_if_t<hpx::is_execution_policy_v<
                decltype(std::declval<Scheduler>().get_policy())>>>
          : std::true_type
        {
        };
        // clang-format on
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Scheduler, typename ExPolicy>
    struct scheduler_and_policy : std::decay_t<Scheduler>
    {
        using base_scheduler_type = std::decay_t<Scheduler>;
        using policy_type = std::decay_t<ExPolicy>;

        scheduler_and_policy(scheduler_and_policy const&) = default;
        scheduler_and_policy(scheduler_and_policy&&) noexcept = default;
        scheduler_and_policy& operator=(scheduler_and_policy const&) = default;
        scheduler_and_policy& operator=(
            scheduler_and_policy&&) noexcept = default;
        ~scheduler_and_policy() = default;

        template <typename Scheduler_, typename ExPolicy_>
        scheduler_and_policy(Scheduler_&& sched, ExPolicy_&& policy)
          : base_scheduler_type(HPX_FORWARD(Scheduler_, sched))
          , policy(HPX_FORWARD(ExPolicy_, policy))
        {
        }

        constexpr policy_type const& get_policy() const noexcept
        {
            return policy;
        }

        constexpr base_scheduler_type const& get_scheduler() const noexcept
        {
            return static_cast<base_scheduler_type const&>(*this);
        }

        // Needed for this to be a scheduler under the p2300 definition
        constexpr typename Scheduler::template sender<scheduler_and_policy>
        schedule() const
        {
            return {*this};
        }

        policy_type policy;

        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag>)
        [[nodiscard]] auto query(Tag tag, Property&& prop) const
        {
            return scheduler_and_policy{
                get_scheduler().query(tag, HPX_FORWARD(Property, prop)),
                get_policy()};
        }

        template <typename Tag>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag>)
        [[nodiscard]] auto query(Tag tag) const
        {
            return get_scheduler().query(tag);
        }

        template <typename Tag, typename... Args>
            requires(
                !hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                requires(base_scheduler_type const& sched, Tag t, Args... a) {
                    sched.query(t, HPX_FORWARD(Args, a)...);
                })
        [[nodiscard]] auto query(Tag tag, Args&&... args) const
        {
            return get_scheduler().query(tag, HPX_FORWARD(Args, args)...);
        }

        template <typename Tag>
            requires(
                !hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                requires(base_scheduler_type const& sched, Tag t) {
                    sched.query(t);
                })
        [[nodiscard]] auto query(Tag tag) const
        {
            return get_scheduler().query(tag);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Scheduler, typename ExPolicy>
    scheduler_and_policy(Scheduler&&, ExPolicy&&)
        -> scheduler_and_policy<std::decay_t<Scheduler>,
            std::decay_t<ExPolicy>>;

    ////////////////////////////////////////////////////////////////////////////

    // The scheduling property CPOs detect the public query() member functions
    // of scheduler_and_policy directly (via property_base), so no tag_invoke
    // bridge is needed here.

    // Experimental support for facilities from p2500 (wg21.link/p2500)
    inline namespace p2500 {

        ///////////////////////////////////////////////////////////////////////
        // policy_aware_scheduler is a concept for parallel algorithms that
        // represents a combined scheduler and execution_policy entity. It
        // allows to get both execution policy type and execution policy object
        // parallel algorithm is called with.
        //
        // Customizations of the parallel algorithms can reuse the existing
        // implementation of parallel algorithms with ExecutionPolicy template
        // parameter for "known" base_scheduler_type type.
        HPX_CXX_CORE_EXPORT template <typename Scheduler,
            typename Enable = void>
        struct is_policy_aware_scheduler : std::false_type
        {
        };

        template <typename Scheduler>
        struct is_policy_aware_scheduler<Scheduler,
            std::enable_if_t<is_scheduler_v<Scheduler> &&
                detail::exposes_policy_aware_scheduler_types<
                    std::decay_t<Scheduler>>::value &&
                detail::exposes_get_policy<Scheduler>::value>> : std::true_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename Scheduler>
        inline constexpr bool is_policy_aware_scheduler_v =
            is_policy_aware_scheduler<Scheduler>::value;

        ///////////////////////////////////////////////////////////////////////
        // execute_on is the customization point that serves the purpose to tie
        // scheduler and execution_policy.
        //
        // It's up to scheduler customization to check if it can work with the
        // passed execution policy.
        HPX_CXX_CORE_EXPORT inline constexpr struct execute_on_t final
        {
            template <typename Scheduler, execution_policy ExPolicy>
                requires(hpx::execution::experimental::is_scheduler_v<
                    std::decay_t<Scheduler>>)
            constexpr HPX_FORCEINLINE auto operator()(
                Scheduler&& scheduler, ExPolicy&& policy) const
            {
                return scheduler_and_policy(HPX_FORWARD(Scheduler, scheduler),
                    HPX_FORWARD(ExPolicy, policy));
            }
        } execute_on{};
    }    // namespace p2500
}    // namespace hpx::execution::experimental
