//  Copyright (c) 2017-2026 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/detail/execution_parameter_callbacks.hpp>
#include <hpx/execution/traits/executor_traits.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // define member traits
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(has_pending_closures)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(get_pu_mask)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(set_scheduler_mode)
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // define customization points

    /// Retrieve whether this executor has operations pending or not.
    ///
    /// \param exec  [in] The executor object to use to extract the
    ///              requested information for.
    ///
    /// \note If the executor does not expose this information, this call
    ///       will always return \a false
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct has_pending_closures_t final
    {
        // Primary: forward to member function if available
        template <typename Executor>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                detail::has_has_pending_closures_v<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& exec) const
        {
            return HPX_FORWARD(Executor, exec).has_pending_closures();
        }

        // Fallback: assume stateless scheduling
        template <typename Executor>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                !detail::has_has_pending_closures_v<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& /*exec*/) const
        {
            return false;
        }
    } has_pending_closures{};

    /// Retrieve the bitmask describing the processing units the given
    /// thread is allowed to run on
    ///
    /// All threads::executors invoke sched.get_pu_mask().
    ///
    /// \param exec  [in] The executor object to use for querying the
    ///              number of pending tasks.
    /// \param topo  [in] The topology object to use to extract the
    ///              requested information.
    /// \param thream_num [in] The sequence number of the thread to
    ///              retrieve information for.
    ///
    /// \note If the executor does not support this operation, this call
    ///       will always invoke hpx::threads::get_pu_mask()
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct get_pu_mask_t final
    {
        // Primary: forward to member function if available
        template <typename Executor>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                detail::has_get_pu_mask_v<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& exec,
            threads::topology& topo, std::size_t thread_num) const
        {
            return HPX_FORWARD(Executor, exec).get_pu_mask(topo, thread_num);
        }

        // Fallback: use default implementation
        template <typename Executor>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                !detail::has_get_pu_mask_v<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& /*exec*/,
            threads::topology& topo, std::size_t thread_num) const
        {
            return hpx::parallel::execution::detail::get_pu_mask(
                topo, thread_num);
        }
    } get_pu_mask{};

    /// Set various modes of operation on the scheduler underneath the
    /// given executor.
    ///
    /// \param exec     [in] The executor object to use.
    /// \param mode     [in] The new mode for the scheduler to pick up
    ///
    /// \note This calls exec.set_scheduler_mode(mode) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct set_scheduler_mode_t final
    {
        // Primary: forward to member function if available
        template <typename Executor, typename Mode>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                detail::has_set_scheduler_mode_v<Executor>)
        HPX_FORCEINLINE void operator()(Executor&& exec, Mode const& mode) const
        {
            HPX_FORWARD(Executor, exec).set_scheduler_mode(mode);
        }

        // Fallback: no-op
        template <typename Executor, typename Mode>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                !detail::has_set_scheduler_mode_v<Executor>)
        HPX_FORCEINLINE void operator()(
            Executor&& /*exec*/, Mode const& /*mode*/) const
        {
        }
    } set_scheduler_mode{};
}    // namespace hpx::execution::experimental
