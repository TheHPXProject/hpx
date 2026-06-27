//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/async_continue_fwd.hpp>
#include <hpx/async_distributed/promise.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/futures.hpp>

#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Action, typename RemoteResult,
            typename Cont, typename Target, typename... Ts>
        hpx::future<typename traits::promise_local_result<
            typename result_of_async_continue<Action, Cont>::type>::type>
        async_continue_r(Cont&& cont, Target const& target, Ts&&... vs)
        {
            using result_type = traits::promise_local_result<
                typename result_of_async_continue<Action, Cont>::type>::type;

            using continuation_result_type =
                hpx::traits::extract_action<Action>::remote_result_type;

            hpx::distributed::promise<result_type, RemoteResult> p;
            auto f = p.get_future();

            hpx::post<Action>(hpx::actions::typed_continuation<result_type,
                                  continuation_result_type>(
                                  p.get_id(), HPX_FORWARD(Cont, cont)),
                target, HPX_FORWARD(Ts, vs)...);

            return f;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Cont, typename... Ts>
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Action, Cont>::type>::type>
    async_continue(Cont&& cont, hpx::id_type const& gid, Ts&&... vs)
    {
        using result_type = traits::promise_remote_result<typename detail::
                result_of_async_continue<Action, Cont>::type>::type;

        return detail::async_continue_r<Action, result_type>(
            HPX_FORWARD(Cont, cont), gid, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename... Ts>
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Derived, Cont>::type>::type>
    async_continue(hpx::actions::basic_action<Component, Signature, Derived>,
        Cont&& cont, hpx::id_type const& gid, Ts&&... vs)
    {
        return async_continue<Derived>(
            HPX_FORWARD(Cont, cont), gid, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Cont,
        typename DistPolicy, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Action, Cont>::type>::type>
    async_continue(Cont&& cont, DistPolicy const& policy, Ts&&... vs)
    {
        using result_type = traits::promise_remote_result<typename detail::
                result_of_async_continue<Action, Cont>::type>::type;

        return detail::async_continue_r<Action, result_type>(
            HPX_FORWARD(Cont, cont), policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename DistPolicy, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Derived, Cont>::type>::type>
    async_continue(hpx::actions::basic_action<Component, Signature, Derived>,
        Cont&& cont, DistPolicy const& policy, Ts&&... vs)
    {
        return async_continue<Derived>(
            HPX_FORWARD(Cont, cont), policy, HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx
