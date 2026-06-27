//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file async.hpp
/// \page hpx::async (distributed)
/// \headerfile hpx/async.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    /// \brief The distributed implementation of \c hpx::async can be used by
    ///        giving an action instance as argument instead of a function,
    ///        and also by providing another argument with the locality ID or
    ///        the target ID. The action executes asynchronously.
    ///
    /// \tparam Action The type of action instance
    /// \tparam Target The type of target where the action should be executed
    /// \tparam Ts     The type of any additional arguments
    ///
    /// \param action  The action instance to be executed
    /// \param target  The target where the action should be executed
    /// \param ts      Additional arguments
    ///
    /// \returns \c hpx::future referring to the shared state created by this call to \c hpx::async
    template <typename Action, typename Target, typename... Ts>
    HPX_FORCEINLINE auto async(Action&& action, Target&& target, Ts&&... ts);
    // clang-format on
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/async_continue.hpp>
#include <hpx/async_distributed/bind_action.hpp>
#include <hpx/async_distributed/detail/async_implementations.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action>
    struct async_action_client_dispatch
    {
        template <typename Policy, typename Client, typename Stub,
            typename Data, typename... Ts>
        HPX_FORCEINLINE std::enable_if_t<traits::is_launch_policy_v<Policy>,
            hpx::future<typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>>
        operator()(components::client_base<Client, Stub, Data> const& c,
            Policy const& launch_policy, Ts&&... ts) const
        {
            HPX_ASSERT(c.is_ready());
            return hpx::detail::async_impl<Action>(
                launch_policy, c.get_id(), HPX_FORWARD(Ts, ts)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // launch
    template <typename Action, typename Policy>
    struct async_action_dispatch<Action, Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        using result_type =
            hpx::future<typename traits::promise_local_result<typename hpx::
                    traits::extract_action<Action>::remote_result_type>::type>;

        // id_type
        template <typename Policy_, typename... Ts>
        HPX_FORCEINLINE static result_type call(
            Policy_&& launch_policy, hpx::id_type const& id, Ts&&... ts)
        {
            return hpx::detail::async_impl<Action>(
                HPX_FORWARD(Policy_, launch_policy), id,
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy_, typename Client, typename Stub,
            typename Data, typename... Ts>
        HPX_FORCEINLINE static result_type call(Policy_&& launch_policy,
            components::client_base<Client, Stub, Data> const& c, Ts&&... ts)
        {
            // make sure the action is compatible with the component type
            using component_type = components::client_base<Client, Stub,
                Data>::server_component_type;

            static_assert(traits::is_valid_action_v<Action, component_type>,
                "The action to invoke is not supported by the target");

            // invoke directly if client is ready
            if (c.is_ready())
            {
                return hpx::detail::async_impl<Action>(
                    HPX_FORWARD(Policy_, launch_policy), c.get_id(),
                    HPX_FORWARD(Ts, ts)...);
            }

            // defer invocation otherwise
            return c.then(util::one_shot(hpx::bind_back(
                async_action_client_dispatch<Action>(),
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(Ts, ts)...)));
        }

        // distribution policy
        template <typename Policy_, typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE static util::lazy_enable_if<
            traits::is_distribution_policy_v<DistPolicy>,
            typename DistPolicy::template async_result<Action>>::type
        call(Policy_&& launch_policy, DistPolicy const& policy, Ts&&... ts)
        {
            return policy.template async<Action>(
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(Ts, ts)...);
        }
    };

    // hpx::id_type
    template <typename Action>
    struct async_action_dispatch<Action, hpx::id_type>
    {
        template <typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename hpx::traits::
                    extract_action<Action>::remote_result_type>::type>
        call(hpx::id_type const& id, Ts&&... ts)
        {
            return async_action_dispatch<Action,
                hpx::launch::async_policy>::call(launch::async, id,
                HPX_FORWARD(Ts, ts)...);
        }
    };

    // component::client
    template <typename Action, typename Client>
    struct async_action_dispatch<Action, Client,
        std::enable_if_t<traits::is_client<Client>::value>>
    {
        template <typename Client_, typename Stub, typename Data,
            typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Action>::remote_result_type>::type>
        call(components::client_base<Client_, Stub, Data> const& c, Ts&&... ts)
        {
            return async_action_dispatch<Action,
                hpx::launch::async_policy>::call(launch::async, c,
                HPX_FORWARD(Ts, ts)...);
        }
    };

    // distribution policy
    template <typename Action, typename Policy>
    struct async_action_dispatch<Action, Policy,
        std::enable_if_t<traits::is_distribution_policy_v<Policy>>>
    {
        template <typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            DistPolicy const& policy, Ts&&... ts)
        {
            return async_action_dispatch<Action,
                hpx::launch::async_policy>::call(launch::async, policy,
                HPX_FORWARD(Ts, ts)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action>
    struct async_launch_policy_dispatch<Action,
        std::enable_if_t<traits::is_action<Action>::value>>
    {
        using result_type =
            traits::promise_local_result<typename hpx::traits::extract_action<
                Action>::remote_result_type>::type;

        template <typename Policy, typename... Ts>
        HPX_FORCEINLINE static auto call(
            Policy&& launch_policy, Action const&, Ts&&... ts)
            -> decltype(async<Action>(
                HPX_FORWARD(Policy, launch_policy), HPX_FORWARD(Ts, ts)...))
        {
            static_assert(traits::is_launch_policy_v<std::decay_t<Policy>>,
                "Policy must be a valid launch policy");
            return async<Action>(
                HPX_FORWARD(Policy, launch_policy), HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail

namespace hpx {

    // different versions of clang-format disagree
    // clang-format off
    HPX_CXX_EXPORT template <typename Action, typename F, typename... Ts>
    HPX_FORCEINLINE auto async(F&& f, Ts&&... ts) -> decltype(
        detail::async_action_dispatch<Action, std::decay_t<F>>::call(
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
    // clang-format on
    {
        return detail::async_action_dispatch<Action, std::decay_t<F>>::call(
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    // any action
    template <typename Action>
    struct async_dispatch<Action,
        std::enable_if_t<traits::is_action<Action>::value>>
    {
        template <typename Component, typename Signature, typename Derived,
            typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename hpx::traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(hpx::actions::basic_action<Component, Signature, Derived> const&,
            hpx::id_type const& id, Ts&&... vs)
        {
            constexpr auto priority = traits::action_priority_v<Derived>;
            constexpr auto stacksize = traits::action_stacksize_v<Derived>;
            return async<Derived>(launch::async_policy(priority, stacksize), id,
                HPX_FORWARD(Ts, vs)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename Client, typename Stub, typename Data, typename... Ts>
        HPX_FORCEINLINE static hpx::future<
            typename traits::promise_local_result<typename traits::
                    extract_action<Derived>::remote_result_type>::type>
        call(hpx::actions::basic_action<Component, Signature, Derived> const&,
            components::client_base<Client, Stub, Data> const& c, Ts&&... vs)
        {
            using component_type = components::client_base<Client, Stub,
                Data>::server_component_type;

            static_assert(traits::is_valid_action_v<Derived, component_type>,
                "The action to invoke is not supported by the target");

            constexpr auto priority = traits::action_priority_v<Derived>;
            constexpr auto stacksize = traits::action_stacksize_v<Derived>;
            return async<Derived>(launch::async_policy(priority, stacksize),
                c.get_id(), HPX_FORWARD(Ts, vs)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            DistPolicy const& policy, Ts&&... vs)
        {
            return async<Derived>(policy, HPX_FORWARD(Ts, vs)...);
        }
    };

    // launch with any action
    template <typename Func>
    struct async_dispatch_launch_policy_helper<Func,
        std::enable_if_t<traits::is_action<Func>::value>>
    {
        // clang-format off
        template <typename Policy_, typename F, typename... Ts>
        HPX_FORCEINLINE static auto call(
            Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(
                detail::async_launch_policy_dispatch<std::decay_t<F>>::call(
                    HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...))
        // clang-format on
        {
            return detail::async_launch_policy_dispatch<std::decay_t<F>>::call(
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy_, typename Component, typename Signature,
            typename Derived, typename Client, typename Stub, typename Data,
            typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(Policy_&& launch_policy,
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            components::client_base<Client, Stub, Data> const& c, Ts&&... ts)
        {
            using component_type = components::client_base<Client, Stub,
                Data>::server_component_type;

            static_assert(traits::is_valid_action_v<Derived, component_type>,
                "The action to invoke is not supported by the target");

            return async<Derived>(HPX_FORWARD(Policy_, launch_policy),
                c.get_id(), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy_, typename Component, typename Signature,
            typename Derived, typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE static util::lazy_enable_if<
            traits::is_distribution_policy_v<DistPolicy>,
            typename DistPolicy::template async_result<Derived>>::type
        call(Policy_&& launch_policy,
            hpx::actions::basic_action<Component, Signature, Derived> const&,
            DistPolicy const& policy, Ts&&... ts)
        {
            return async<Derived>(HPX_FORWARD(Policy_, launch_policy), policy,
                HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail

#include <hpx/async_distributed/sync.hpp>

namespace hpx::detail {

    // bound action
    template <typename Bound>
    struct async_dispatch<Bound,
        std::enable_if_t<hpx::is_bound_action_v<Bound>>>
    {
        template <typename Action, typename Is, typename... Ts, typename... Us>
        HPX_FORCEINLINE static hpx::future<
            typename hpx::detail::bound_action<Action, Is, Ts...>::result_type>
        call(hpx::detail::bound_action<Action, Is, Ts...> const& bound,
            Us&&... vs)
        {
            return bound.async(HPX_FORWARD(Us, vs)...);
        }
    };
}    // namespace hpx::detail

#endif
