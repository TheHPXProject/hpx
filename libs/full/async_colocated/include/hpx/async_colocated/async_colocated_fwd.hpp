//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated(hpx::id_type const& id, Ts&&... vs);

    template <typename Component, typename Signature, typename Derived,
        typename... Ts>
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated(hpx::actions::basic_action<Component, Signature, Derived>,
        hpx::id_type const& id, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    // MSVC complains about ambiguities if it sees this forward declaration
#if !defined(HPX_MSVC)
    template <typename Action, typename Continuation, typename... Ts>
        requires(traits::is_continuation_v < Continuation)
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Action>::remote_result_type>>
    async_colocated(Continuation&& cont, hpx::id_type const& id, Ts&&... vs);

    template <typename Continuation, typename Component, typename Signature,
        typename Derived, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    hpx::future<traits::promise_local_result_t<
        typename hpx::traits::extract_action<Derived>::remote_result_type>>
    async_colocated(Continuation&& cont,
        hpx::actions::basic_action<Component, Signature, Derived>,
        hpx::id_type const& id, Ts&&... vs);
#endif
}    // namespace hpx::detail
