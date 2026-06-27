//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file dataflow.hpp
/// \page hpx::dataflow (distributed)
/// \headerfile hpx/async.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    /// \brief The distributed implementation of \c hpx::dataflow can be used by
    ///        giving an action instance as argument instead of a function,
    ///        and also by providing another argument with the locality ID or
    ///        the target ID. The action executes asynchronously.
    ///
    /// \note Its behavior is similar to \c hpx::async with the exception that if
    ///       one of the arguments is a future, then \c hpx::dataflow will wait
    ///       for the future to be ready to launch the thread.
    ///
    /// \tparam Action The type of action instance
    /// \tparam Target The type of target where the action should be executed
    /// \tparam Ts     The type of any additional arguments
    ///
    /// \param action  The action instance to be executed
    /// \param target  The target where the action should be executed
    /// \param ts      Additional arguments
    ///
    /// \returns \c hpx::future referring to the shared state created by this call
    ///          to \c hpx::dataflow
    template <typename Action, typename Target, typename... Ts>
    decltype(auto) dataflow(Action&& action, Target&& target, Ts&&... ts);
    // clang-format on
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    template <typename Policy, typename Action, typename Args>
    struct dataflow_return_impl</*IsAction=*/true, Policy, Action, Args>
    {
        using type = hpx::future<typename Action::result_type>;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Policy>
    struct dataflow_dispatch_impl<true, Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        template <typename Allocator, typename Policy_, typename Component,
            typename Signature, typename Derived, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(Allocator const& alloc,
            Policy_&& policy,
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            hpx::id_type const& id, Ts&&... ts)
        {
            return detail::create_dataflow(alloc, HPX_FORWARD(Policy_, policy),
                Derived{}, id,
                traits::acquire_future_disp()(HPX_FORWARD(Ts, ts))...);
        }
    };

    template <typename FD>
    struct dataflow_dispatch_impl<true, FD,
        std::enable_if_t<!traits::is_launch_policy_v<FD> &&
            !traits::is_one_way_executor_v<FD> &&
            !traits::is_two_way_executor_v<FD>>>
    {
        template <typename Allocator, typename Component, typename Signature,
            typename Derived, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(Allocator const& alloc,
            hpx::actions::basic_action<Component, Signature, Derived> const&
                act,
            hpx::id_type const& id, Ts&&... ts)
        {
            return dataflow_dispatch_impl<true, launch>::call(
                alloc, launch::async, act, id, HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::lcos::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    template <typename Action, typename F, typename... Ts>
        requires(traits::is_action_v<Action> && !traits::is_launch_policy_v<F>)
    HPX_DEPRECATED_V(1, 9,
        "hpx::dataflow<Action>(...) is deprecated, use hpx::dataflow(Action{}, "
        "...) instead")
    // clang-format on
    decltype(auto) dataflow(F&& f, Ts&&... ts)
    {
        return hpx::detail::dataflow(hpx::util::internal_allocator<>{},
            hpx::launch::async, Action{}, HPX_FORWARD(F, f),
            HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename F, typename... Ts>
        requires(traits::is_action_v<Action> && traits::is_launch_policy_v<F>)
    HPX_DEPRECATED_V(1, 9,
        "hpx::dataflow<Action>(policy, ...) is deprecated, use "
        "hpx::dataflow(policy, Action{}, ...) instead")
    // clang-format on
    decltype(auto) dataflow(F&& f, Ts&&... ts)
    {
        return hpx::detail::dataflow(hpx::util::internal_allocator<>{},
            HPX_FORWARD(F, f), Action{}, HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx

#endif
