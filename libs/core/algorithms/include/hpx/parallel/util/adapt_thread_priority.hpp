//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/executors.hpp>

namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
        requires(hpx::is_execution_policy_v<ExPolicy>)
    decltype(auto) adapt_thread_priority(
        ExPolicy&& policy, hpx::threads::thread_priority new_priority)
    {
        constexpr bool supports_priority =
            hpx::execution::experimental::has_query_v<
                std::decay_t<ExPolicy> const&,
                hpx::execution::experimental::with_priority_t,
                hpx::threads::thread_priority>;
        // clang-format on

        if constexpr (supports_priority)
        {
            hpx::threads::thread_priority priority =
                hpx::execution::experimental::get_priority(policy);

            // modify priority only if it is the default (do not overwrite user
            // supplied values)
            if (priority == hpx::threads::thread_priority::default_ &&
                new_priority != hpx::threads::thread_priority::default_)
            {
                priority = new_priority;
            }

            return hpx::execution::experimental::with_priority(
                HPX_FORWARD(ExPolicy, policy), priority);
        }
        else
        {
            return HPX_FORWARD(ExPolicy, policy);
        }
    }
}    // namespace hpx::execution::experimental

namespace hpx::parallel::util {

    template <typename ExPolicy>
        requires(hpx::is_execution_policy_v<ExPolicy>)
    HPX_DEPRECATED_V(1, 11,
        "hpx::parallel::util::adapt_thread_priority is deprecated. Please use "
        "hpx::execution::experimental::adapt_thread_priority instead.")
    decltype(auto) adapt_thread_priority(
        ExPolicy&& policy, hpx::threads::thread_priority new_priority)
    {
        return hpx::execution::experimental::adapt_thread_priority(
            HPX_FORWARD(ExPolicy, policy), new_priority);
    }
}    // namespace hpx::parallel::util
