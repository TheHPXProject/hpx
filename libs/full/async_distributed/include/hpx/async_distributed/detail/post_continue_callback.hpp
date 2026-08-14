//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/async_distributed/detail/post_callback.hpp>
#include <hpx/async_distributed/make_continuation.hpp>
#include <hpx/modules/actions_base.hpp>

#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Cont, typename Callback,
        typename... Ts>
    bool post_continue_cb(
        Cont&& cont, id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        using action_type = typename hpx::traits::extract_action<Action>::type;
        using result_type = typename action_type::remote_result_type;
        using local_result_type = typename action_type::local_result_type;

        return hpx::post_cb<Action>(
            hpx::actions::typed_continuation<local_result_type, result_type>(
                HPX_FORWARD(Cont, cont)),
            gid, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename Callback, typename... Ts>
    bool post_continue_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        Cont&& cont, id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        return post_continue_cb<Derived>(HPX_FORWARD(Cont, cont), gid,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_continue_cb(
        id_type const& cont, id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        using action_type = typename hpx::traits::extract_action<Action>::type;
        using result_type = typename action_type::remote_result_type;
        using local_result_type = typename action_type::local_result_type;

        return hpx::post_cb<Action>(
            hpx::actions::typed_continuation<local_result_type, result_type>(
                cont, make_continuation()),
            gid, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Callback, typename... Ts>
    bool post_continue_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        id_type const& cont, id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        return post_continue_cb<Derived>(
            cont, gid, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(1, 9,
        "hpx::apply_continue_cb is deprecated, use hpx::post_continue_cb "
        "instead")
    bool apply_continue_cb(Ts&&... ts)
    {
        return post_continue_cb<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(1, 9,
        "hpx::apply_continue_cb is deprecated, use hpx::post_continue_cb "
        "instead")
    bool apply_continue_cb(Ts&&... ts)
    {
        return post_continue_cb(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx
