//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/async_distributed/detail/post_continue_fwd.hpp>
#include <hpx/async_distributed/make_continuation.hpp>
#include <hpx/modules/actions_base.hpp>

#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Cont, typename... Ts>
    bool post_continue(Cont&& cont, id_type const& gid, Ts&&... vs)
    {
        using action_type = typename hpx::traits::extract_action<Action>::type;
        using remote_result_type = typename action_type::remote_result_type;
        using local_result_type = typename action_type::local_result_type;

        return post<Action>(hpx::actions::typed_continuation<local_result_type,
                                remote_result_type>(HPX_FORWARD(Cont, cont)),
            gid, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename... Ts>
    bool post_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        Cont&& cont, id_type const& gid, Ts&&... vs)
    {
        return post_continue<Derived>(
            HPX_FORWARD(Cont, cont), gid, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_continue(id_type const& cont, id_type const& gid, Ts&&... vs)
    {
        using action_type = typename hpx::traits::extract_action<Action>::type;
        using remote_result_type = typename action_type::remote_result_type;
        using local_result_type = typename action_type::local_result_type;

        return hpx::post<Action>(
            hpx::actions::typed_continuation<local_result_type,
                remote_result_type>(cont, make_continuation()),
            gid, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    bool post_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        id_type const& cont, id_type const& gid, Ts&&... vs)
    {
        return post_continue<Derived>(cont, gid, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(1, 9,
        "hpx::apply_continue is deprecated, use hpx::post_continue instead")
    bool apply_continue(Ts&&... ts)
    {
        return post_continue<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(1, 9,
        "hpx::apply_continue is deprecated, use hpx::post_continue instead")
    bool apply_continue(Ts&&... ts)
    {
        return post_continue(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx
