//  Copyright (c) 2007-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/agas_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/futures.hpp>

#include <hpx/async_colocated/async_colocated.hpp>
#include <hpx/async_colocated/async_colocated_callback_fwd.hpp>

#include <utility>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated_cb([[maybe_unused]] hpx::id_type const& id,
        [[maybe_unused]] Callback&& cb, [[maybe_unused]] Ts&&... vs)
    {
#if defined(HPX_COMPUTE_DEVICE_CODE)
        HPX_ASSERT(false);
#else
        // Attach the requested action as a continuation to a resolve_async
        // call on the locality responsible for the target gid.
        hpx::id_type service_target(
            agas::primary_namespace::get_service_instance(id.get_gid()),
            hpx::id_type::management_type::unmanaged);

        using remote_result_type =
            hpx::traits::extract_action<Action>::remote_result_type;
        using action_type = agas::server::primary_namespace::colocate_action;

        using placeholders::_2;
        return detail::async_continue_r_cb<action_type, remote_result_type>(
            util::functional::async_continuation(hpx::bind<Action>(
                hpx::bind(util::functional::extract_locality(), _2, id),
                HPX_FORWARD(Ts, vs)...)),
            service_target, HPX_FORWARD(Callback, cb), id.get_gid());
#endif
    }

    template <typename Component, typename Signature, typename Derived,
        typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Callback&& cb, Ts&&... vs)
    {
        return async_colocated_cb<Derived>(
            id, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Continuation, typename Callback,
        typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated_cb([[maybe_unused]] Continuation&& cont,
        [[maybe_unused]] hpx::id_type const& id, [[maybe_unused]] Callback&& cb,
        [[maybe_unused]] Ts&&... vs)
    {
#if defined(HPX_COMPUTE_DEVICE_CODE)
        HPX_ASSERT(false);
#else
        // Attach the requested action as a continuation to a resolve_async
        // call on the locality responsible for the target gid.
        hpx::id_type service_target(
            agas::primary_namespace::get_service_instance(id.get_gid()),
            hpx::id_type::management_type::unmanaged);

        using remote_result_type =
            hpx::traits::extract_action<Action>::remote_result_type;
        using action_type = agas::server::primary_namespace::colocate_action;

        using placeholders::_2;
        return detail::async_continue_r_cb<action_type, remote_result_type>(
            util::functional::async_continuation(
                hpx::bind<Action>(
                    hpx::bind(util::functional::extract_locality(), _2, id),
                    HPX_FORWARD(Ts, vs)...),
                HPX_FORWARD(Continuation, cont)),
            service_target, HPX_FORWARD(Callback, cb), id.get_gid());
#endif
    }

    template <typename Continuation, typename Component, typename Signature,
        typename Derived, typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated_cb(Continuation&& cont,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Callback&& cb, Ts&&... vs)
    {
        return async_colocated_cb<Derived>(HPX_FORWARD(Continuation, cont), id,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx::detail
