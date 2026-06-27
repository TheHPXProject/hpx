//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/schedulers/deadlock_detection.hpp>
#include <hpx/schedulers/local_queue_scheduler.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT using default_static_queue_scheduler_terminated_queue =
        lockfree_lifo;
#else
    HPX_CXX_CORE_EXPORT using default_static_queue_scheduler_terminated_queue =
        lockfree_fifo;
#endif

    ///////////////////////////////////////////////////////////////////////////
    /// The local_queue_scheduler maintains exactly one queue of work
    /// items (threads) per OS thread, where this OS thread pulls its next work
    /// from.
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_static_queue_scheduler_terminated_queue>
    class static_queue_scheduler final
      : public local_queue_scheduler<Mutex, PendingQueuing, StagedQueuing,
            TerminatedQueuing>
    {
    public:
        using base_type = local_queue_scheduler<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing>;

        explicit static_queue_scheduler(
            typename base_type::init_parameter_type const& init,
            bool deferred_initialization = true)
          : base_type(init, deferred_initialization)
        {
        }

        static std::string_view get_scheduler_name()
        {
            return "static_queue_scheduler";
        }

        void set_scheduler_mode(scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) noexcept override
        {
            // this scheduler does not support stealing or numa stealing
            mode = static_cast<scheduler_mode>(
                mode & ~scheduler_mode::enable_stealing);
            mode = static_cast<scheduler_mode>(
                mode & ~scheduler_mode::enable_stealing_numa);
            scheduler_base::set_scheduler_mode(mode, pu_mask);
        }

        // Return the next thread to be executed, return false if none is
        // available
        bool get_next_thread(std::size_t num_thread, bool,
            threads::thread_id_ref_type& thrd, bool /*enable_stealing*/)
        {
            using thread_queue_type = typename base_type::thread_queue_type;

            {
                HPX_ASSERT(num_thread < this->queues_.size());

                thread_queue_type* q = this->queues_[num_thread];
                bool result = q->get_next_thread(thrd);

                q->increment_num_pending_accesses();
                if (result)
                    return true;
                q->increment_num_pending_misses();
            }

            return false;
        }

        // This is a function which gets called periodically by the thread
        // manager to allow for maintenance tasks to be executed in the
        // scheduler. Returns true if the OS thread calling this function has to
        // be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(std::size_t num_thread, bool running,
            [[maybe_unused]] std::int64_t& idle_loop_count, bool,
            std::size_t& added, thread_id_ref_type* = nullptr)
        {
            HPX_ASSERT(num_thread < this->queues_.size());

            added = 0;

            bool result = true;

            result =
                this->queues_[num_thread]->wait_or_add_new(running, added) &&
                result;
            if (0 != added)
                return result;

            // Check if we have been disabled
            if (!running)
            {
                return true;
            }

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
            // no new work is available, are we deadlocked?
            if (HPX_UNLIKELY(get_minimal_deadlock_detection_enabled() &&
                    LHPX_ENABLED(error)))
            {
                bool suspended_only = true;

                for (std::size_t i = 0;
                    suspended_only && i != this->queues_.size(); ++i)
                {
                    suspended_only = this->queues_[i]->dump_suspended_threads(
                        i, idle_loop_count, running);
                }

                if (HPX_UNLIKELY(suspended_only))
                {
                    LTM_(warning).format(
                        "queue({}): no new work available, are we deadlocked?",
                        num_thread);
                }
            }
#endif

            return result;
        }
    };
}    // namespace hpx::threads::policies
