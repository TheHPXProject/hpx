//  Copyright (c)      2013 Thomas Heller
//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/schedulers/local_priority_queue_scheduler.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT using default_static_priority_queue_scheduler_terminated_queue =
        lockfree_lifo;
#else
    HPX_CXX_CORE_EXPORT using default_static_priority_queue_scheduler_terminated_queue =
        lockfree_fifo;
#endif

    ///////////////////////////////////////////////////////////////////////////
    // The static_priority_queue_scheduler maintains exactly one queue of work
    // items (threads) per OS thread, where this OS thread pulls its next work
    // from. Additionally, it maintains separate queues: several for high
    // priority threads and one for low priority threads.
    //
    // High priority threads are executed by the first N OS threads before any
    // other work is executed. Low priority threads are executed by the last OS
    // thread whenever no other work is available. This scheduler does not do
    // any work stealing.
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_static_priority_queue_scheduler_terminated_queue>
    class static_priority_queue_scheduler final
      : public local_priority_queue_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing>
    {
    public:
        using base_type = local_priority_queue_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing>;

        using init_parameter_type = typename base_type::init_parameter_type;

        explicit static_priority_queue_scheduler(
            init_parameter_type const& init,
            bool deferred_initialization = true)
          : base_type(init, deferred_initialization)
        {
        }

        void set_scheduler_mode(scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) noexcept override
        {
            // this scheduler does not support stealing or numa stealing
            mode = static_cast<policies::scheduler_mode>(
                mode & ~policies::scheduler_mode::enable_stealing);
            mode = static_cast<policies::scheduler_mode>(
                mode & ~policies::scheduler_mode::enable_stealing_numa);
            scheduler_base::set_scheduler_mode(mode, pu_mask);
        }

        static std::string_view get_scheduler_name()
        {
            return "static_priority_queue_scheduler";
        }
    };
}    // namespace hpx::threads::policies
