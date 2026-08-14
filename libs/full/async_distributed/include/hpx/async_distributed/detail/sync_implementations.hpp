//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/async_implementations.hpp>
#include <hpx/async_distributed/detail/sync_implementations_fwd.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>

#include <utility>

namespace hpx::detail {
    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Result>
    struct sync_local_invoke_direct
    {
        template <typename... Ts>
        HPX_FORCEINLINE static Result call(
            hpx::id_type const& /*id*/, naming::address&& addr, Ts&&... vs)
        {
            using remote_result_type = typename Action::remote_result_type;
            using get_remote_result_type =
                traits::get_remote_result<Result, remote_result_type>;

            return get_remote_result_type::call(Action::execute_function(
                addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...));
        }
    };

    template <typename Action>
    struct sync_local_invoke_direct<Action, void>
    {
        template <typename... Ts>
        HPX_FORCEINLINE static void call(
            hpx::id_type const& /*id*/, naming::address&& addr, Ts&&... vs)
        {
            Action::execute_function(
                addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Launch, typename... Ts>
    hpx::traits::extract_action_t<Action>::local_result_type sync_impl(
        Launch&& policy, hpx::id_type const& id, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = typename action_type::local_result_type;
        using component_type = typename action_type::component_type;

        [[maybe_unused]] std::pair<bool, components::pinned_ptr> r;
        naming::address addr;

        if constexpr (traits::component_supports_migration<
                          component_type>::call())
        {
            auto f = [id](naming::address const& addr) {
                return traits::action_was_object_migrated<Action>::call(
                    id, addr.address_);
            };

            if (agas::is_local_address_cached(id, addr, r, HPX_MOVE(f)) &&
                can_invoke_locally<action_type>())
            {
                // route launch policy through component
                launch const adapted_policy =
                    traits::action_select_direct_execution<Action>::call(
                        policy, addr.address_);

                if (!r.first &&
                    (adapted_policy == launch::sync ||
                        action_type::direct_execution::value))
                {
                    return hpx::detail::sync_local_invoke_direct<action_type,
                        result_type>::call(id, HPX_MOVE(addr),
                        HPX_FORWARD(Ts, vs)...);
                }

                // fall through
            }
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr) &&
                can_invoke_locally<action_type>())
            {
                // route launch policy through component
                launch const adapted_policy =
                    traits::action_select_direct_execution<Action>::call(
                        policy, addr.address_);

                if (adapted_policy == launch::sync ||
                    action_type::direct_execution::value)
                {
                    return hpx::detail::sync_local_invoke_direct<action_type,
                        result_type>::call(id, HPX_MOVE(addr),
                        HPX_FORWARD(Ts, vs)...);
                }

                // fall through
            }
        }

        // Note: the pinned_ptr is still being held, if necessary
        return async_remote_impl<Action>(HPX_FORWARD(Launch, policy), id,
            HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...)
            .get();
    }
    /// \endcond
}    // namespace hpx::detail
