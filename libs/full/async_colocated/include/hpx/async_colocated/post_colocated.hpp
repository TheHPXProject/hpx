//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/agas_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/async_colocated/functional/colocated_helpers.hpp>
#include <hpx/async_colocated/post_colocated_fwd.hpp>
#include <hpx/async_colocated/register_post_colocated.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_colocated(hpx::id_type const& id, Ts&&... vs)
    {
        // shortcut co-location code if target already is a locality
        if (naming::is_locality(id))
        {
            return post<Action>(id, HPX_FORWARD(Ts, vs)...);
        }

        // Attach the requested action as a continuation to a resolve_async
        // call on the locality responsible for the target gid.
        hpx::id_type service_target(
            agas::primary_namespace::get_service_instance(id.get_gid()),
            hpx::id_type::management_type::unmanaged);

        using action_type = agas::server::primary_namespace::colocate_action;

        using placeholders::_2;
        return post_continue<action_type>(
            util::functional::post_continuation(hpx::bind<Action>(
                hpx::bind(util::functional::extract_locality(), _2, id),
                HPX_FORWARD(Ts, vs)...)),
            service_target, id.get_gid());
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    bool post_colocated(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Ts&&... vs)
    {
        return post_colocated<Derived>(id, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_colocated(Continuation&& cont, hpx::id_type const& id, Ts&&... vs)
    {
        // shortcut co-location code if target already is a locality
        if (naming::is_locality(id))
        {
            return post_continue<Action>(
                HPX_FORWARD(Continuation, cont), id, HPX_FORWARD(Ts, vs)...);
        }

        // Attach the requested action as a continuation to a resolve_async
        // call on the locality responsible for the target gid.
        hpx::id_type service_target(
            agas::primary_namespace::get_service_instance(id.get_gid()),
            hpx::id_type::management_type::unmanaged);

        using action_type = agas::server::primary_namespace::colocate_action;

        using placeholders::_2;
        return post_continue<action_type>(
            util::functional::post_continuation(
                hpx::bind<Action>(
                    hpx::bind(util::functional::extract_locality(), _2, id),
                    HPX_FORWARD(Ts, vs)...),
                HPX_FORWARD(Continuation, cont)),
            service_target, id.get_gid());
    }

    HPX_CXX_EXPORT template <typename Continuation, typename Component,
        typename Signature, typename Derived, typename... Ts>
    bool post_colocated(Continuation&& cont,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Ts&&... vs)
    {
        return post_colocated<Derived>(
            HPX_FORWARD(Continuation, cont), id, HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx::detail

#endif
