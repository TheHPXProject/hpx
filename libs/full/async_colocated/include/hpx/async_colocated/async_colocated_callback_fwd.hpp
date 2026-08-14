//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/futures.hpp>

#include <hpx/async_colocated/async_colocated_fwd.hpp>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated_cb(hpx::id_type const& id, Callback&& cb, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Callback&& cb, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated_cb(
        Continuation&& cont, hpx::id_type const& id, Callback&& cb, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Continuation, typename Component,
        typename Signature, typename Derived, typename Callback, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated_cb(Continuation&& cont,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& id, Callback&& cb, Ts&&... vs);
}    // namespace hpx::detail
