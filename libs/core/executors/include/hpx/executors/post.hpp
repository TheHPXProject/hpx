//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Define post() overloads for plain local functions and function objects.
    // dispatching trait for hpx::post

    // launch a plain function/function object
    HPX_CXX_CORE_EXPORT template <typename Func, typename Enable>
    struct post_dispatch
    {
        template <typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>, bool>
        call(F&& f, Ts&&... ts)
        {
            execution::parallel_executor exec;
            hpx::parallel::execution::post(
                exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            return false;
        }
    };

    // The overload for hpx::post taking an executor simply forwards to the
    // corresponding executor customization point.
    template <typename Executor>
    struct post_dispatch<Executor,
        std::enable_if_t<traits::is_one_way_executor_v<Executor> ||
            traits::is_two_way_executor_v<Executor>>>
    {
        template <typename Executor_, typename F, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            Executor_&& exec, F&& f, Ts&&... ts)
        {
            parallel::execution::post(HPX_FORWARD(Executor_, exec),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            return false;
        }
    };
}    // namespace hpx::detail
