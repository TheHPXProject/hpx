//  Copyright (c) 2015-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <utility>

namespace hpx::components::server {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // simple utility action which invoke an arbitrary function
        template <typename F, typename... Ts>
        struct invoke_function
        {
            static util::invoke_result_t<F, Ts...> call(F f, Ts... ts)
            {
                return f(HPX_MOVE(ts)...);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // action definition exposing invoke_function<>
    HPX_CXX_EXPORT template <typename F, typename... Ts>
    struct invoke_function_action
      : ::hpx::actions::action<util::invoke_result_t<F, Ts...> (*)(F, Ts...),
            &detail::invoke_function<F, Ts...>::call,
            invoke_function_action<F, Ts...>>
    {
    };
}    // namespace hpx::components::server
