//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/executors.hpp>
#include <hpx/modules/synchronization.hpp>

namespace hpx::test {

    namespace ex = hpx::execution::experimental;

    // scope.join() has no completion scheduler. ex::sync_wait(scope.join())
    // from hpx_main can take stdexec's OS-blocking wait and starve the pool.
    // start_detached with get_start_scheduler connects join immediately;
    // binary_semaphore::acquire suspends the HPX task instead.
    template <typename Scope>
    void wait_join(Scope& scope)
    {
        hpx::binary_semaphore done{0};
        ex::start_detached(
            scope.join() | ex::then([&]() noexcept { done.release(); }),
            ex::make_env(ex::prop(
                ex::get_start_scheduler, ex::thread_pool_scheduler{})));
        done.acquire();
    }

}    // namespace hpx::test
