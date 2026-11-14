//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The idle callback of the local-priority-tuned-fifo scheduler is instantiated
// here, apart from the one of the other schedulers, so that its code doesn't
// change where the code for the other schedulers is placed.

#include <hpx/config.hpp>
#include <hpx/threading_base/detail/idle_callback.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/scheduling_policy.hpp>

#include <cstddef>

namespace hpx::threads::policies {

    template HPX_CORE_EXPORT bool
    scheduler_base::idle_callback_with_rules<tuned_scheduling_rules>(
        std::size_t, tuned_scheduling_rules const&);
}    // namespace hpx::threads::policies
