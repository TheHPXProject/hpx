//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/async_callback_fwd.hpp>
#include <hpx/async_distributed/detail/async_implementations_fwd.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    // launch
    template <typename Action, typename Policy>
    struct async_cb_action_dispatch<Action, Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        // id_type
        template <typename Policy_, typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>
        call(Policy_&& launch_policy, hpx::id_type const& id, Callback&& cb,
            Ts&&... ts)
        {
            return hpx::detail::async_cb_impl<Action>(
                HPX_FORWARD(Policy_, launch_policy), id,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy_, typename Client, typename Stub,
            typename Data, typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>
        call(Policy_&& launch_policy,
            components::client_base<Client, Stub, Data> const& c, Callback&& cb,
            Ts&&... ts)
        {
            typedef typename components::client_base<Client, Stub,
                Data>::server_component_type component_type;

            static_assert(traits::is_valid_action_v<Action, component_type>,
                "The action to invoke is not supported by the target");

            return hpx::detail::async_cb_impl<Action>(
                HPX_FORWARD(Policy_, launch_policy), c.get_id(),
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }

        // distribution policy
        template <typename Policy_, typename DistPolicy, typename Callback,
            typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>,
            hpx::future<typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>>
        call(Policy_&& launch_policy, DistPolicy const& policy, Callback&& cb,
            Ts&&... ts)
        {
            return policy.template async_cb<Action>(
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(Callback, cb),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    // hpx::id_type
    template <typename Action>
    struct async_cb_action_dispatch<Action, hpx::id_type>
    {
        template <typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>
        call(hpx::id_type const& id, Callback&& cb, Ts&&... ts)
        {
            return async_cb_action_dispatch<Action,
                hpx::launch::async_policy>::call(launch::async, id,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }
    };

    // component::client
    template <typename Action, typename Client>
    struct async_cb_action_dispatch<Action, Client,
        std::enable_if_t<traits::is_client<Client>::value>>
    {
        template <typename Client_, typename Stub, typename Data,
            typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>
        call(components::client_base<Client_, Stub, Data> const& c,
            Callback&& cb, Ts&&... ts)
        {
            typedef typename components::client_base<Client_, Stub,
                Data>::server_component_type component_type;

            static_assert(traits::is_valid_action_v<Action, component_type>,
                "The action to invoke is not supported by the target");

            return async_cb_action_dispatch<Action,
                hpx::launch::async_policy>::call(launch::async, c.get_id(),
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }
    };

    // distribution policy
    template <typename Action, typename Policy>
    struct async_cb_action_dispatch<Action, Policy,
        std::enable_if_t<traits::is_distribution_policy_v<Policy>>>
    {
        template <typename DistPolicy, typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>
        call(DistPolicy const& policy, Callback&& cb, Ts&&... ts)
        {
            return async_cb_action_dispatch<Action,
                hpx::launch::async_policy>::call(launch::async, policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail

namespace hpx {

    // clang-format off
    HPX_CXX_EXPORT template <typename Action, typename F, typename... Ts>
    HPX_FORCEINLINE auto async_cb(F&& f, Ts&&... ts) -> decltype(
        detail::async_cb_action_dispatch<Action, std::decay_t<F>>::call(
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
    // clang-format on
    {
        return detail::async_cb_action_dispatch<Action, std::decay_t<F>>::call(
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    // any action
    template <typename Action>
    struct async_cb_dispatch<Action,
        std::enable_if_t<traits::is_action<Action>::value>>
    {
        template <typename Component, typename Signature, typename Derived,
            typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(hpx::actions::basic_action<Component, Signature, Derived> const&,
            hpx::id_type const& id, Callback&& cb, Ts&&... ts)
        {
            return async_cb<Derived>(
                id, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename Client, typename Stub, typename Data, typename Callback,
            typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(hpx::actions::basic_action<Component, Signature, Derived> const&,
            components::client_base<Client, Stub, Data> const& c, Callback&& cb,
            Ts&&... ts)
        {
            typedef typename components::client_base<Client, Stub,
                Data>::server_component_type component_type;

            static_assert(traits::is_valid_action_v<Action, component_type>,
                "The action to invoke is not supported by the target");

            return async_cb<Derived>(
                c.get_id(), HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename DistPolicy, typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(hpx::actions::basic_action<Component, Signature, Derived> const&,
            DistPolicy const& policy, Callback&& cb, Ts&&... ts)
        {
            return async_cb<Derived>(
                policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }
    };

    template <typename Policy>
    struct async_cb_dispatch<Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        template <typename Policy_, typename Component, typename Signature,
            typename Derived, typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(Policy_&& launch_policy,
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            hpx::id_type const& id, Callback&& cb, Ts&&... ts)
        {
            return async_cb<Derived>(HPX_FORWARD(Policy_, launch_policy), id,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy_, typename Component, typename Signature,
            typename Derived, typename Client, typename Stub, typename Data,
            typename Callback, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(Policy_&& launch_policy,
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            components::client_base<Client, Stub, Data> const& c, Callback&& cb,
            Ts&&... ts)
        {
            typedef typename components::client_base<Client, Stub,
                Data>::server_component_type component_type;

            static_assert(traits::is_valid_action_v<Derived, component_type>,
                "The action to invoke is not supported by the target");

            return async_cb<Derived>(HPX_FORWARD(Policy_, launch_policy),
                c.get_id(), HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy_, typename Component, typename Signature,
            typename Derived, typename DistPolicy, typename Callback,
            typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(Policy_&& launch_policy,
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            DistPolicy const& policy, Callback&& cb, Ts&&... ts)
        {
            return async_cb<Derived>(HPX_FORWARD(Policy_, launch_policy),
                policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail

namespace hpx {

    // different versions of clang-format disagree
    // clang-format off
    HPX_CXX_EXPORT template <typename F, typename... Ts>
    HPX_FORCEINLINE auto async_cb(F&& f, Ts&&... ts) -> decltype(
        detail::async_cb_dispatch< std::decay_t<F>>::call(
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
    // clang-format on
    {
        return detail::async_cb_dispatch<std::decay_t<F>>::call(
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx
