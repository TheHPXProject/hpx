//  Copyright (c) 2022-2025 Hartmut Kaiser
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
    decltype(auto) adapt_sharing_mode(
        ExPolicy&& policy, hpx::threads::thread_sharing_hint sharing)
    {
        constexpr bool supports_sharing_hint =
            hpx::execution::experimental::has_query_v<
                std::decay_t<ExPolicy> const&,
                hpx::execution::experimental::with_hint_t,
                hpx::threads::thread_schedule_hint>;

        if constexpr (supports_sharing_hint)
        {
            hpx::threads::thread_schedule_hint hint =
                hpx::execution::experimental::get_hint(policy);

            // modify sharing hint only if it is the default (do not overwrite
            // user supplied values)
            if (hint.sharing_mode() ==
                    hpx::threads::thread_sharing_hint::none &&
                sharing != hpx::threads::thread_sharing_hint::none)
            {
                hint.sharing_mode(sharing);
            }

            return hpx::execution::experimental::with_hint(
                HPX_FORWARD(ExPolicy, policy), hint);
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
        "hpx::parallel::util::adapt_sharing_mode is deprecated. Please use "
        "hpx::execution::experimental::adapt_sharing_mode instead.")
    decltype(auto) adapt_sharing_mode(
        ExPolicy&& policy, hpx::threads::thread_sharing_hint sharing)
    {
        return hpx::execution::experimental::adapt_sharing_mode(
            HPX_FORWARD(ExPolicy, policy), sharing);
    }
}    // namespace hpx::parallel::util
