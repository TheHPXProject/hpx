//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/threading_base.hpp>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    void call_async(threads::thread_init_data&& data,
        hpx::id_type const& target, naming::address::address_type lva,
        naming::address::component_type comptype,
        threads::thread_priority priority, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
    void call_async(threads::thread_init_data&& data, Continuation&& cont,
        hpx::id_type const& target, naming::address::address_type lva,
        naming::address::component_type comptype,
        threads::thread_priority priority, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    HPX_FORCEINLINE void call_sync(naming::address::address_type lva,
        naming::address::component_type comptype, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
    HPX_FORCEINLINE void call_sync(Continuation&& cont,
        naming::address::address_type lva,
        naming::address::component_type comptype, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action,
        bool DirectExecute = Action::direct_execution::value>
    struct post_helper;
}    // namespace hpx::detail
