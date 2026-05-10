//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/executor_scheduler_fwd.hpp

#pragma once

#include <hpx/config.hpp>

namespace hpx::execution::experimental {

    // Forward declarations, see executor_scheduler.hpp
    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_scheduler;

    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_sender;

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Receiver>
    struct executor_operation_state;

}    // namespace hpx::execution::experimental
