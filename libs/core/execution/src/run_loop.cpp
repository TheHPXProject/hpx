//  Copyright (c) 2023-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution/algorithms/run_loop.hpp>

#include <atomic>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::execution::experimental::detail {

    void intrusive_ptr_add_ref(run_loop_data* p) noexcept
    {
        p->count_.increment();
    }

    void intrusive_ptr_release(run_loop_data* p) noexcept
    {
        if (0 == p->count_.decrement())
        {
            // The thread that decrements the reference count to zero must
            // perform an acquire to ensure that it doesn't start destructing
            // the object until all previous writes have drained.
            std::atomic_thread_fence(std::memory_order_acquire);

            delete p;
        }
    }
}    // namespace hpx::execution::experimental::detail
