//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/detail/post_implementations_fwd.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_impl(Continuation&& c, hpx::id_type const& id, hpx::launch policy,
        Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        if (!traits::action_is_target_valid<action_type>::call(id))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::detail::post_impl",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<action_type>());
        }

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
                !r.first)
            {
                return hpx::detail::post_l_p<action_type>(
                    HPX_FORWARD(Continuation, c), id, HPX_MOVE(addr), policy,
                    HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr))
            {
                return hpx::detail::post_l_p<action_type>(
                    HPX_FORWARD(Continuation, c), id, HPX_MOVE(addr), policy,
                    HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }

#if defined(HPX_HAVE_NETWORKING)
        // Note: the pinned_ptr is still being held, if necessary

        // apply remotely
        return hpx::detail::post_r_p<action_type>(HPX_MOVE(addr),
            HPX_FORWARD(Continuation, c), id, policy, HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "hpx::post_impl",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_impl(Continuation&& c, hpx::id_type const& id,
        naming::address&& addr, hpx::launch policy, Ts&&... vs)
    {
        if (!addr)
        {
            return post_impl<Action>(HPX_FORWARD(Continuation, c), id, policy,
                HPX_FORWARD(Ts, vs)...);
        }

        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        // Determine whether the id is local or remote
        if (!traits::action_is_target_valid<action_type>::call(id))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::detail::post_impl",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<action_type>());
        }

        if (naming::get_locality_id_from_gid(addr.locality_) ==
            agas::get_locality_id())
        {
            if constexpr (traits::component_supports_migration<
                              component_type>::call())
            {
                HPX_ASSERT(
                    !traits::action_was_object_migrated<action_type>::call(
                        id, addr.address_)
                        .first);
                HPX_ASSERT(pin_count_is_valid<component_type>(addr.address_));
            }

            return hpx::detail::post_l_p<action_type>(
                HPX_FORWARD(Continuation, c), id, HPX_MOVE(addr), policy,
                HPX_FORWARD(Ts, vs)...);
        }

#if defined(HPX_HAVE_NETWORKING)
        // object was migrated or is not local, apply remotely
        return hpx::detail::post_r_p<action_type>(HPX_MOVE(addr),
            HPX_FORWARD(Continuation, c), id, policy, HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
            "hpx::detail::post_impl",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_impl(hpx::id_type const& id, hpx::launch policy, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        if (!traits::action_is_target_valid<action_type>::call(id))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::detail::post_impl",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<action_type>());
        }

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
                !r.first)
            {
                return hpx::detail::post_l_p<action_type>(
                    id, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr))
            {
                return hpx::detail::post_l_p<action_type>(
                    id, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }

#if defined(HPX_HAVE_NETWORKING)
        // Note: the pinned_ptr is still being held, if necessary

        // object was migrated or is not local, apply remotely
        return hpx::detail::post_r_p<Action>(
            HPX_MOVE(addr), id, policy, HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "hpx::post_impl",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_impl(hpx::id_type const& id, naming::address&& addr,
        hpx::launch policy, Ts&&... vs)
    {
        if (!addr)
        {
            return post_impl<Action>(id, policy, HPX_FORWARD(Ts, vs)...);
        }

        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        // Determine whether the id is local or remote
        if (!traits::action_is_target_valid<action_type>::call(id))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::detail::post_impl",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<action_type>());
        }

        if (naming::get_locality_id_from_gid(addr.locality_) ==
            agas::get_locality_id())
        {
            if constexpr (traits::component_supports_migration<
                              component_type>::call())
            {
                HPX_ASSERT(
                    !traits::action_was_object_migrated<action_type>::call(
                        id, addr.address_)
                        .first);
                HPX_ASSERT(pin_count_is_valid<component_type>(addr.address_));
            }

            return hpx::detail::post_l_p<action_type>(
                id, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);
        }

#if defined(HPX_HAVE_NETWORKING)
        // object was migrated or is not local, apply remotely
        return hpx::detail::post_r_p<action_type>(
            HPX_MOVE(addr), id, policy, HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
            "hpx::detail::post_impl",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_cb_impl(Continuation&& c, hpx::id_type const& id,
        hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        if (!traits::action_is_target_valid<action_type>::call(id))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::detail::post_cb_impl",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<action_type>());
        }

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
                !r.first)
            {
                bool const result = hpx::detail::post_l_p<action_type>(
                    HPX_FORWARD(Continuation, c), id, HPX_MOVE(addr), policy,
                    HPX_FORWARD(Ts, vs)...);

                invoke_callback(HPX_FORWARD(Callback, cb));
                return result;
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr))
            {
                bool const result = hpx::detail::post_l_p<action_type>(
                    HPX_FORWARD(Continuation, c), id, HPX_MOVE(addr), policy,
                    HPX_FORWARD(Ts, vs)...);

                invoke_callback(HPX_FORWARD(Callback, cb));
                return result;
            }

            // fall through
        }

#if defined(HPX_HAVE_NETWORKING)
        // Note: the pinned_ptr is still being held, if necessary

        // object was migrated or is not local, apply remotely
        return hpx::detail::post_r_p_cb<action_type>(HPX_MOVE(addr),
            HPX_FORWARD(Continuation, c), id, policy, HPX_FORWARD(Callback, cb),
            HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
            "hpx::detail::post_cb_impl",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_cb_impl(
        hpx::id_type const& id, hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = typename action_type::component_type;

        if (!traits::action_is_target_valid<action_type>::call(id))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::detail::post_cb_impl",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<action_type>());
        }

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
                !r.first)
            {
                bool const result = hpx::detail::post_l_p<action_type>(
                    id, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);

                invoke_callback(HPX_FORWARD(Callback, cb));
                return result;
            }

            // fall through
        }
        else
        {
            // Determine whether the id is local or remote
            if (agas::is_local_address_cached(id, addr))
            {
                bool const result = hpx::detail::post_l_p<action_type>(
                    id, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);

                invoke_callback(HPX_FORWARD(Callback, cb));
                return result;
            }

            // fall through
        }

#if defined(HPX_HAVE_NETWORKING)
        // Note: the pinned_ptr is still being held, if necessary

        // object was migrated or is not local, apply remotely
        return hpx::detail::post_r_p_cb<action_type>(HPX_MOVE(addr), id, policy,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
            "hpx::detail::post_cb_impl",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }
}    // namespace hpx::detail
