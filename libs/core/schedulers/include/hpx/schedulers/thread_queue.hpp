//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
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
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/schedulers/queue_helpers.hpp>

#if defined(HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION)
#include <hpx/schedulers/deadlock_detection.hpp>
#endif
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
#include <hpx/modules/timing.hpp>
#include <hpx/schedulers/maintain_queue_wait_times.hpp>
#endif
#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
#include <hpx/modules/timing.hpp>
#include <hpx/modules/util.hpp>
#endif
#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
#include <hpx/modules/util.hpp>
#endif

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
    // // Queue back-end interface:
    //
    // template <typename T>
    // struct queue_backend
    // {
    //     typedef ... container_type;
    //     typedef ... value_type;
    //     typedef ... reference;
    //     typedef ... const_reference;
    //     typedef ... size_type;
    //
    //     queue_backend(
    //         size_type initial_size = ...
    //       , size_type num_thread = ...
    //         );
    //
    //     bool push(const_reference val);
    //
    //     bool pop(reference val, bool steal = true);
    //
    //     bool empty();
    // };
    //
    // struct queue_policy
    // {
    //     template <typename T>
    //     struct apply
    //     {
    //         typedef ... type;
    //     };
    // };
    HPX_CXX_CORE_EXPORT template <typename Mutex, typename PendingQueuing,
        typename StagedQueuing, typename TerminatedQueuing>
    class thread_queue
    {
    private:
        // we use a simple mutex to protect the data members for now
        using mutex_type = Mutex;

        // this is the type of the map holding all threads (except depleted
        // ones)
        using thread_map_type =
            std::unordered_set<thread_id_type, std::hash<thread_id_type>,
                std::equal_to<>, util::internal_allocator<thread_id_type>>;

        using thread_heap_type = std::vector<thread_id_type,
            util::internal_allocator<thread_id_type>>;

        struct task_description
        {
            thread_init_data data;
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
            std::uint64_t wait_time;
#endif
        };

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        struct thread_description
        {
            thread_id_ref_type data;
            std::uint64_t wait_time;
        };
        using thread_description_ptr = thread_description*;
#else
        using thread_description_ptr = thread_id_ref_type::thread_repr*;
#endif

        using work_items_type = typename PendingQueuing::template apply<
            thread_description_ptr>::type;

        using task_items_type =
            typename StagedQueuing::template apply<task_description>::type;

        using terminated_items_type =
            typename TerminatedQueuing::template apply<thread_data*>::type;

    protected:
        template <typename Lock>
        void create_thread_object(threads::thread_id_ref_type& thrd,
            threads::thread_init_data& data, Lock& lk)
        {
            HPX_ASSERT_OWNS_LOCK(lk);

            std::ptrdiff_t const stacksize =
                data.scheduler_base->get_stack_size(data.stacksize);

#if !defined(HPX_HAVE_ADDRESS_SANITIZER)
            thread_heap_type* heap = nullptr;

            if (stacksize == parameters_.nostack_stacksize_)
            {
                heap = &thread_heap_nostack_;
            }
            else if (stacksize <= parameters_.small_stacksize_)
            {
                heap = &thread_heap_small_;
            }
            else if (stacksize <= parameters_.medium_stacksize_)
            {
                heap = &thread_heap_medium_;
            }
            else if (stacksize <= parameters_.large_stacksize_)
            {
                heap = &thread_heap_large_;
            }
            else if (stacksize <= parameters_.huge_stacksize_)
            {
                heap = &thread_heap_huge_;
            }
            HPX_ASSERT(heap);
#endif

            if (data.initial_state ==
                    thread_schedule_state::pending_do_not_schedule ||
                data.initial_state == thread_schedule_state::pending_boost)
            {
                data.initial_state = thread_schedule_state::pending;
            }

            // ASAN gets confused by reusing threads/stacks
#if !defined(HPX_HAVE_ADDRESS_SANITIZER)
            // Check for an unused thread object.
            if (heap && !heap->empty())    //-V522
            {
                // Take ownership of the thread object and rebind it.
                thrd = heap->back();
                heap->pop_back();
                get_thread_id_data(thrd)->rebind(data);
            }
            else
#endif
            {
                hpx::unlock_guard<Lock> ull(lk);

                // Allocate a new thread object.
                threads::thread_data* p;
                if (stacksize == parameters_.nostack_stacksize_)
                {
                    p = threads::thread_data_stackless::create(
                        data, this, stacksize);
                }
                else
                {
                    p = threads::thread_data_stackful::create(
                        data, this, stacksize);
                }
                thrd = thread_id_ref_type(p, thread_id_addref::no);
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // add new threads if there is some amount of work available
        std::size_t add_new(std::int64_t add_count, thread_queue* addfrom,
            std::unique_lock<mutex_type>& lk, bool steal = false)
        {
            HPX_ASSERT_OWNS_LOCK(lk);

            if (HPX_UNLIKELY(0 == add_count))
            {
                return 0;
            }

            std::size_t added = 0;
            task_description task;
            while (add_count-- && addfrom->new_tasks_.pop(task, steal))
            {
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
                if (get_maintain_queue_wait_times_enabled())
                {
                    addfrom->new_tasks_wait_ +=
                        hpx::chrono::high_resolution_clock::now() -
                        task.waittime;
                    ++addfrom->new_tasks_wait_count_;
                }
#endif
                // create the new thread
                threads::thread_init_data& data = task.data;

                [[maybe_unused]] bool const schedule_now =
                    data.initial_state == thread_schedule_state::pending;

                threads::thread_id_ref_type thrd;
                create_thread_object(thrd, data, lk);

                // add the new entry to the map of all threads
                std::pair<thread_map_type::iterator, bool> const p =
                    thread_map_.emplace(thrd.noref());

                // 26110: Caller failing to hold lock 'lk'
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif

                if (HPX_UNLIKELY(!p.second))
                {
                    --addfrom->new_tasks_count_.data_;
                    lk.unlock();
                    HPX_THROW_BAD_ALLOC("thread_queue::add_new");
                }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

                ++thread_map_count_;

                // Decrement only after thread_map_count_ has been incremented
                --addfrom->new_tasks_count_.data_;

                // insert the thread into the work-items queue assuming it is
                // in pending state, thread would go out of scope otherwise
                HPX_ASSERT(schedule_now);

                // pushing the new thread into the pending queue of the
                // specified thread_queue
                ++added;
                schedule_thread(HPX_MOVE(thrd));
            }

            if (added)
            {
                LTM_(debug).format("add_new: added {} tasks to queues", added);
            }
            return added;
        }

        ///////////////////////////////////////////////////////////////////////
        bool add_new_always(std::size_t& added, thread_queue* addfrom,
            std::unique_lock<mutex_type>& lk, bool const steal = false)
        {
            HPX_ASSERT_OWNS_LOCK(lk);

            // no need to try converting from other queue if that has no staged
            // threads
            if (HPX_LIKELY(addfrom->new_tasks_count_.data_.load(
                               std::memory_order_relaxed) == 0))
            {
                return false;
            }

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
            util::tick_counter tc(add_new_time_);
#endif

            // create new threads from pending tasks (if appropriate)
            std::int64_t add_count = -1;    // default is no constraint

            // if we are desperate (no work in the queues), add some even if the
            // map holds more than max_thread_count
            if (HPX_LIKELY(parameters_.max_thread_count_))
            {
                std::int64_t const count =
                    static_cast<std::int64_t>(thread_map_.size());

                if (parameters_.max_thread_count_ >=
                    count + parameters_.min_add_new_count_)
                {
                    HPX_ASSERT(parameters_.max_thread_count_ - count <
                        (std::numeric_limits<std::int64_t>::max)());

                    add_count =
                        std::clamp(parameters_.max_thread_count_ - count,
                            parameters_.min_add_new_count_,
                            parameters_.max_add_new_count_);
                }
                else if (work_items_.empty())
                {
                    // add this number of threads
                    add_count = parameters_.min_add_new_count_;

                    // increase max_thread_count
                    parameters_.max_thread_count_ +=
                        parameters_.min_add_new_count_;    //-V101
                }
                else
                {
                    return false;
                }
            }

            std::size_t const added_new =
                add_new(add_count, addfrom, lk, steal);
            added += added_new;
            return added_new != 0;
        }

        constexpr threads::thread_stacksize get_stacksize_enum_from_size(
            std::ptrdiff_t const size) const
        {
            if (size == parameters_.nostack_stacksize_)
                return threads::thread_stacksize::nostack;

            if (size <= parameters_.small_stacksize_)
                return threads::thread_stacksize::small_;
            if (size <= parameters_.medium_stacksize_)
                return threads::thread_stacksize::medium;
            if (size <= parameters_.large_stacksize_)
                return threads::thread_stacksize::large;

            return threads::thread_stacksize::huge;
        }

        template <typename Lock>
        void recycle_thread(thread_id_type&& thrd, Lock& lk)
        {
            HPX_ASSERT_OWNS_LOCK(lk);

            auto* thrd_data = get_thread_id_data(thrd);
            HPX_ASSERT_MSG(thrd_data != nullptr, "invalid thread id");

            switch (std::ptrdiff_t const size = thrd_data->get_stack_size();
                get_stacksize_enum_from_size(size))
            {
            case threads::thread_stacksize::small_:
                if (thread_heap_small_.size() <
                    parameters_.cached_threads_count_)
                {
                    thread_heap_small_.emplace_back(HPX_MOVE(thrd));
                    return;
                }
                break;

            case threads::thread_stacksize::medium:
                if (thread_heap_medium_.size() <
                    parameters_.cached_threads_count_)
                {
                    thread_heap_medium_.emplace_back(HPX_MOVE(thrd));
                    return;
                }
                break;

            case threads::thread_stacksize::large:
                if (thread_heap_large_.size() <
                    parameters_.cached_threads_count_)
                {
                    thread_heap_large_.emplace_back(HPX_MOVE(thrd));
                    return;
                }
                break;

            case threads::thread_stacksize::huge:
                if (thread_heap_huge_.size() <
                    parameters_.cached_threads_count_)
                {
                    thread_heap_huge_.emplace_back(HPX_MOVE(thrd));
                    return;
                }
                break;

            case threads::thread_stacksize::nostack:
                if (thread_heap_nostack_.size() <
                    parameters_.cached_threads_count_)
                {
                    thread_heap_nostack_.emplace_back(HPX_MOVE(thrd));
                    return;
                }
                break;

            default:
                HPX_ASSERT_MSG(
                    false, util::format("Invalid stack size {1}", size));
                return;
            }

            deallocate(thrd_data);
        }

    public:
        // This function makes sure all threads which are marked for deletion
        // (state is terminated) are properly destroyed.
        //
        // This returns 'true' if there are no more terminated threads waiting
        // to be deleted.
        template <typename Lock>
        bool cleanup_terminated_locked(Lock& lk, bool delete_all = false)
        {
            HPX_ASSERT_OWNS_LOCK(lk);

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
            util::tick_counter tc(cleanup_terminated_time_);
#endif

            if (terminated_items_count_.load(std::memory_order_acquire) == 0)
                return true;

            if (delete_all)
            {
                // delete all threads
                std::int64_t term_delta = 0;
                std::int64_t map_delta = 0;
                thread_data* to_delete;
                while (terminated_items_.pop(to_delete))
                {
                    ++term_delta;
                    thread_id_type tid(to_delete);

                    // this thread has to be managed by this queue, it may have
                    // ended up on the terminate threads list more than once,
                    // however
                    HPX_ASSERT(
                        &get_thread_id_data(tid)->get_queue<thread_queue>() ==
                        this);

                    if (thread_map_.erase(tid) != 0)
                    {
                        recycle_thread(HPX_MOVE(tid), lk);
                        ++map_delta;
                    }
                }
                terminated_items_count_.fetch_sub(
                    term_delta, std::memory_order_acq_rel);
                [[maybe_unused]] std::int64_t remaining =
                    thread_map_count_.fetch_sub(
                        map_delta, std::memory_order_acq_rel);
                HPX_ASSERT(remaining - map_delta >= 0);
            }
            else
            {
                // delete only this many threads
                std::int64_t delete_count =
                    (std::min) (static_cast<std::int64_t>(
                                    terminated_items_count_ / 10),
                        static_cast<std::int64_t>(
                            parameters_.max_delete_count_));

                // delete at least this many threads
                delete_count = (std::max) (delete_count,
                    static_cast<std::int64_t>(parameters_.min_delete_count_));

                std::int64_t term_delta = 0;
                std::int64_t map_delta = 0;
                thread_data* to_delete;
                while (delete_count && terminated_items_.pop(to_delete))
                {
                    ++term_delta;
                    thread_id_type tid(to_delete);

                    // this thread has to be managed by this queue, it may have
                    // ended up on the terminate threads list more than once,
                    // however
                    HPX_ASSERT(
                        &get_thread_id_data(tid)->get_queue<thread_queue>() ==
                        this);

                    if (thread_map_.erase(tid) != 0)
                    {
                        recycle_thread(HPX_MOVE(tid), lk);
                        ++map_delta;
                    }
                    --delete_count;
                }
                terminated_items_count_.fetch_sub(
                    term_delta, std::memory_order_acq_rel);
                [[maybe_unused]] std::int64_t remaining =
                    thread_map_count_.fetch_sub(
                        map_delta, std::memory_order_acq_rel);
                HPX_ASSERT(remaining - map_delta >= 0);
            }
            return terminated_items_count_.load(std::memory_order_acquire) == 0;
        }

    public:
        bool cleanup_terminated(bool delete_all = false)    //-V1071
        {
            if (terminated_items_count_.load(std::memory_order_acquire) == 0)
                return true;

            if (delete_all)
            {
                // do not lock mutex while deleting all threads, do it piece-wise
                while (true)
                {
                    std::unique_lock<mutex_type> lk(mtx_, std::try_to_lock);
                    if (!lk.owns_lock())
                        break;    // avoid long wait on lock

                    if (cleanup_terminated_locked(lk, false))
                    {
                        return true;
                    }
                }
                return false;
            }

            std::unique_lock<mutex_type> lk(mtx_, std::try_to_lock);
            if (!lk.owns_lock())
                return false;    // avoid long wait on lock

            return cleanup_terminated_locked(lk, false);
        }

        explicit thread_queue(thread_queue_init_parameters const& parameters =
                                  thread_queue_init_parameters{})
          : parameters_(parameters)
          , thread_map_count_(0)
          , work_items_(128)
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
          , work_items_wait_(0)
          , work_items_wait_count_(0)
#endif
          , terminated_items_(128)
          , terminated_items_count_(0)
          , new_tasks_(128)
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
          , new_tasks_wait_(0)
          , new_tasks_wait_count_(0)
#endif
#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
          , add_new_time_(0)
          , cleanup_terminated_time_(0)
#endif
#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
          , pending_misses_(0)
          , pending_accesses_(0)
          , stolen_from_pending_(0)
          , stolen_from_staged_(0)
          , stolen_to_pending_(0)
          , stolen_to_staged_(0)
#endif
        {
            new_tasks_count_.data_ = 0;
            work_items_count_.data_ = 0;
        }

        static void deallocate(threads::thread_data* p) noexcept
        {
            p->destroy();
        }

        ~thread_queue()
        {
            for (auto const& t : thread_heap_small_)
                deallocate(get_thread_id_data(t));

            for (auto const& t : thread_heap_medium_)
                deallocate(get_thread_id_data(t));

            for (auto const& t : thread_heap_large_)
                deallocate(get_thread_id_data(t));

            for (auto const& t : thread_heap_huge_)
                deallocate(get_thread_id_data(t));

            for (auto const& t : thread_heap_nostack_)
                deallocate(get_thread_id_data(t));
        }

        thread_queue(thread_queue const&) = delete;
        thread_queue(thread_queue&&) = delete;
        thread_queue& operator=(thread_queue const&) = delete;
        thread_queue& operator=(thread_queue&&) = delete;

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool reset) noexcept
        {
            return util::get_and_reset_value(add_new_time_, reset);
        }

        std::uint64_t get_cleanup_time(bool reset) noexcept
        {
            return util::get_and_reset_value(cleanup_terminated_time_, reset);
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        // This returns the current length of the queues (work items and new items)
        std::int64_t get_queue_length(
            std::memory_order order = std::memory_order_acquire) const noexcept
        {
            return work_items_count_.data_.load(order) +
                new_tasks_count_.data_.load(order);
        }

        // This returns the current length of the pending queue
        std::int64_t get_pending_queue_length(
            std::memory_order order = std::memory_order_acquire) const noexcept
        {
            return work_items_count_.data_.load(order);
        }

        // This returns the current length of the staged queue
        std::int64_t get_staged_queue_length(
            std::memory_order order = std::memory_order_acquire) const noexcept
        {
            return new_tasks_count_.data_.load(order);
        }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        std::uint64_t get_average_task_wait_time() const noexcept
        {
            std::uint64_t count = new_tasks_wait_count_;
            if (count == 0)
                return 0;
            return new_tasks_wait_ / count;
        }

        std::uint64_t get_average_thread_wait_time() const noexcept
        {
            std::uint64_t count = work_items_wait_count_;
            if (count == 0)
                return 0;
            return work_items_wait_ / count;
        }
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(bool reset) noexcept
        {
            return util::get_and_reset_value(pending_misses_, reset);
        }

        void increment_num_pending_misses(std::size_t num = 1) noexcept
        {
            pending_misses_.fetch_add(num, std::memory_order_relaxed);
        }

        std::int64_t get_num_pending_accesses(bool reset) noexcept
        {
            return util::get_and_reset_value(pending_accesses_, reset);
        }

        void increment_num_pending_accesses(std::size_t num = 1) noexcept
        {
            pending_accesses_.fetch_add(num, std::memory_order_relaxed);
        }

        std::int64_t get_num_stolen_from_pending(bool reset) noexcept
        {
            return util::get_and_reset_value(stolen_from_pending_, reset);
        }

        void increment_num_stolen_from_pending(std::size_t num = 1) noexcept
        {
            stolen_from_pending_.fetch_add(num, std::memory_order_relaxed);
        }

        std::int64_t get_num_stolen_from_staged(bool reset) noexcept
        {
            return util::get_and_reset_value(stolen_from_staged_, reset);
        }

        void increment_num_stolen_from_staged(std::size_t num = 1) noexcept
        {
            stolen_from_staged_.fetch_add(num, std::memory_order_relaxed);
        }

        std::int64_t get_num_stolen_to_pending(bool reset) noexcept
        {
            return util::get_and_reset_value(stolen_to_pending_, reset);
        }

        void increment_num_stolen_to_pending(std::size_t num = 1) noexcept
        {
            stolen_to_pending_.fetch_add(num, std::memory_order_relaxed);
        }

        std::int64_t get_num_stolen_to_staged(bool reset) noexcept
        {
            return util::get_and_reset_value(stolen_to_staged_, reset);
        }

        void increment_num_stolen_to_staged(std::size_t num = 1) noexcept
        {
            stolen_to_staged_.fetch_add(num, std::memory_order_relaxed);
        }
#else
        static constexpr void increment_num_pending_misses(
            std::size_t /* num */ = 1) noexcept
        {
        }
        static constexpr void increment_num_pending_accesses(
            std::size_t /* num */ = 1) noexcept
        {
        }
        static constexpr void increment_num_stolen_from_pending(
            std::size_t /* num */ = 1) noexcept
        {
        }
        static constexpr void increment_num_stolen_from_staged(
            std::size_t /* num */ = 1) noexcept
        {
        }
        static constexpr void increment_num_stolen_to_pending(
            std::size_t /* num */ = 1) noexcept
        {
        }
        static constexpr void increment_num_stolen_to_staged(
            std::size_t /* num */ = 1) noexcept
        {
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(
            thread_init_data& data, thread_id_ref_type* id, error_code& ec)
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
                // The mutex can not be locked while a new thread is getting
                // created, as it might have that the current HPX thread gets
                // suspended.
                threads::thread_id_ref_type thrd;

                std::unique_lock<mutex_type> lk(mtx_);

                bool const schedule_now =
                    data.initial_state == thread_schedule_state::pending;

                create_thread_object(thrd, data, lk);

                // add a new entry in the map for this thread
                std::pair<thread_map_type::iterator, bool> const p =
                    thread_map_.emplace(thrd.noref());

                if (HPX_UNLIKELY(!p.second))
                {
                    lk.unlock();
                    HPX_THROWS_BAD_ALLOC_IF(ec, "thread_queue::create_thread");
                    return;
                }
                ++thread_map_count_;

                // this thread has to be in the map now
                HPX_ASSERT(thread_map_.find(thrd.noref()) != thread_map_.end());
                HPX_ASSERT(
                    &get_thread_id_data(thrd)->get_queue<thread_queue>() ==
                    this);

                // push the new thread in the pending thread queue
                if (schedule_now)
                {
                    // return the thread_id_ref of the newly created thread
                    if (id)
                    {
                        *id = thrd;
                    }
                    schedule_thread(HPX_MOVE(thrd));
                }
                else
                {
                    // if the thread should not be scheduled the id must be
                    // returned to the caller as otherwise the thread would
                    // go out of scope right away.
                    HPX_ASSERT(id != nullptr);
                    *id = HPX_MOVE(thrd);    //-V1004
                }

                if (&ec != &throws)
                    ec = make_success_code();
                return;
            }

            // if the initial state is not pending, delayed creation will fail
            // as the newly created thread would go out of scope right away
            // (can't be scheduled).
            if (data.initial_state != thread_schedule_state::pending)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "thread_queue::create_thread",
                    "staged tasks must have 'pending' as their initial "
                    "state");
            }

            // do not execute the work, but register a task description for
            // later thread creation
            ++new_tasks_count_.data_;

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
            new_tasks_.push(task_description{
                HPX_MOVE(data), hpx::chrono::high_resolution_clock::now()});
#else
            new_tasks_.push(task_description{HPX_MOVE(data)});    //-V106
#endif
            if (&ec != &throws)
                ec = make_success_code();
        }

        void move_work_items_from(thread_queue* src, std::int64_t count)
        {
            thread_description_ptr trd;
            while (src->work_items_.pop(trd))
            {
                --src->work_items_count_.data_;

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
                if (get_maintain_queue_wait_times_enabled())
                {
                    std::uint64_t now =
                        hpx::chrono::high_resolution_clock::now();
                    src->work_items_wait_ += now - trd->waittime;
                    ++src->work_items_wait_count_;
                    trd->waittime = now;
                }
#endif

                bool const finished = (count == ++work_items_count_.data_);
                work_items_.push(trd);
                if (finished)
                    break;
            }
        }

        void move_task_items_from(thread_queue* src, std::int64_t count)
        {
            task_description task;
            while (src->new_tasks_.pop(task))
            {
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
                if (get_maintain_queue_wait_times_enabled())
                {
                    std::int64_t now =
                        hpx::chrono::high_resolution_clock::now();
                    src->new_tasks_wait_ += now - task.waittime;
                    ++src->new_tasks_wait_count_;
                    task.waittime = now;
                }
#endif

                bool const finish = (count == ++new_tasks_count_.data_);

                // Decrement only after the local new_tasks_count_ has
                // been incremented
                --src->new_tasks_count_.data_;

                if (new_tasks_.push(HPX_MOVE(task)))
                {
                    if (finish)
                        break;
                }
                else
                {
                    --new_tasks_count_.data_;
                }
            }
        }

        // Return the next thread to be executed, return false if none is
        // available
        bool get_next_thread(threads::thread_id_ref_type& thrd,
            bool allow_stealing = false, bool steal = false) HPX_HOT
        {
            std::int64_t const work_items_count =
                work_items_count_.data_.load(std::memory_order_relaxed);

            if (work_items_count == 0)
            {
                return false;
            }

            if (allow_stealing &&
                parameters_.min_tasks_to_steal_pending_ > work_items_count)
            {
                return false;
            }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
            thread_description_ptr task_desc;
            if (work_items_.pop(task_desc, steal))
            {
                --work_items_count_.data_;

                if (get_maintain_queue_wait_times_enabled())
                {
                    work_items_wait_ +=
                        hpx::chrono::high_resolution_clock::now() -
                        task_desc->waittime;
                    ++work_items_wait_count_;
                }

                thrd = HPX_MOVE(task_desc->data);
                delete task_desc;

                return true;
            }
#else
            thread_description_ptr next_thrd;
            if (work_items_.pop(next_thrd, steal))
            {
                thrd.reset(next_thrd, false);    // do not addref!
                --work_items_count_.data_;
                return true;
            }
#endif
            return false;
        }

        // Return the next threads to be executed, return 0 if none are
        // available
        template <typename Iterator>
        std::size_t get_next_threads(Iterator it, std::int64_t max_items,
            bool allow_stealing = false, bool steal = false)
        {
            std::int64_t const work_items_count =
                (std::min) (work_items_count_.data_.load(
                                std::memory_order_relaxed),
                    max_items);

            if (work_items_count == 0)
            {
                return 0;
            }

            if (allow_stealing &&
                parameters_.min_tasks_to_steal_pending_ > work_items_count)
            {
                return 0;
            }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
            std::size_t const max_items_requested = max_items;

            thread_description_ptr task_desc;
            while (work_items_.pop(task_desc, steal))
            {
                if (get_maintain_queue_wait_times_enabled())
                {
                    work_items_wait_ +=
                        hpx::chrono::high_resolution_clock::now() -
                        task_desc->waittime;
                    ++work_items_wait_count_;
                }

                *it++ = HPX_MOVE(task_desc->data);
                delete task_desc;

                --max_items;
                if (--work_items_count_.data_ == 0)
                {
                    break;
                }
            }

            return max_items_requested - max_items;
#else
            if constexpr (work_items_type::support_bulk_dequeue)
            {
                std::size_t const dequeued =
                    work_items_.pop_bulk(it, work_items_count, steal);
                work_items_count_.data_ -= dequeued;
                return dequeued;
            }
            else
            {
                std::size_t const max_items_requested = max_items;

                thread_description_ptr next_thrd;
                while (max_items != 0 && work_items_.pop(next_thrd, steal))
                {
                    *it++ = threads::thread_id_ref_type(next_thrd,
                        threads::thread_id_addref::no);    // do not addref!

                    --max_items;
                    if (--work_items_count_.data_ == 0)
                    {
                        break;
                    }
                }

                return max_items_requested - max_items;
            }
#endif
        }

        // Schedule the passed thread
        void schedule_thread(
            threads::thread_id_ref_type thrd, bool other_end = false)
        {
            ++work_items_count_.data_;

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
            work_items_.push(new thread_description{HPX_MOVE(thrd),
                                 hpx::chrono::high_resolution_clock::now()},
                other_end);
#else
            // detach the thread from the id_ref without decrementing
            // the reference count
            work_items_.push(thrd.detach(), other_end);
#endif
        }

        // Destroy the passed thread as it has been terminated
        void destroy_thread(threads::thread_data* thrd)
        {
            HPX_ASSERT(&thrd->get_queue<thread_queue>() == this);

            terminated_items_.push(thrd);

            if (++terminated_items_count_ > parameters_.max_terminated_threads_)
            {
                cleanup_terminated(true);    // clean up all terminated threads
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // Return the number of existing threads with the given state.
        std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown) const
        {
            if (thread_schedule_state::terminated == state)
                return terminated_items_count_;

            if (thread_schedule_state::staged == state)
                return new_tasks_count_.data_;

            if (thread_schedule_state::unknown == state)
            {
                return thread_map_count_ + new_tasks_count_.data_ -
                    terminated_items_count_;
            }

            // acquire lock only if absolutely necessary
            std::lock_guard<mutex_type> lk(mtx_);

            std::int64_t num_threads = 0;
            auto const end = thread_map_.end();
            for (auto it = thread_map_.begin(); it != end; ++it)
            {
                if (get_thread_id_data(*it)->get_state().state() == state)
                    ++num_threads;
            }
            return num_threads;
        }

        ///////////////////////////////////////////////////////////////////////
        void abort_all_suspended_threads()
        {
            std::lock_guard<mutex_type> lk(mtx_);
            auto const end = thread_map_.end();
            for (auto it = thread_map_.begin(); it != end; ++it)
            {
                auto* const thrd = get_thread_id_data(*it);
                if (thrd->get_state().state() ==
                    thread_schedule_state::suspended)
                {
                    [[maybe_unused]] auto const s =
                        thrd->set_state(thread_schedule_state::pending,
                            thread_restart_state::abort);

                    // thread holds self-reference
                    HPX_ASSERT(thrd->count_ > 1);
                    schedule_thread(thread_id_ref_type(thrd));
                }
            }
        }

        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state = thread_schedule_state::unknown) const
        {
            std::uint64_t count = thread_map_count_;
            if (state == thread_schedule_state::terminated)
            {
                count = terminated_items_count_;
            }
            else if (state == thread_schedule_state::staged)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "thread_queue::iterate_threads",
                    "can't iterate over thread ids of staged threads");
            }

            std::vector<thread_id_type> ids;
            ids.reserve(static_cast<std::size_t>(count));

            if (state == thread_schedule_state::unknown)
            {
                std::lock_guard<mutex_type> lk(mtx_);
                auto const end = thread_map_.end();
                for (auto it = thread_map_.begin(); it != end; ++it)
                {
                    ids.push_back(*it);
                }
            }
            else
            {
                std::lock_guard<mutex_type> lk(mtx_);
                auto const end = thread_map_.end();
                for (auto it = thread_map_.begin(); it != end; ++it)
                {
                    if (get_thread_id_data(*it)->get_state().state() == state)
                        ids.push_back(*it);
                }
            }

            // now invoke callback function for all matching threads
            if (std::any_of(
                    ids.begin(), ids.end(), [&](auto id) { return !f(id); }))
                return false;

            return true;
        }

        // This is a function that gets called periodically by the thread
        // manager to allow for maintenance tasks to be executed in the
        // scheduler. Returns true if the OS thread calling this function has to
        // be terminated (i.e. no more work has to be done).
        inline bool wait_or_add_new(bool const running, std::size_t& added,
            bool const steal = false) HPX_HOT
        {
            // no need to try converting from other queue if that has no staged
            // threads
            auto const new_tasks_count =
                new_tasks_count_.data_.load(std::memory_order_relaxed);
            if (HPX_LIKELY(0 == new_tasks_count))
            {
                return !running;
            }

            if (new_tasks_count < parameters_.min_tasks_to_steal_staged_)
            {
                return false;
            }

            // No obvious work has to be done, so a lock won't hurt too much.
            //
            // We prefer to exit this function (some kind of very short busy
            // waiting) to blocking on this lock. Locking fails either when a
            // thread is currently doing thread maintenance, which means there
            // might be new work, or the thread owning the lock just falls
            // through to the cleanup work below (no work is available) in which
            // case the current thread (which failed to acquire the lock) will
            // just retry to enter this loop.
            std::unique_lock<mutex_type> lk(mtx_, std::try_to_lock);
            if (!lk.owns_lock())
                return false;    // avoid long wait on lock

            // stop running after all HPX threads have been terminated
            return !add_new_always(added, this, lk, steal);
        }

        inline bool wait_or_add_new(bool const running, std::size_t& added,
            thread_queue* add_from, bool const steal = false) HPX_HOT
        {
            // try to generate new threads from task lists, but only if our own
            // list of threads is empty
            if (0 == work_items_count_.data_.load(std::memory_order_relaxed))
            {
                // see if we can avoid grabbing the lock below

                // don't try to steal if there are only a few tasks left on this
                // queue
                std::int64_t new_tasks_count =
                    add_from->new_tasks_count_.data_.load(
                        std::memory_order_relaxed);

                bool const enough_threads = new_tasks_count != 0 &&
                    new_tasks_count >= parameters_.min_tasks_to_steal_staged_;

                if (running && !enough_threads)
                {
                    if (new_tasks_count != 0)
                    {
                        LTM_(debug).format("thread_queue::wait_or_add_new: "
                                           "not enough threads "
                                           "to steal from queue {} to "
                                           "queue {}, have {} but "
                                           "need at least {}",
                            add_from, this, new_tasks_count,
                            parameters_.min_tasks_to_steal_staged_);
                    }

                    return false;
                }

                // No obvious work has to be done, so a lock won't hurt too
                // much.
                //
                // We prefer to exit this function (some kind of very short busy
                // waiting) to blocking on this lock. Locking fails either when
                // a thread is currently doing thread maintenance, which means
                // there might be new work, or the thread owning the lock just
                // falls through to the cleanup work below (no work is
                // available) in which case the current thread (which failed to
                // acquire the lock) will just retry to enter this loop.
                std::unique_lock<mutex_type> lk(mtx_, std::try_to_lock);
                if (!lk.owns_lock())
                    return false;    // avoid long wait on lock

                // stop running after all HPX threads have been terminated
                bool const added_new =
                    add_new_always(added, add_from, lk, steal);
                if (!added_new)
                {
                    // Before exiting each of the OS threads deletes the
                    // remaining terminated HPX threads
                    // REVIEW: Should we be doing this if we are stealing?
                    bool const can_exit = cleanup_terminated_locked(lk, true);
                    if (!running && can_exit)
                    {
                        // we don't have any registered work items anymore
                        // do_some_work();       // notify possibly waiting threads
                        return true;    // terminate scheduling loop
                    }
                    return false;
                }
                else
                {
                    cleanup_terminated_locked(lk);
                    return false;
                }
            }

            bool const can_exit = cleanup_terminated(true);
            if (!running && can_exit)
            {
                // we don't have any registered work items anymore
                return true;    // terminate scheduling loop
            }

            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        bool dump_suspended_threads(
            [[maybe_unused]] std::size_t const num_thread,
            [[maybe_unused]] std::int64_t& idle_loop_count,
            [[maybe_unused]] bool const running)
        {
#if !defined(HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION)
            return false;
#else
            if (get_minimal_deadlock_detection_enabled())
            {
                std::lock_guard<mutex_type> lk(mtx_);
                return detail::dump_suspended_threads(
                    num_thread, thread_map_, idle_loop_count, running);
            }
            return false;
#endif
        }

        ///////////////////////////////////////////////////////////////////////
        void on_start_thread(std::size_t /* num_thread */)
        {
            thread_heap_small_.reserve(parameters_.init_threads_count_);
            thread_heap_medium_.reserve(parameters_.init_threads_count_);
            thread_heap_large_.reserve(parameters_.init_threads_count_);
            thread_heap_huge_.reserve(parameters_.init_threads_count_);

            // Pre-allocate init_threads_count threads, with accompanying stack,
            // with the default stack size
            static_assert(
                thread_stacksize::default_ == thread_stacksize::small_,
                "This assumes that the default stacksize is \"small_\". If "
                "the default changes, so should this code. If this "
                "static_assert fails you've most likely changed the default "
                "without changing the code here.");

            std::lock_guard<mutex_type> lk(mtx_);
            for (std::uint64_t i = 0; i < parameters_.init_threads_count_; ++i)
            {
                // We don't care about the init parameters since this thread
                // will be rebound once it is actually used
                hpx::threads::thread_init_data init_data;

                // We start the reference count at zero since the thread goes
                // immediately into the list of recycled threads
                threads::thread_data* p =
                    threads::thread_data_stackful::create(init_data, this,
                        parameters_.small_stacksize_, thread_id_addref::no);
                HPX_ASSERT(p);

                // Finally, store the thread for later use
                thread_heap_small_.emplace_back(p);
            }
        }
        static constexpr void on_stop_thread(std::size_t) noexcept {}
        static constexpr void on_error(
            std::size_t, std::exception_ptr const&) noexcept
        {
        }

    private:
        thread_queue_init_parameters parameters_;

        mutable mutex_type mtx_;    // mutex protecting the members

        thread_map_type thread_map_;    // mapping of thread id's to HPX-threads

        // overall count of work items
        std::atomic<std::int64_t> thread_map_count_;

        work_items_type work_items_;    // list of active work items

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        // overall wait time of work items
        std::atomic<std::int64_t> work_items_wait_;
        // overall number of work items in queue
        std::atomic<std::int64_t> work_items_wait_count_;
#endif
        // list of terminated threads
        terminated_items_type terminated_items_;
        // count of terminated items
        std::atomic<std::int64_t> terminated_items_count_;

        task_items_type new_tasks_;    // list of new tasks to run

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        // overall wait time of new tasks
        std::atomic<std::int64_t> new_tasks_wait_;
        // overall number tasks waited
        std::atomic<std::int64_t> new_tasks_wait_count_;
#endif

        thread_heap_type thread_heap_small_;
        thread_heap_type thread_heap_medium_;
        thread_heap_type thread_heap_large_;
        thread_heap_type thread_heap_huge_;
        thread_heap_type thread_heap_nostack_;

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t add_new_time_;
        std::uint64_t cleanup_terminated_time_;
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        // # of times our associated worker-thread couldn't find work in work_items
        std::atomic<std::int64_t> pending_misses_;

        // # of times our associated worker-thread looked for work in work_items
        std::atomic<std::int64_t> pending_accesses_;

        // count of work_items stolen from this queue
        std::atomic<std::int64_t> stolen_from_pending_;
        // count of new_tasks stolen from this queue
        std::atomic<std::int64_t> stolen_from_staged_;
        // count of work_items stolen to this queue from other queues
        std::atomic<std::int64_t> stolen_to_pending_;
        // count of new_tasks stolen to this queue from other queues
        std::atomic<std::int64_t> stolen_to_staged_;
#endif
        // count of new tasks to run, separate to new cache line to avoid false
        // sharing
        util::cache_line_data<std::atomic<std::int64_t>> new_tasks_count_;

        // count of active work items
        util::cache_line_data<std::atomic<std::int64_t>> work_items_count_;
    };

}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
