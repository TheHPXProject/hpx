//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel_executor_aggregated.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/async_base.hpp>

namespace hpx::parallel::execution {

    // The parallel_policy_executor has fully subsumed the functionalities of
    // the original parallel_policy_executor_aggregated.
    template <typename Policy = hpx::launch::async_policy>
    using parallel_policy_executor_aggregated HPX_DEPRECATED_V(1, 9,
        "hpx::parallel::execution::parallel_policy_executor_aggregated is "
        "deprecated, use hpx::execution::parallel_policy_executor instead") =
        hpx::execution::parallel_policy_executor<Policy>;

    ///////////////////////////////////////////////////////////////////////////
    using parallel_executor_aggregated HPX_DEPRECATED_V(1, 9,
        "hpx::parallel::execution::parallel_executor_aggregated is "
        "deprecated, use hpx::execution::parallel_executor instead") =
        hpx::execution::parallel_executor;

}    // namespace hpx::parallel::execution
