//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/agas_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/async_colocated/async_colocated_fwd.hpp>
#include <hpx/async_colocated/functional/colocated_helpers.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    template <typename Action, typename Ts = Action::arguments_type>
    struct async_colocated_bound_action;

    template <typename Action, typename... Ts>
    struct async_colocated_bound_action<Action, hpx::tuple<Ts...>>
    {
        using type = hpx::detail::bound_action<Action,
            hpx::util::make_index_pack<1 + sizeof...(Ts)>,
            hpx::detail::bound<hpx::util::functional::extract_locality,
                hpx::util::index_pack<0, 1>, hpx::detail::placeholder<2ul>,
                hpx::id_type>,
            Ts...>;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated(
        [[maybe_unused]] hpx::id_type const& id, [[maybe_unused]] Ts&&... vs)
    {
        // Attach the requested action as a continuation to a resolve_async
        // call on the locality responsible for the target gid.
        hpx::id_type service_target(
            agas::primary_namespace::get_service_instance(id.get_gid()),
            hpx::id_type::management_type::unmanaged);

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using remote_result_type =
            hpx::traits::extract_action<Action>::remote_result_type;
        using action_type = agas::server::primary_namespace::colocate_action;

        using placeholders::_2;
        return detail::async_continue_r<action_type, remote_result_type>(
            util::functional::async_continuation(hpx::bind<Action>(
                hpx::bind(util::functional::extract_locality(), _2, id),
                HPX_FORWARD(Ts, vs)...)),
            service_target, id.get_gid());
#else
        HPX_ASSERT(false);
        return hpx::future<typename traits::promise_local_result<typename hpx::
                traits::extract_action<Action>::remote_result_type>::type>{};
#endif
    }

    template <typename Component, typename Signature, typename Derived,
        typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Ts&&... vs)
    {
        return async_colocated<Derived>(id, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Continuation, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated([[maybe_unused]] Continuation&& cont,
        [[maybe_unused]] hpx::id_type const& id, [[maybe_unused]] Ts&&... vs)
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
        return detail::async_continue_r<action_type, remote_result_type>(
            util::functional::async_continuation(
                hpx::bind<Action>(
                    hpx::bind(util::functional::extract_locality(), _2, id),
                    HPX_FORWARD(Ts, vs)...),
                HPX_FORWARD(Continuation, cont)),
            service_target, id.get_gid());
#endif
    }

    template <typename Continuation, typename Component, typename Signature,
        typename Derived, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated(Continuation&& cont,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Ts&&... vs)
    {
        return async_colocated<Derived>(
            HPX_FORWARD(Continuation, cont), id, HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx::detail
