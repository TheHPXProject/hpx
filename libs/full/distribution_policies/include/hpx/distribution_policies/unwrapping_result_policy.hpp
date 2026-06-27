//  Copyright (c) 2014-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file unwrapping_result_policy.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx::components {

    /// This class is a distribution policy that can be used with actions that
    /// return futures. For those actions it is possible to apply certain
    /// optimizations if the action is invoked synchronously.
    HPX_CXX_EXPORT struct unwrapping_result_policy
    {
    public:
        explicit unwrapping_result_policy(id_type const& id)
          : id_(id)
        {
        }

        template <typename Client, typename Stub, typename Data>
        explicit unwrapping_result_policy(
            client_base<Client, Stub, Data> const& client)
          : id_(client.get_id())
        {
        }

        template <typename Action>
        struct async_result
        {
            using type = typename traits::promise_local_result<typename hpx::
                    traits::extract_action<Action>::remote_result_type>::type;
        };

        template <typename Action, typename... Ts>
        HPX_FORCEINLINE typename async_result<Action>::type async(
            launch policy, Ts&&... vs) const
        {
            return hpx::detail::async_unwrap_result_impl<Action>(
                policy, get_next_target(), HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename... Ts>
        HPX_FORCEINLINE typename async_result<Action>::type async(
            launch::sync_policy, Ts&&... vs) const
        {
            return hpx::detail::sync_impl<Action>(
                launch::sync, get_next_target(), HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename Callback, typename... Ts>
        typename async_result<Action>::type async_cb(
            launch policy, Callback&& cb, Ts&&... vs) const
        {
            return hpx::detail::async_cb_impl<Action>(policy, get_next_target(),
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Continuation, typename... Ts>
        bool apply(Continuation&& c, launch policy, Ts&&... vs) const
        {
            return hpx::detail::post_impl<Action>(HPX_FORWARD(Continuation, c),
                get_next_target(), policy, HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename... Ts>
        bool apply(launch policy, Ts&&... vs) const
        {
            return hpx::detail::post_impl<Action>(
                get_next_target(), policy, HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Continuation, typename Callback,
            typename... Ts>
        bool apply_cb(
            Continuation&& c, launch policy, Callback&& cb, Ts&&... vs) const
        {
            return hpx::detail::post_cb_impl<Action>(
                HPX_FORWARD(Continuation, c), get_next_target(), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename Callback, typename... Ts>
        bool apply_cb(launch policy, Callback&& cb, Ts&&... vs) const
        {
            return hpx::detail::post_cb_impl<Action>(get_next_target(), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        hpx::id_type const& get_next_target() const
        {
            return id_;
        }

    protected:
        /// \cond NOINTERNAL
        hpx::id_type const& id_;    // locality to encapsulate
        /// \endcond
    };
}    // namespace hpx::components

/// \cond NOINTERNAL
namespace hpx {

    HPX_CXX_EXPORT using unwrap_result =
        hpx::components::unwrapping_result_policy;

    template <>
    struct traits::is_distribution_policy<components::unwrapping_result_policy>
      : std::true_type
    {
    };    // namespace traits
}    // namespace hpx
/// \endcond
