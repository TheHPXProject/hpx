//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The pool of the local-priority-tuned-fifo scheduler is instantiated here,
// apart from the other pools, so that its code doesn't change where the code
// for the other pools is placed.

#include <hpx/config.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/thread_pools/scheduled_thread_pool.hpp>
#include <hpx/thread_pools/scheduled_thread_pool_impl.hpp>

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_priority_tuned_scheduler<>>;
