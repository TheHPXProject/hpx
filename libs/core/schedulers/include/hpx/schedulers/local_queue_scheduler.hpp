//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/affinity.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/schedulers/deadlock_detection.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>
#include <hpx/schedulers/thread_queue.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT using default_local_queue_scheduler_terminated_queue =
        lockfree_lifo;
#else
    HPX_CXX_CORE_EXPORT using default_local_queue_scheduler_terminated_queue =
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
            default_local_queue_scheduler_terminated_queue>
    class local_queue_scheduler : public scheduler_base
    {
    public:
        using has_periodic_maintenance = std::false_type;

        using thread_queue_type = thread_queue<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing>;

        struct init_parameter
        {
            init_parameter(std::size_t num_queues,
                detail::affinity_data const& affinity_data,
                thread_queue_init_parameters const& thread_queue_init =
                    thread_queue_init_parameters{},
                char const* description = "local_queue_scheduler")
              : num_queues_(num_queues)
              , thread_queue_init_(thread_queue_init)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            init_parameter(std::size_t num_queues,
                detail::affinity_data const& affinity_data,
                char const* description)
              : num_queues_(num_queues)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            std::size_t num_queues_;
            thread_queue_init_parameters thread_queue_init_;
            detail::affinity_data const& affinity_data_;
            char const* description_;
        };
        using init_parameter_type = init_parameter;

        explicit local_queue_scheduler(init_parameter_type const& init,
            bool deferred_initialization = true)
          : scheduler_base(
                init.num_queues_, init.description_, init.thread_queue_init_)
          , queues_(init.num_queues_)
          , curr_queue_(0)
          , affinity_data_(init.affinity_data_)
          , numa_domain_masks_(
                init.num_queues_, create_topology().get_machine_affinity_mask())
          , outside_numa_domain_masks_(
                init.num_queues_, create_topology().get_machine_affinity_mask())
        {
#if !defined(HPX_NATIVE_MIC)    // we know that the MIC has one NUMA domain only
            resize(steals_in_numa_domain_,
                static_cast<std::size_t>(threads::hardware_concurrency()));
            resize(steals_outside_numa_domain_,
                static_cast<std::size_t>(threads::hardware_concurrency()));
#endif
            if (!deferred_initialization)
            {
                HPX_ASSERT(init.num_queues_ != 0);
                for (std::size_t i = 0; i < init.num_queues_; ++i)
                    queues_[i] = new thread_queue_type(thread_queue_init_);
            }
        }

        local_queue_scheduler(local_queue_scheduler const&) = delete;
        local_queue_scheduler(local_queue_scheduler&&) = delete;
        local_queue_scheduler& operator=(local_queue_scheduler const&) = delete;
        local_queue_scheduler& operator=(local_queue_scheduler&&) = delete;

        ~local_queue_scheduler() override
        {
            for (std::size_t i = 0; i != queues_.size(); ++i)
                delete queues_[i];
        }

        static std::string_view get_scheduler_name()
        {
            return "local_queue_scheduler";
        }

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool reset) override
        {
            std::uint64_t time = 0;

            for (std::size_t i = 0; i != queues_.size(); ++i)
                time += queues_[i]->get_creation_time(reset);

            return time;
        }

        std::uint64_t get_cleanup_time(bool reset) override
        {
            std::uint64_t time = 0;

            for (std::size_t i = 0; i != queues_.size(); ++i)
                time += queues_[i]->get_cleanup_time(reset);

            return time;
        }
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_pending_misses = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    num_pending_misses +=
                        queues_[i]->get_num_pending_misses(reset);

                return num_pending_misses;
            }

            num_pending_misses +=
                queues_[num_thread]->get_num_pending_misses(reset);
            return num_pending_misses;
        }

        std::int64_t get_num_pending_accesses(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_pending_accesses = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    num_pending_accesses +=
                        queues_[i]->get_num_pending_accesses(reset);

                return num_pending_accesses;
            }

            num_pending_accesses +=
                queues_[num_thread]->get_num_pending_accesses(reset);
            return num_pending_accesses;
        }

        std::int64_t get_num_stolen_from_pending(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    num_stolen_threads +=
                        queues_[i]->get_num_stolen_from_pending(reset);
                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread]->get_num_stolen_from_pending(reset);
            return num_stolen_threads;
        }

        std::int64_t get_num_stolen_to_pending(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    num_stolen_threads +=
                        queues_[i]->get_num_stolen_to_pending(reset);
                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread]->get_num_stolen_to_pending(reset);
            return num_stolen_threads;
        }

        std::int64_t get_num_stolen_from_staged(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    num_stolen_threads +=
                        queues_[i]->get_num_stolen_from_staged(reset);
                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread]->get_num_stolen_from_staged(reset);
            return num_stolen_threads;
        }

        std::int64_t get_num_stolen_to_staged(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    num_stolen_threads +=
                        queues_[i]->get_num_stolen_to_staged(reset);
                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread]->get_num_stolen_to_staged(reset);
            return num_stolen_threads;
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        void abort_all_suspended_threads() override
        {
            for (std::size_t i = 0; i != queues_.size(); ++i)
                queues_[i]->abort_all_suspended_threads();
        }

        ///////////////////////////////////////////////////////////////////////
        bool cleanup_terminated(bool delete_all) override
        {
            bool empty = true;
            for (std::size_t i = 0; i != queues_.size(); ++i)
                empty = queues_[i]->cleanup_terminated(delete_all) && empty;

            return empty;
        }

        bool cleanup_terminated(
            std::size_t num_thread, bool delete_all) override
        {
            return queues_[num_thread]->cleanup_terminated(delete_all);
        }

        ///////////////////////////////////////////////////////////////////////
        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(thread_init_data& data, thread_id_ref_type* id,
            error_code& ec) override
        {
            std::size_t num_thread =
                data.schedulehint.mode == thread_schedule_hint_mode::thread ?
                data.schedulehint.hint :
                static_cast<std::size_t>(-1);

            std::size_t const queue_size = queues_.size();

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_++ % queue_size;
            }
            else if (num_thread >= queue_size)
            {
                num_thread %= queue_size;
            }

            num_thread = select_active_pu(num_thread);

            HPX_ASSERT(num_thread < queue_size);
            queues_[num_thread]->create_thread(data, id, ec);

            LTM_(debug)
                .format("local_queue_scheduler::create_thread: pool({}), "
                        "scheduler({}), worker_thread({}), thread({})",
                    *this->get_parent_pool(), *this, num_thread,
                    id ? *id : invalid_thread_id)
#ifdef HPX_HAVE_THREAD_DESCRIPTION
                .format(", description({})", data.description)
#endif
                ;
        }

        // Return the next thread to be executed, return false if none is
        // available
        bool get_next_thread(std::size_t num_thread, bool running,
            threads::thread_id_ref_type& thrd, bool /*enable_stealing*/)
        {
            std::size_t const queues_size = queues_.size();

            {
                HPX_ASSERT(num_thread < queues_size);

                thread_queue_type* q = queues_[num_thread];
                bool const result = q->get_next_thread(thrd);

                q->increment_num_pending_accesses();
                if (result)
                    return true;
                q->increment_num_pending_misses();

                // Give up, we should have work to convert.
                if (q->get_staged_queue_length(std::memory_order_relaxed) != 0)
                {
                    return false;
                }
            }

            if (!running)
            {
                return false;
            }

            bool const numa_stealing = has_scheduler_mode(
                policies::scheduler_mode::enable_stealing_numa, num_thread);
            if (!numa_stealing)
            {
                // steal work items: first try to steal from other cores in
                // the same NUMA node
                std::size_t const pu_number =
                    affinity_data_.get_pu_num(num_thread);

                // we know that the MIC has one NUMA domain only
#if !defined(HPX_NATIVE_MIC)
                if (test(steals_in_numa_domain_, pu_number))    //-V600 //-V111
#endif
                {
                    mask_cref_type this_numa_domain =
                        numa_domain_masks_[num_thread];

                    // steal thread from other queue
                    for (std::size_t i = 1; i != queues_size; ++i)
                    {
                        // FIXME: Do a better job here.
                        std::size_t const idx = (i + num_thread) % queues_size;

                        HPX_ASSERT(idx != num_thread);

                        if (std::size_t const pu_num =
                                affinity_data_.get_pu_num(idx);
                            !test(this_numa_domain,
                                pu_num))    //-V560 //-V600 //-V111
                        {
                            continue;
                        }

                        thread_queue_type* q = queues_[idx];
                        if (q->get_next_thread(thrd, running))
                        {
                            q->increment_num_stolen_from_pending();
                            queues_[num_thread]
                                ->increment_num_stolen_to_pending();
                            return true;
                        }
                    }
                }

                // we know that the MIC has one NUMA domain only)
#if !defined(HPX_NATIVE_MIC)
                // if nothing found, ask everybody else
                if (test(steals_outside_numa_domain_,
                        pu_number))    //-V600 //-V111
                {
                    mask_cref_type numa_domain =
                        outside_numa_domain_masks_[num_thread];

                    // steal thread from other queue
                    for (std::size_t i = 1; i != queues_size; ++i)
                    {
                        // FIXME: Do a better job here.
                        std::size_t const idx = (i + num_thread) % queues_size;

                        HPX_ASSERT(idx != num_thread);

                        std::size_t const pu_num =
                            affinity_data_.get_pu_num(idx);
                        if (!test(numa_domain,
                                pu_num))    //-V560 //-V600 //-V111
                            continue;

                        thread_queue_type* q = queues_[idx];
                        if (q->get_next_thread(thrd, running))
                        {
                            q->increment_num_stolen_from_pending();
                            queues_[num_thread]
                                ->increment_num_stolen_to_pending();
                            return true;
                        }
                    }
                }
#endif
            }

            else    // not NUMA-sensitive - numa stealing ok
            {
                for (std::size_t i = 1; i != queues_size; ++i)
                {
                    // FIXME: Do a better job here.
                    std::size_t const idx = (i + num_thread) % queues_size;

                    HPX_ASSERT(idx != num_thread);

                    thread_queue_type* q = queues_[idx];
                    if (q->get_next_thread(thrd, running))
                    {
                        q->increment_num_stolen_from_pending();
                        queues_[num_thread]->increment_num_stolen_to_pending();
                        return true;
                    }
                }
            }

            return false;
        }

        // Schedule the passed thread
        void schedule_thread(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint, bool allow_fallback,
            thread_priority /* priority */ = thread_priority::default_) override
        {
            // NOTE: This scheduler ignores NUMA hints.
            auto num_thread = static_cast<std::size_t>(-1);
            if (schedulehint.mode ==
                thread_schedule_hint_mode::thread)    //-V1051
            {
                num_thread = schedulehint.hint;
            }
            else
            {
                allow_fallback = false;
            }

            std::size_t const queue_size = queues_.size();

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_++ % queue_size;
            }
            else if (num_thread >= queue_size)
            {
                num_thread %= queue_size;
            }

            num_thread = select_active_pu(num_thread, allow_fallback);

            HPX_ASSERT(get_thread_id_data(thrd)->get_scheduler_base() == this);

            HPX_ASSERT(num_thread < queues_.size());

            LTM_(debug).format("local_queue_scheduler::schedule_thread: "
                               "pool({}), scheduler({}), worker_thread({}), "
                               "thread({}), description({})",
                *this->get_parent_pool(), *this, num_thread,
                get_thread_id_data(thrd)->get_thread_id(),
                get_thread_id_data(thrd)->get_description());

            queues_[num_thread]->schedule_thread(thrd);
        }

        void schedule_thread_last(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint, bool allow_fallback,
            thread_priority /* priority */ = thread_priority::default_) override
        {
            // NOTE: This scheduler ignores NUMA hints.
            auto num_thread = static_cast<std::size_t>(-1);
            if (schedulehint.mode ==
                thread_schedule_hint_mode::thread)    //-V1051
            {
                num_thread = schedulehint.hint;
            }
            else
            {
                allow_fallback = false;
            }

            std::size_t const queue_size = queues_.size();

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_++ % queue_size;
            }
            else if (num_thread >= queue_size)
            {
                num_thread %= queue_size;
            }

            num_thread = select_active_pu(num_thread, allow_fallback);

            HPX_ASSERT(get_thread_id_data(thrd)->get_scheduler_base() == this);

            HPX_ASSERT(num_thread < queues_.size());
            queues_[num_thread]->schedule_thread(thrd, true);
        }

        // Destroy the passed thread as it has been terminated
        void destroy_thread(threads::thread_data* thrd) override
        {
            HPX_ASSERT(thrd->get_scheduler_base() == this);
            thrd->get_queue<thread_queue_type>().destroy_thread(thrd);
        }

        ///////////////////////////////////////////////////////////////////////
        // This returns the current length of the queues (work items and new items)
        std::int64_t get_queue_length(std::size_t num_thread) const override
        {
            // Return queue length of one specific queue.
            std::int64_t count = 0;
            if (static_cast<std::size_t>(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < queues_.size());

                return queues_[num_thread]->get_queue_length();
            }

            for (std::size_t i = 0; i != queues_.size(); ++i)
                count += queues_[i]->get_queue_length();

            return count;
        }

        ///////////////////////////////////////////////////////////////////////
        // Queries the current thread count of the queues.
        std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown,
            thread_priority priority = thread_priority::default_,
            std::size_t num_thread = static_cast<std::size_t>(-1),
            bool /* reset */ = false) const override
        {
            // Return thread count of one specific queue.
            std::int64_t count = 0;
            if (static_cast<std::size_t>(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < queues_.size());

                switch (priority)
                {
                case thread_priority::default_:
                case thread_priority::low:
                case thread_priority::initially_bound:
                case thread_priority::bound:
                case thread_priority::normal:
                case thread_priority::boost:
                case thread_priority::high:
                    [[fallthrough]];
                case thread_priority::high_recursive:
                    return queues_[num_thread]->get_thread_count(state);

                default:
                    [[fallthrough]];
                case thread_priority::unknown:
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "local_queue_scheduler::get_thread_count",
                        "unknown thread priority value "
                        "(thread_priority::unknown)");
                }
                }
            }

            // Return the cumulative count for all queues.
            switch (priority)
            {
            case thread_priority::default_:
            case thread_priority::low:
            case thread_priority::initially_bound:
            case thread_priority::bound:
            case thread_priority::normal:
            case thread_priority::boost:
            case thread_priority::high:
                [[fallthrough]];
            case thread_priority::high_recursive:
            {
                for (std::size_t i = 0; i != queues_.size(); ++i)
                    count += queues_[i]->get_thread_count(state);
                break;
            }

            default:
                [[fallthrough]];
            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_queue_scheduler::get_thread_count",
                    "unknown thread priority value "
                    "(thread_priority::unknown)");
            }
            }
            return count;
        }

        // Queries whether a given core is idle
        bool is_core_idle(std::size_t num_thread) const override
        {
            return queues_[num_thread]->get_queue_length() == 0;
        }

        ///////////////////////////////////////////////////////////////////////
        // Enumerate matching threads from all queues
        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state =
                thread_schedule_state::unknown) const override
        {
            bool result = true;
            for (std::size_t i = 0; i != queues_.size(); ++i)
            {
                result = result && queues_[i]->enumerate_threads(f, state);
            }
            return result;
        }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        ///////////////////////////////////////////////////////////////////////
        // Queries the current average thread wait time of the queues.
        std::int64_t get_average_thread_wait_time(
            std::size_t num_thread = std::size_t(-1)) const override
        {
            // Return average thread wait time of one specific queue.
            std::uint64_t wait_time = 0;
            std::uint64_t count = 0;
            if (std::size_t(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < queues_.size());

                wait_time +=
                    queues_[num_thread]->get_average_thread_wait_time();
                return wait_time / (count + 1);
            }

            for (std::size_t i = 0; i != queues_.size(); ++i)
            {
                wait_time += queues_[i]->get_average_thread_wait_time();
                ++count;
            }

            return wait_time / (count + 1);
        }

        ///////////////////////////////////////////////////////////////////////
        // Queries the current average task wait time of the queues.
        std::int64_t get_average_task_wait_time(
            std::size_t num_thread = std::size_t(-1)) const override
        {
            // Return average task wait time of one specific queue.
            std::uint64_t wait_time = 0;
            std::uint64_t count = 0;
            if (std::size_t(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < queues_.size());

                wait_time += queues_[num_thread]->get_average_task_wait_time();
                return wait_time / (count + 1);
            }

            for (std::size_t i = 0; i != queues_.size(); ++i)
            {
                wait_time += queues_[i]->get_average_task_wait_time();
                ++count;
            }

            return wait_time / (count + 1);
        }
#endif

        // This is a function which gets called periodically by the thread
        // manager to allow for maintenance tasks to be executed in the
        // scheduler. Returns true if the OS thread calling this function has to
        // be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(std::size_t num_thread, bool running,
            std::int64_t& idle_loop_count, bool /* enable_stealing */,
            std::size_t& added, thread_id_ref_type* = nullptr)
        {
            std::size_t const queues_size = queues_.size();
            HPX_ASSERT(num_thread < queues_.size());

            added = 0;

            bool result = true;

            result =
                queues_[num_thread]->wait_or_add_new(running, added) && result;
            if (0 != added)
                return result;

            // Check if we have been disabled
            if (!running)
            {
                return true;
            }

            // limited or no stealing across domains
            if (!has_scheduler_mode(
                    policies::scheduler_mode::enable_stealing_numa, num_thread))
            {
                // steal work items: first try to steal from other cores in the
                // same NUMA node
                std::size_t const pu_number =
                    affinity_data_.get_pu_num(num_thread);

                // we know that the MIC has one NUMA domain only
#if !defined(HPX_NATIVE_MIC)
                if (test(steals_in_numa_domain_, pu_number))    //-V600 //-V111
#endif
                {
                    mask_cref_type numa_domain_mask =
                        numa_domain_masks_[num_thread];
                    for (std::size_t i = 1; i != queues_size; ++i)
                    {
                        // FIXME: Do a better job here.
                        std::size_t const idx = (i + num_thread) % queues_size;

                        HPX_ASSERT(idx != num_thread);

                        if (!test(numa_domain_mask,
                                affinity_data_.get_pu_num(idx)))    //-V600
                        {
                            continue;
                        }
                        result = queues_[num_thread]->wait_or_add_new(
                                     running, added, queues_[idx]) &&
                            result;
                        if (0 != added)
                        {
                            queues_[idx]->increment_num_stolen_from_staged(
                                added);
                            queues_[num_thread]->increment_num_stolen_to_staged(
                                added);
                            return result;
                        }
                    }
                }

                // we know that the MIC has one NUMA domain only)
#if !defined(HPX_NATIVE_MIC)
                // if nothing found, ask everybody else
                if (test(steals_outside_numa_domain_,
                        pu_number))    //-V600 //-V111
                {
                    mask_cref_type numa_domain_mask =
                        outside_numa_domain_masks_[num_thread];
                    for (std::size_t i = 1; i != queues_size; ++i)
                    {
                        // FIXME: Do a better job here.
                        std::size_t const idx = (i + num_thread) % queues_size;

                        HPX_ASSERT(idx != num_thread);

                        if (!test(numa_domain_mask,
                                affinity_data_.get_pu_num(idx)))    //-V600
                        {
                            continue;
                        }

                        result = queues_[num_thread]->wait_or_add_new(
                                     running, added, queues_[idx]) &&
                            result;
                        if (0 != added)
                        {
                            queues_[idx]->increment_num_stolen_from_staged(
                                added);
                            queues_[num_thread]->increment_num_stolen_to_staged(
                                added);
                            return result;
                        }
                    }
                }
#endif
            }
            else    // not NUMA-sensitive : numa stealing ok
            {
                for (std::size_t i = 1; i != queues_size; ++i)
                {
                    // FIXME: Do a better job here.
                    std::size_t const idx = (i + num_thread) % queues_size;

                    HPX_ASSERT(idx != num_thread);

                    result = queues_[num_thread]->wait_or_add_new(
                                 running, added, queues_[idx]) &&
                        result;
                    if (0 != added)
                    {
                        queues_[idx]->increment_num_stolen_from_staged(added);
                        queues_[num_thread]->increment_num_stolen_to_staged(
                            added);
                        return result;
                    }
                }
            }

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
            // no new work is available, are we deadlocked?
            if (HPX_UNLIKELY(get_minimal_deadlock_detection_enabled() &&
                    LHPX_ENABLED(error)))
            {
                bool suspended_only = true;

                for (std::size_t i = 0; suspended_only && i != queues_.size();
                    ++i)
                {
                    suspended_only = queues_[i]->dump_suspended_threads(
                        i, idle_loop_count, running);
                }

                if (HPX_UNLIKELY(suspended_only))
                {
                    LTM_(warning).format(
                        "pool({}), scheduler({}), queue({}): no new work "
                        "available, are we deadlocked?",
                        *this->get_parent_pool(), *this, num_thread);
                }
            }
#else
            HPX_UNUSED(idle_loop_count);
#endif

            return result;
        }

        ///////////////////////////////////////////////////////////////////////
        void on_start_thread(std::size_t num_thread) override
        {
            hpx::threads::detail::set_local_thread_num_tss(num_thread);
            hpx::threads::detail::set_thread_pool_num_tss(
                parent_pool_->get_pool_id().index());

            if (nullptr == queues_[num_thread])
            {
                queues_[num_thread] = new thread_queue_type(thread_queue_init_);
            }

            queues_[num_thread]->on_start_thread(num_thread);

            auto const& topo = create_topology();

            // pre-calculate certain constants for the given thread number
            std::size_t const num_pu = affinity_data_.get_pu_num(num_thread);
            mask_cref_type machine_mask = topo.get_machine_affinity_mask();
            mask_cref_type core_mask = topo.get_thread_affinity_mask(num_pu);
            mask_cref_type node_mask = topo.get_numa_node_affinity_mask(num_pu);

            if (any(core_mask) && any(node_mask))
            {
#if !defined(HPX_NATIVE_MIC)    // we know that the MIC has one NUMA domain only
                set(steals_in_numa_domain_, num_pu);
#endif
                numa_domain_masks_[num_thread] = node_mask;
            }

            // we allow the thread on the boundary of the NUMA domain to steal
            mask_type first_mask = mask_type();
            resize(first_mask, mask_size(core_mask));

            std::size_t const first = find_first(node_mask);
            if (first != static_cast<std::size_t>(-1))
                set(first_mask, first);
            else
                first_mask = core_mask;

            if (has_scheduler_mode(
                    policies::scheduler_mode::enable_stealing_numa,
                    num_thread) &&
                any(first_mask & core_mask))
            {
#if !defined(HPX_NATIVE_MIC)    // we know that the MIC has one NUMA domain only
                set(steals_outside_numa_domain_, num_pu);
#endif
                outside_numa_domain_masks_[num_thread] =
                    not_(node_mask) & machine_mask;
            }
        }

        void on_stop_thread(std::size_t num_thread) override
        {
            if (num_thread < queues_.size())
            {
                queues_[num_thread]->on_stop_thread(num_thread);
            }
        }

        void on_error(
            std::size_t num_thread, std::exception_ptr const& e) override
        {
            if (num_thread < queues_.size())
            {
                queues_[num_thread]->on_error(num_thread, e);
            }
        }

    protected:
        std::vector<thread_queue_type*> queues_;
        std::atomic<std::size_t> curr_queue_;

        detail::affinity_data const& affinity_data_;

#if !defined(HPX_NATIVE_MIC)    // we know that the MIC has one NUMA domain only
        mask_type steals_in_numa_domain_ = mask_type();
        mask_type steals_outside_numa_domain_ = mask_type();
#endif
        std::vector<mask_type> numa_domain_masks_;
        std::vector<mask_type> outside_numa_domain_masks_;
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
