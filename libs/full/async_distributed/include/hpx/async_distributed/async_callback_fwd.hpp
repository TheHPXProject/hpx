//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // dispatch point used for async_cb implementations
        template <typename Func, typename Enable = void>
        struct async_cb_dispatch;

        // dispatch point used for async_cb<Action> implementations
        template <typename Action, typename Func, typename Enable = void>
        struct async_cb_action_dispatch;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // clang-format off
    HPX_CXX_EXPORT template <typename Action, typename F, typename... Ts>
    HPX_FORCEINLINE auto async_cb(F&& f, Ts&&... ts)
        -> decltype(detail::async_cb_action_dispatch<Action,
            std::decay_t<F>>::call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
    // clang-format on
}    // namespace hpx
