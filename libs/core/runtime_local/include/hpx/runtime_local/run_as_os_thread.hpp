//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/runtime_local/service_executors.hpp>

#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    hpx::future<util::invoke_result_t<F, Ts...>> run_as_os_thread(
        F&& f, Ts&&... vs)
    {
        HPX_ASSERT(threads::get_self_ptr() != nullptr);

        hpx::execution::experimental::io_pool_executor executor;
        return parallel::execution::async_execute(
            executor, HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx

namespace hpx::threads {

    template <typename F, typename... Ts>
    HPX_DEPRECATED_V(1, 10,
        "hpx::threads::run_as_os_thread is deprecated, use "
        "hpx::run_as_os_thread instead")
    decltype(auto) run_as_os_thread(F&& f, Ts&&... ts)
    {
        return hpx::run_as_os_thread(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::threads
