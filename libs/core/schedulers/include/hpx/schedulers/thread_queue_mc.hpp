//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>
#include <hpx/schedulers/macros.hpp>
#include <hpx/schedulers/queue_holder_thread.hpp>

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
#include <hpx/modules/timing.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

//#define DEBUG_QUEUE_EXTRA 1

namespace hpx {

    HPX_CXX_CORE_EXPORT inline constexpr hpx::debug::enable_print<
        THREAD_QUEUE_MC_DEBUG>
        tqmc_deb("_TQ_MC_");
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    HPX_CXX_CORE_EXPORT template <typename Mutex, typename PendingQueuing,
        typename StagedQueuing, typename TerminatedQueuing>
    class thread_queue_mc
    {
    public:
        // we use a simple mutex to protect the data members for now
        using mutex_type = Mutex;

        using thread_queue_type = thread_queue_mc;

        using thread_heap_type =
            std::list<thread_id_type, util::internal_allocator<thread_id_type>>;

        using task_description = thread_init_data;
        using thread_description = thread_data;

        using work_items_type =
            typename PendingQueuing::template apply<thread_id_ref_type>::type;

        using task_items_type =
            concurrentqueue_fifo::apply<task_description>::type;

        // ----------------------------------------------------------------
        // Take thread init data from the new work queue and convert it into
        // full thread_data items that are added to the pending queue.
        //
        // New work items are taken from the queue owned by 'addfrom' and
        // added to the pending queue of this thread holder
        //
        // This is not thread safe, only the thread owning the holder should
        // call this function
        std::size_t add_new(
            std::int64_t add_count, thread_queue_type* addfrom, bool stealing)
        {
            if (addfrom->new_tasks_count_.data_.load(
                    std::memory_order_relaxed) == 0)
            {
                return 0;
            }

            std::size_t added = 0;
            task_description task;
            while (add_count-- && addfrom->new_task_items_.pop(task, stealing))
            {
                // create the new thread
                threads::thread_init_data& data = task;
                threads::thread_id_ref_type tid;

                holder_->create_thread_object(tid, data, true);
                holder_->add_to_thread_map(tid.noref());

                // Decrement only after thread_map_count_ has been incremented
                --addfrom->new_tasks_count_.data_;

                tqmc_deb.debug(debug::str<>("add_new"), "stealing", stealing,
                    debug::threadinfo<threads::thread_id_ref_type*>(&tid));

                // insert the thread into work-items queue assuming it is in
                // pending state
                HPX_ASSERT(
                    data.initial_state == thread_schedule_state::pending);

                // pushing the new thread into the pending queue of the
                // specified thread_queue
                ++added;
                schedule_work(HPX_MOVE(tid), stealing);
            }

            return added;
        }

    public:
        explicit thread_queue_mc(thread_queue_init_parameters const& parameters,
            std::size_t queue_num = static_cast<std::size_t>(-1))
          : parameters_(parameters)
          , queue_index_(static_cast<int>(queue_num))
          , holder_(nullptr)
          , new_task_items_(1024)
          , work_items_(1024)
        {
            new_tasks_count_.data_ = 0;
            work_items_count_.data_ = 0;
        }

        thread_queue_mc(thread_queue_mc const&) = delete;
        thread_queue_mc(thread_queue_mc&&) = delete;
        thread_queue_mc& operator=(thread_queue_mc const&) = delete;
        thread_queue_mc& operator=(thread_queue_mc&&) = delete;

        // ----------------------------------------------------------------
        void set_holder(queue_holder_thread<thread_queue_type>* holder)
        {
            holder_ = holder;
            tqmc_deb.debug(debug::str<>("set_holder"), "D",
                debug::dec<2>(holder_->domain_index_), "Q",
                debug::dec<3>(queue_index_));
        }

        // ----------------------------------------------------------------
        ~thread_queue_mc() = default;

        // ----------------------------------------------------------------
        // This returns the current length of the queues (work items and new
        // items)
        std::int64_t get_queue_length() const noexcept
        {
            return static_cast<std::int64_t>(work_items_count_.data_.load(
                       std::memory_order_relaxed)) +
                new_tasks_count_.data_.load(std::memory_order_relaxed);
        }

        // ----------------------------------------------------------------
        // This returns the current length of the pending queue
        std::int64_t get_queue_length_pending() const noexcept
        {
            return work_items_count_.data_.load(std::memory_order_relaxed);
        }

        // ----------------------------------------------------------------
        // This returns the current length of the staged queue
        std::int64_t get_queue_length_staged(
            std::memory_order order = std::memory_order_relaxed) const noexcept
        {
            return new_tasks_count_.data_.load(order);
        }

        // ----------------------------------------------------------------
        // Return the number of existing threads with the given state.
        std::int64_t get_thread_count() const noexcept
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "get_thread_count",
                "use get_queue_length_staged/get_queue_length_pending");
        }

        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(thread_init_data& data, thread_id_ref_type* id,
            bool const caller_owns_holder, error_code& ec)
        {
            // thread has not been created yet
            if (id)
                *id = invalid_thread_id;

            if (data.stacksize == threads::thread_stacksize::current)
            {
                data.stacksize = get_self_stacksize_enum();
            }

            HPX_ASSERT(data.stacksize != threads::thread_stacksize::current);

            if (data.run_now)
            {
                threads::thread_id_ref_type tid;
                holder_->create_thread_object(tid, data, caller_owns_holder);
                holder_->add_to_thread_map(tid.noref());

                // push the new thread in the pending queue thread
                if (data.initial_state == thread_schedule_state::pending)
                {
                    // return the thread_id_ref of the newly created thread
                    if (id)
                    {
                        *id = tid;
                    }
                    schedule_work(HPX_MOVE(tid), false);
                }
                else
                {
                    // if the thread should not be scheduled the id must be
                    // returned to the caller as otherwise the thread would
                    // go out of scope right away.
                    HPX_ASSERT(id != nullptr);
                    *id = HPX_MOVE(tid);    //-V1004
                }

                if (&ec != &throws)
                    ec = make_success_code();
                return;
            }

            // if the initial state is not pending, delayed creation will
            // fail as the newly created thread would go out of scope right
            // away (can't be scheduled).
            if (data.initial_state != thread_schedule_state::pending)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "thread_queue_mc::create_thread",
                    "staged tasks must have 'pending' as their initial state");
            }

            // do not execute the work, but register a task description for
            // later thread creation
            ++new_tasks_count_.data_;

            new_task_items_.push(task_description(HPX_MOVE(data)));

            if (&ec != &throws)
                ec = make_success_code();
        }

        // ----------------------------------------------------------------
        /// Return the next thread to be executed, return false if none is
        /// available
        bool get_next_thread(threads::thread_id_ref_type& thrd, bool other_end,
            bool check_new = false) HPX_HOT
        {
            std::int64_t const work_items_count_count =
                work_items_count_.data_.load(std::memory_order_relaxed);

            if (0 != work_items_count_count && work_items_.pop(thrd, other_end))
            {
                --work_items_count_.data_;
                tqmc_deb.debug(debug::str<>("get_next_thread"), "stealing",
                    other_end, "D", debug::dec<2>(holder_->domain_index_), "Q",
                    debug::dec<3>(queue_index_), "n",
                    debug::dec<4>(new_tasks_count_.data_), "w",
                    debug::dec<4>(work_items_count_.data_),
                    debug::threadinfo<threads::thread_id_ref_type*>(&thrd));
                return true;
            }
            if (check_new && add_new(32, this, false) > 0)    //-V112
            {
                // use check_now false to prevent infinite recursion
                return get_next_thread(thrd, other_end, false);
            }
            return false;
        }

        // ----------------------------------------------------------------
        /// Schedule the passed thread (put it on the ready work queue)
        void schedule_work(threads::thread_id_ref_type thrd, bool other_end)
        {
            ++work_items_count_.data_;
            tqmc_deb.debug(debug::str<>("schedule_work"), "stealing", other_end,
                "D", debug::dec<2>(holder_->domain_index_), "Q",
                debug::dec<3>(queue_index_), "n",
                debug::dec<4>(new_tasks_count_.data_), "w",
                debug::dec<4>(work_items_count_.data_),
                debug::threadinfo<threads::thread_id_ref_type*>(&thrd));

            work_items_.push(HPX_MOVE(thrd), other_end);
#ifdef DEBUG_QUEUE_EXTRA
            debug_queue(work_items_);
#endif
        }

        ///////////////////////////////////////////////////////////////////////
        static constexpr void on_start_thread(std::size_t) noexcept {}
        static constexpr void on_stop_thread(std::size_t) noexcept {}
        static constexpr void on_error(
            std::size_t, std::exception_ptr const&) noexcept
        {
        }

        // pops all tasks off the queue, prints info and pushes them back on
        // just because we can't iterate over the queue/stack in general
#if defined(DEBUG_QUEUE_EXTRA)
        void debug_queue(work_items_type& q)
        {
            std::unique_lock<std::mutex> Lock(debug_mtx_);
            //
            work_items_type work_items_copy_;
            int x = 0;
            thread_description* thrd;
            tqmc_deb.debug(debug::str<>("debug_queue"), "Pop work items");
            while (q.pop(thrd))
            {
                tqmc_deb.debug(debug::str<>("debug_queue"), x++,
                    debug::threadinfo<threads::thread_data*>(thrd));
                work_items_copy_.push(thrd);
            }
            tqmc_deb.debug(debug::str<>("debug_queue"), "Push work items");
            while (work_items_copy_.pop(thrd))
            {
                q.push(thrd);
                tqmc_deb.debug(debug::str<>("debug_queue"), --x,
                    debug::threadinfo<threads::thread_data*>(thrd));
            }
            tqmc_deb.debug(debug::str<>("debug_queue"), "Finished");
        }
#endif

    public:
        thread_queue_init_parameters parameters_;

        int const queue_index_;

        queue_holder_thread<thread_queue_type>* holder_;

        // count of new tasks to run, separate to new cache line to avoid false
        // sharing

        task_items_type new_task_items_;
        work_items_type work_items_;

        util::cache_line_data<std::atomic<std::int32_t>> new_tasks_count_;
        util::cache_line_data<std::atomic<std::int32_t>> work_items_count_;

#ifdef DEBUG_QUEUE_EXTRA
        std::mutex debug_mtx_;
#endif
    };
}    // namespace hpx::threads::policies
