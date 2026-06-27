//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <type_traits>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename Action, typename Cont>
        struct result_of_async_continue
          : traits::action_remote_result<
                typename util::invoke_result<std::decay_t<Cont>, hpx::id_type,
                    typename hpx::traits::extract_action<
                        Action>::remote_result_type>::type>
        {
        };

        template <typename Action, typename RemoteResult, typename Cont,
            typename Target, typename... Ts>
        hpx::future<typename traits::promise_local_result<
            typename result_of_async_continue<Action, Cont>::type>::type>
        async_continue_r(Cont&& cont, Target const& target, Ts&&... vs);
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Cont, typename... Ts>
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Action, Cont>::type>::type>
    async_continue(Cont&& cont, hpx::id_type const& gid, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename... Ts>
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Derived, Cont>::type>::type>
    async_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/
        ,
        Cont&& cont, hpx::id_type const& gid, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    // MSVC complains about ambiguities if it sees this forward declaration
#ifndef HPX_MSVC
    HPX_CXX_EXPORT template <typename Action, typename Cont,
        typename DistPolicy, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Action, Cont>::type>::type>
    async_continue(Cont&& cont, DistPolicy const& policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename DistPolicy, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    hpx::future<typename traits::promise_local_result<
        typename detail::result_of_async_continue<Derived, Cont>::type>::type>
    async_continue(hpx::actions::basic_action<Component, Signature, Derived>,
        Cont&& cont, DistPolicy const& policy, Ts&&... vs);
#endif
}    // namespace hpx
