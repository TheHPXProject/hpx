//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/detail/async_implementations.hpp>
#include <hpx/async_distributed/detail/async_unwrap_result_implementations_fwd.hpp>
#include <hpx/async_distributed/detail/sync_implementations.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>

#include <utility>

namespace hpx::detail {

    /// \cond NOINTERNAL
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::traits::extract_action_t<Action>::local_result_type
    async_local_unwrap_impl(launch policy, hpx::id_type const& id,
        naming::address& addr, std::pair<bool, components::pinned_ptr>& r,
        Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = typename action_type::local_result_type;

        if (policy == launch::sync || action_type::direct_execution::value)
        {
            return hpx::detail::sync_local_invoke_direct<action_type,
                result_type>::call(id, HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
        }

        if (hpx::has_async_policy(policy))
        {
            return keep_alive(
                hpx::async(policy, action_invoker<action_type>(), addr.address_,
                    addr.type_, HPX_FORWARD(Ts, vs)...),
                id, HPX_MOVE(r.second));
        }

        HPX_ASSERT(policy == launch::deferred);

        return keep_alive(
            hpx::async(launch::deferred, action_invoker<action_type>(),
                addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...),
            id, HPX_MOVE(r.second));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Launch, typename... Ts>
    hpx::traits::extract_action_t<Action>::local_result_type
    async_unwrap_result_impl(
        Launch&& policy, hpx::id_type const& id, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        [[maybe_unused]] std::pair<bool, components::pinned_ptr> r;
        naming::address addr;

        if constexpr (traits::component_supports_migration<
                          component_type>::call())
        {
            auto f = [id](naming::address const& addr) {
                return traits::action_was_object_migrated<action_type>::call(
                    id, addr.address_);
            };

            if (agas::is_local_address_cached(id, addr, r, HPX_MOVE(f)) &&
                can_invoke_locally<action_type>() && !r.first)
            {
                // route launch policy through component
                launch const adapted_policy =
                    traits::action_select_direct_execution<action_type>::call(
                        policy, addr.address_);

                return async_local_unwrap_impl<Action>(
                    adapted_policy, id, addr, r, HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr) &&
                can_invoke_locally<action_type>())
            {
                // route launch policy through component
                launch const adapted_policy =
                    traits::action_select_direct_execution<action_type>::call(
                        policy, addr.address_);

                return async_local_unwrap_impl<Action>(
                    adapted_policy, id, addr, r, HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }

        // Note: the pinned_ptr is still being held, if necessary

        // the asynchronous result is auto-unwrapped by the return type
        return async_remote_impl<Action>(HPX_FORWARD(Launch, policy), id,
            HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
    }
    /// \endcond
}    // namespace hpx::detail
