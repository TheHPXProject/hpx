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
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/schedulers/deadlock_detection.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>
#include <hpx/schedulers/queue_helpers.hpp>
#include <hpx/schedulers/thread_queue.hpp>

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT using default_local_priority_queue_scheduler_terminated_queue =
        lockfree_lifo;
#else
    HPX_CXX_CORE_EXPORT using default_local_priority_queue_scheduler_terminated_queue =
        lockfree_fifo;
#endif

    ///////////////////////////////////////////////////////////////////////////
    /// The local_priority_queue_scheduler maintains exactly one queue of work
    /// items (threads) per OS thread, where this OS thread pulls its next work
    /// from. Additionally, it maintains separate queues: several for high
    /// priority threads and one for low priority threads. High priority threads
    /// are executed by the first N OS threads before any other work is
    /// executed. Low priority threads are executed by the last OS thread
    /// whenever no other work is available.
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_local_priority_queue_scheduler_terminated_queue>
    class local_priority_queue_scheduler : public scheduler_base
    {
    public:
        using has_periodic_maintenance = std::false_type;

        using thread_queue_type = thread_queue<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing>;

        // the scheduler type takes two initialization parameters:
        //    the number of queues
        //    the number of high priority queues
        //    the max-count per queue
        struct init_parameter
        {
            init_parameter(std::size_t const num_queues,
                detail::affinity_data const& affinity_data,
                std::size_t const num_high_priority_queues =
                    static_cast<std::size_t>(-1),
                thread_queue_init_parameters const& thread_queue_init =
                    thread_queue_init_parameters{},
                char const* description = "local_priority_queue_scheduler",
                numa_sensitive_mode numa_sensitive =
                    numa_sensitive_mode::boundary) noexcept
              : num_queues_(num_queues)
              , num_high_priority_queues_(
                    num_high_priority_queues == static_cast<std::size_t>(-1) ?
                        num_queues :
                        num_high_priority_queues)
              , thread_queue_init_(thread_queue_init)
              , affinity_data_(affinity_data)
              , description_(description)
              , numa_sensitive_(numa_sensitive)
            {
            }

            init_parameter(std::size_t const num_queues,
                detail::affinity_data const& affinity_data,
                char const* description) noexcept
              : num_queues_(num_queues)
              , num_high_priority_queues_(num_queues)
              , affinity_data_(affinity_data)
              , description_(description)
              , numa_sensitive_(numa_sensitive_mode::boundary)
            {
            }

            std::size_t num_queues_;
            std::size_t num_high_priority_queues_;
            thread_queue_init_parameters thread_queue_init_;
            detail::affinity_data const& affinity_data_;
            char const* description_;
            numa_sensitive_mode numa_sensitive_;
        };
        using init_parameter_type = init_parameter;

        explicit local_priority_queue_scheduler(init_parameter_type const& init,
            bool const deferred_initialization = true)
          : scheduler_base(
                init.num_queues_, init.description_, init.thread_queue_init_)
          , curr_queue_(0)
          , affinity_data_(init.affinity_data_)
          , numa_sensitive_(init.numa_sensitive_)
          , num_queues_(init.num_queues_)
          , num_high_priority_queues_(init.num_high_priority_queues_)
          , queues_(num_queues_)
          , thread_numa_domain_(num_queues_, 0)
          , victims_(num_queues_)
          , low_priority_queue_(thread_queue_init_)
        {
            // Start stealing staged tasks soon after a worker ran out of work
            // (see wait_or_add_new). This takes load off the pending queue of
            // a busy worker, which all idle workers would otherwise pop from
            // concurrently.
            set_steal_staged_threshold(
                thread_queue_init_.idle_loop_count_steal_staged_);

            if (!deferred_initialization)
            {
                HPX_ASSERT(num_queues_ != 0);
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    queues_[i].bound_queue_ =
                        new thread_queue_type(thread_queue_init_);
                    queues_[i].queue_ =
                        new thread_queue_type(thread_queue_init_);
                }

                HPX_ASSERT(num_high_priority_queues_ != 0);
                HPX_ASSERT(num_high_priority_queues_ <= num_queues_);
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    queues_[i].high_priority_queue_ =
                        new thread_queue_type(thread_queue_init_);
                }
                for (std::size_t i = num_high_priority_queues_;
                    i != num_queues_; ++i)
                {
                    queues_[i].high_priority_queue_ = nullptr;
                }
            }
        }

        ~local_priority_queue_scheduler() override
        {
            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                delete queues_[i].queue_;
                delete queues_[i].bound_queue_;
            }

            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                delete queues_[i].high_priority_queue_;
            }
        }

        local_priority_queue_scheduler(
            local_priority_queue_scheduler const&) = delete;
        local_priority_queue_scheduler(
            local_priority_queue_scheduler&&) = delete;
        local_priority_queue_scheduler& operator=(
            local_priority_queue_scheduler const&) = delete;
        local_priority_queue_scheduler& operator=(
            local_priority_queue_scheduler&&) = delete;

        static std::string_view get_scheduler_name()
        {
            return "local_priority_queue_scheduler";
        }

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool reset) override
        {
            std::uint64_t time = 0;

            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                time +=
                    queues_[i].high_priority_queue_->get_creation_time(reset);
            }

            time += low_priority_queue_.get_creation_time(reset);

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                time += queues_[i].bound_queue_->get_creation_time(reset);
                time += queues_[i].queue_->get_creation_time(reset);
            }
            return time;
        }

        std::uint64_t get_cleanup_time(bool reset) override
        {
            std::uint64_t time = 0;

            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                time +=
                    queues_[i].high_priority_queue_->get_cleanup_time(reset);
            }

            time += low_priority_queue_.get_cleanup_time(reset);

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                time += queues_[i].bound_queue_->get_cleanup_time(reset);
                time += queues_[i].queue_->get_cleanup_time(reset);
            }
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
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    num_pending_misses +=
                        queues_[i].high_priority_queue_->get_num_pending_misses(
                            reset);
                }
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    num_pending_misses +=
                        queues_[i].bound_queue_->get_num_pending_misses(reset);
                    num_pending_misses +=
                        queues_[i].queue_->get_num_pending_misses(reset);
                }
                num_pending_misses +=
                    low_priority_queue_.get_num_pending_misses(reset);

                return num_pending_misses;
            }

            num_pending_misses +=
                queues_[num_thread].bound_queue_->get_num_pending_misses(reset);
            num_pending_misses +=
                queues_[num_thread].queue_->get_num_pending_misses(reset);

            if (num_thread < num_high_priority_queues_)
            {
                num_pending_misses +=
                    queues_[num_thread]
                        .high_priority_queue_->get_num_pending_misses(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                num_pending_misses +=
                    low_priority_queue_.get_num_pending_misses(reset);
            }
            return num_pending_misses;
        }

        std::int64_t get_num_pending_accesses(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_pending_accesses = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    num_pending_accesses +=
                        queues_[i]
                            .high_priority_queue_->get_num_pending_accesses(
                                reset);
                }
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    num_pending_accesses +=
                        queues_[i].bound_queue_->get_num_pending_accesses(
                            reset);
                    num_pending_accesses +=
                        queues_[i].queue_->get_num_pending_accesses(reset);
                }
                num_pending_accesses +=
                    low_priority_queue_.get_num_pending_accesses(reset);

                return num_pending_accesses;
            }

            num_pending_accesses +=
                queues_[num_thread].bound_queue_->get_num_pending_accesses(
                    reset);
            num_pending_accesses +=
                queues_[num_thread].queue_->get_num_pending_accesses(reset);

            if (num_thread < num_high_priority_queues_)
            {
                num_pending_accesses +=
                    queues_[num_thread]
                        .high_priority_queue_->get_num_pending_accesses(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                num_pending_accesses +=
                    low_priority_queue_.get_num_pending_accesses(reset);
            }
            return num_pending_accesses;
        }

        std::int64_t get_num_stolen_from_pending(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i]
                            .high_priority_queue_->get_num_stolen_from_pending(
                                reset);
                }
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i].queue_->get_num_stolen_from_pending(reset);
                }
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_from_pending(reset);

                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread].queue_->get_num_stolen_from_pending(reset);

            if (num_thread < num_high_priority_queues_)
            {
                num_stolen_threads +=
                    queues_[num_thread]
                        .high_priority_queue_->get_num_stolen_from_pending(
                            reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_from_pending(reset);
            }
            return num_stolen_threads;
        }

        std::int64_t get_num_stolen_to_pending(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i]
                            .high_priority_queue_->get_num_stolen_to_pending(
                                reset);
                }
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i].queue_->get_num_stolen_to_pending(reset);
                }
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_to_pending(reset);

                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread].queue_->get_num_stolen_to_pending(reset);

            if (num_thread < num_high_priority_queues_)
            {
                num_stolen_threads +=
                    queues_[num_thread]
                        .high_priority_queue_->get_num_stolen_to_pending(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_to_pending(reset);
            }
            return num_stolen_threads;
        }

        std::int64_t get_num_stolen_from_staged(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i]
                            .high_priority_queue_->get_num_stolen_from_staged(
                                reset);
                }

                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i].queue_->get_num_stolen_from_staged(reset);
                }
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_from_staged(reset);

                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread].queue_->get_num_stolen_from_staged(reset);

            if (num_thread < num_high_priority_queues_)
            {
                num_stolen_threads +=
                    queues_[num_thread]
                        .high_priority_queue_->get_num_stolen_from_staged(
                            reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_from_staged(reset);
            }
            return num_stolen_threads;
        }

        std::int64_t get_num_stolen_to_staged(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t num_stolen_threads = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i]
                            .high_priority_queue_->get_num_stolen_to_staged(
                                reset);
                }
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    num_stolen_threads +=
                        queues_[i].queue_->get_num_stolen_to_staged(reset);
                }
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_to_staged(reset);

                return num_stolen_threads;
            }

            num_stolen_threads +=
                queues_[num_thread].queue_->get_num_stolen_to_staged(reset);

            if (num_thread < num_high_priority_queues_)
            {
                num_stolen_threads +=
                    queues_[num_thread]
                        .high_priority_queue_->get_num_stolen_to_staged(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                num_stolen_threads +=
                    low_priority_queue_.get_num_stolen_to_staged(reset);
            }
            return num_stolen_threads;
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        void abort_all_suspended_threads() override
        {
            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                queues_[i].bound_queue_->abort_all_suspended_threads();
                queues_[i].queue_->abort_all_suspended_threads();
            }

            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                queues_[i].high_priority_queue_->abort_all_suspended_threads();
            }
            low_priority_queue_.abort_all_suspended_threads();
        }

        ///////////////////////////////////////////////////////////////////////
        bool cleanup_terminated(bool delete_all) override
        {
            bool empty = true;

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                empty =
                    queues_[i].bound_queue_->cleanup_terminated(delete_all) &&
                    empty;
                empty =
                    queues_[i].queue_->cleanup_terminated(delete_all) && empty;
            }
            if (!delete_all)
                return empty;

            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                empty = queues_[i].high_priority_queue_->cleanup_terminated(
                            delete_all) &&
                    empty;
            }
            empty = low_priority_queue_.cleanup_terminated(delete_all) && empty;

            return empty;
        }

        bool cleanup_terminated(
            std::size_t num_thread, bool delete_all) override
        {
            bool empty = true;

            empty = queues_[num_thread].bound_queue_->cleanup_terminated(
                        delete_all) &&
                empty;
            empty =
                queues_[num_thread].queue_->cleanup_terminated(delete_all) &&
                empty;
            if (!delete_all)
                return empty;

            if (num_thread < num_high_priority_queues_)
            {
                empty =
                    queues_[num_thread]
                        .high_priority_queue_->cleanup_terminated(delete_all) &&
                    empty;
            }
            return empty;
        }

        ///////////////////////////////////////////////////////////////////////
        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(thread_init_data& data, thread_id_ref_type* id,
            error_code& ec) override
        {
            std::size_t num_thread = static_cast<std::size_t>(-1);
            thread_priority priority = data.priority;

            // If the initial priority is initially_bound, promote it to
            // bound for the first scheduling step, regardless of hint mode.
            if (data.priority == thread_priority::initially_bound)
            {
                priority = thread_priority::bound;
                data.priority = thread_priority::normal;
            }

            if (data.schedulehint.mode() == thread_schedule_hint_mode::thread)
            {
                HPX_ASSERT(data.schedulehint.hint >= 0);
                num_thread = data.schedulehint.hint;
            }
            else if (data.schedulehint.mode() ==
                thread_schedule_hint_mode::numa)
            {
                // Route to a worker thread on the requested NUMA domain.
                // Fall back to round-robin if the domain has no threads.
                HPX_ASSERT(data.schedulehint.hint >= 0);
                auto const& topo = create_topology();
                std::size_t num_nodes = topo.get_number_of_numa_nodes();
                if (num_nodes == 0)
                    num_nodes = 1;
                std::size_t const requested_domain =
                    static_cast<std::size_t>(data.schedulehint.hint) %
                    num_nodes;

                std::size_t const start = curr_queue_.data_++ % num_queues_;
                for (std::size_t i = 0; i < num_queues_; ++i)
                {
                    std::size_t const idx = (start + i) % num_queues_;
                    if (thread_numa_domain_[idx] == requested_domain)
                    {
                        num_thread = idx;
                        break;
                    }
                }
                data.schedulehint.mode(thread_schedule_hint_mode::thread);
            }
            else
            {
                data.schedulehint.mode(thread_schedule_hint_mode::thread);
            }

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_.data_++ % num_queues_;
            }
            else if (num_thread >= num_queues_)
            {
                num_thread %= num_queues_;
            }
            num_thread = select_active_pu(num_thread);

            data.schedulehint.hint = static_cast<std::int16_t>(num_thread);

            // now create the thread
            switch (priority)
            {
            case thread_priority::boost:
                data.priority = thread_priority::normal;
                [[fallthrough]];
            case thread_priority::high_recursive:
                [[fallthrough]];
            case thread_priority::high:
            {
                std::size_t num = num_thread % num_high_priority_queues_;
                queues_[num].high_priority_queue_->create_thread(data, id, ec);

                LTM_(debug)
                    .format("local_priority_queue_scheduler::create_thread, "
                            "high priority queue: pool({}), scheduler({}), "
                            "worker_thread({}), thread({}), priority({})",
                        *this->get_parent_pool(), *this, num,
                        id ? *id : invalid_thread_id, data.priority)
#ifdef HPX_HAVE_THREAD_DESCRIPTION
                    .format(", description({})", data.description)
#endif
                    ;
                break;
            }

            case thread_priority::low:
            {
                low_priority_queue_.create_thread(data, id, ec);

                LTM_(debug)
                    .format("local_priority_queue_scheduler::create_thread, "
                            "low priority queue: pool({}), scheduler({}), "
                            "thread({}), priority({})",
                        *this->get_parent_pool(), *this,
                        id ? *id : invalid_thread_id, data.priority)
#ifdef HPX_HAVE_THREAD_DESCRIPTION
                    .format(", description({})", data.description)
#endif
                    ;
                break;
            }

            case thread_priority::bound:
                [[fallthrough]];
            case thread_priority::initially_bound:
            {
                HPX_ASSERT(num_thread < num_queues_);
                queues_[num_thread].bound_queue_->create_thread(data, id, ec);

                LTM_(debug)
                    .format("local_priority_queue_scheduler::create_thread, "
                            "bound priority queue: pool({}), scheduler({}), "
                            "worker_thread({}), thread({}), priority({})",
                        *this->get_parent_pool(), *this, num_thread,
                        id ? *id : invalid_thread_id, data.priority)
#ifdef HPX_HAVE_THREAD_DESCRIPTION
                    .format(", description({})", data.description)
#endif
                    ;
                break;
            }

            case thread_priority::default_:
                [[fallthrough]];
            case thread_priority::normal:
            {
                HPX_ASSERT(num_thread < num_queues_);
                queues_[num_thread].queue_->create_thread(data, id, ec);

                LTM_(debug)
                    .format("local_priority_queue_scheduler::create_thread, "
                            "normal priority queue: pool({}), scheduler({}), "
                            "worker_thread({}), thread({}), priority({})",
                        *this->get_parent_pool(), *this, num_thread,
                        id ? *id : invalid_thread_id, data.priority)
#ifdef HPX_HAVE_THREAD_DESCRIPTION
                    .format(", description({})", data.description)
#endif
                    ;
                break;
            }

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_priority_queue_scheduler::create_thread",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
        }

        template <typename Body>
        static bool for_each_circular(
            std::size_t const start_idx, std::size_t const count, Body&& body)
        {
            for (std::size_t i = start_idx; i < count; ++i)
            {
                if (body(i))
                    return true;
            }
            for (std::size_t i = 0; i < start_idx; ++i)
            {
                if (body(i))
                    return true;
            }
            return false;
        }

        bool attempt_stealing_pending(std::size_t num_thread,
            threads::thread_id_ref_type& thrd,
            [[maybe_unused]] thread_queue_type* this_high_priority_queue,
            [[maybe_unused]] thread_queue_type* this_queue,
            std::int64_t const min_tasks)
        {
            // Helper to increment steal counters only when enabled.
            constexpr auto increment_counters =
                []([[maybe_unused]] thread_queue_type* from_q,
                    [[maybe_unused]] thread_queue_type* to_q) noexcept {
#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                    from_q->increment_num_stolen_from_pending();
                    to_q->increment_num_stolen_to_pending();
#endif
                };

            std::vector<std::size_t> const& victim_threads =
                victims_[num_thread].data_.victim_threads_;
            std::size_t const victim_count = victim_threads.size();
            if (victim_count == 0)
                return false;

            auto const steal = [&](std::size_t idx) -> bool {
                HPX_ASSERT(idx != num_thread);

                // suppress contracts warning about 'num_thread' not implicitly
                // captured by a contract assertion
                (void) num_thread;

                if (thread_queue_type* q = queues_[idx].queue_;
                    q->get_next_thread(thrd, true, true,
                        victim_min_tasks(num_thread, idx, q, min_tasks, false)))
                {
                    increment_counters(q, this_queue);
                    hpx::tracing::work_stolen(num_thread, idx, thrd);
                    return true;
                }
                return false;
            };

            std::size_t const num_high = num_high_priority_queues_;
            auto const steal_hp = [&](std::size_t idx) -> bool {
                HPX_ASSERT(idx != num_thread);

                // suppress contracts warning about 'num_thread' not implicitly
                // captured by a contract assertion
                (void) num_thread;

                if (idx < num_high)
                {
                    if (thread_queue_type* q =
                            queues_[idx].high_priority_queue_;
                        q->get_next_thread(thrd, true, true))
                    {
                        increment_counters(q, this_high_priority_queue);
                        hpx::tracing::work_stolen(num_thread, idx, thrd);
                        return true;
                    }
                }
                return steal(idx);
            };

            std::size_t const offset =
                victims_[num_thread].data_.victim_offset_++;
            if (num_thread < num_high)
            {
                return for_each_circular(offset % victim_count, victim_count,
                    [&](std::size_t const i) {
                        return steal_hp(victim_threads[i]);
                    });
            }
            else
            {
                return for_each_circular(offset % victim_count, victim_count,
                    [&](std::size_t const i) {
                        return steal(victim_threads[i]);
                    });
            }
        }

        // Return the next thread to be executed, return false if none is
        // available
        bool get_next_thread(std::size_t num_thread, bool const running,
            threads::thread_id_ref_type& thrd, bool enable_stealing)
        {
            bool const found = get_next_thread_impl(
                num_thread, running, thrd, enable_stealing);
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
            if (found)
                this->clear_free_pass(num_thread);
#endif
            return found;
        }

        // may this worker ignore the early stealing minima (once after it
        // was woken by a notification)?
        bool has_free_pass(
            [[maybe_unused]] std::size_t num_thread) const noexcept
        {
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
            return this->scheduler_base::has_free_pass(num_thread);
#else
            return false;
#endif
        }

        // Number of consecutive looks at a short, non-empty queue of a victim
        // after which the early minimum for that victim is ignored.
        static constexpr std::uint32_t steal_looks = 16;

        // The early minimum for one victim, lifted once its queue was seen
        // short but not empty often enough in a row (seeing it empty starts
        // over), i.e. its owner doesn't get to it:
        // - pending tasks: steal_looks looks by this thief
        // - staged tasks: the looks of all thieves are counted on the victim,
        //   steal_looks / queue length are needed, and only the one thief
        //   that resets the count may ignore the minimum
        std::int64_t victim_min_tasks(std::size_t num_thread, std::size_t idx,
            thread_queue_type* q, std::int64_t const min_tasks,
            bool const staged)
        {
            if (min_tasks <= 1)
                return min_tasks;

            if (staged)
            {
                std::int64_t const len =
                    q->get_staged_queue_length(std::memory_order_relaxed);
                auto& c = q->get_staged_looks();
                if (len == 0)
                {
                    // write the shared counter only if it is not zero yet
                    if (c.load(std::memory_order_relaxed) != 0)
                        c.store(0, std::memory_order_relaxed);
                    return min_tasks;
                }
                if (len >= min_tasks)
                    return min_tasks;

                auto const n = static_cast<std::uint64_t>(len);
                std::uint64_t const needed = (steal_looks + n - 1) / n;
                std::uint64_t seen =
                    c.fetch_add(1, std::memory_order_relaxed) + 1;
                if (seen > needed &&
                    c.compare_exchange_strong(
                        seen, 0, std::memory_order_relaxed))
                {
                    return 0;
                }
                return min_tasks;
            }

            std::int64_t const len =
                q->get_pending_queue_length(std::memory_order_relaxed);
            std::uint32_t& looks = victims_[num_thread].data_.looks_[idx];
            if (len == 0)
            {
                looks = 0;
                return min_tasks;
            }
            if (len >= min_tasks)
                return min_tasks;

            if (looks <= steal_looks)
                ++looks;
            return looks > steal_looks ? 0 : min_tasks;
        }

    private:
        bool get_next_thread_impl(std::size_t num_thread, bool const running,
            threads::thread_id_ref_type& thrd, bool enable_stealing)
        {
            // tri-state result for trying a queue:
            // - nothing: no work found, no staged items
            // - found: a thread was popped and 'thrd' is valid -> return true
            // - staged_non_empty: no thread popped but there is staged work ->
            //   give up and return false
            enum class try_result : std::int8_t
            {
                staged_non_empty = 0,
                found = 1,
                nothing = 2
            };

            // Helper: try a single queue. Returns
            // - try_result::staged_non_empty if staged items exist (signal to
            //   give up)
            // - try_result::found if a thread was obtained
            // - try_result::nothing otherwise
            constexpr auto try_queue =
                [](thread_queue_type* q,
                    thread_id_ref_type& next_thrd) noexcept -> try_result {
                bool const found = q->get_next_thread(next_thrd);

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                q->increment_num_pending_accesses();
                if (found)
                    return try_result::found;
                q->increment_num_pending_misses();
#else
                if (found)
                    return try_result::found;
#endif

                // If there is staged work waiting to be converted, signal
                // caller to give up so conversion can proceed instead of
                // continuing to try other queues.
                if (q->get_staged_queue_length(std::memory_order_relaxed) != 0)
                    return try_result::staged_non_empty;

                return try_result::nothing;
            };

            thread_queue_type* this_high_priority_queue = nullptr;
            if (num_thread < num_high_priority_queues_)
            {
                this_high_priority_queue =
                    queues_[num_thread].high_priority_queue_;

                if (auto const r = try_queue(this_high_priority_queue, thrd);
                    r == try_result::found)
                {
                    // We successfully popped a thread from the queue
                    return true;
                }
                else if (r == try_result::staged_non_empty)
                {
                    // Staged work exists; give up so it can be converted
                    return false;
                }
            }

            // Try the bound queue (per-thread bound-to-thread queue).
            if (auto const r =
                    try_queue(queues_[num_thread].bound_queue_, thrd);
                r == try_result::found)
            {
                return true;
            }
            else if (r == try_result::staged_non_empty)
            {
                return false;
            }

            // Try the "this" queue (main per-thread queue).
            thread_queue_type* this_queue = queues_[num_thread].queue_;
            if (auto const r = try_queue(this_queue, thrd);
                r == try_result::found)
            {
                return true;
            }
            else if (r == try_result::staged_non_empty)
            {
                return false;
            }

            // If the thread system is shutting down (not running), give up.
            if (!running)
                return false;

            // Attempt stealing from other queues if enabled. If stealing
            // succeeds, attempt_stealing_pending returns true, and we return
            // true (thread obtained).
            if (enable_stealing)
            {
                // Only steal pending tasks from workers that have a few of
                // them. Otherwise all idle workers pop the tasks a worker has
                // just converted one by one, and go idle again together. The
                // tasks of a worker that is busy running a long task still
                // have to be stolen: the minimum is lifted per victim (see
                // victim_min_tasks), and for a worker that was just woken up
                // (free pass).
                std::int64_t min_tasks = 0;
                if (!has_free_pass(num_thread) &&
                    !has_scheduler_mode(
                        policies::scheduler_mode::fast_idle_mode, num_thread))
                {
                    min_tasks =
                        thread_queue_init_.min_tasks_to_steal_pending_early_;
                }

                if (attempt_stealing_pending(num_thread, thrd,
                        this_high_priority_queue, this_queue, min_tasks))
                {
                    return true;
                }
            }

            // Final fallback: try the global low-priority queue
            return low_priority_queue_.get_next_thread(thrd);
        }

    public:
        // Schedule the passed thread
        void schedule_thread(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint const schedulehint,
            bool allow_fallback = false,
            thread_priority priority = thread_priority::default_) override
        {
            // NOTE: This scheduler ignores NUMA hints.
            auto num_thread = static_cast<std::size_t>(-1);
            if (schedulehint.mode() == thread_schedule_hint_mode::thread)
            {
                num_thread = schedulehint.hint;
            }
            else
            {
                allow_fallback = false;
            }

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_.data_++ % num_queues_;
            }
            else if (num_thread >= num_queues_)
            {
                num_thread %= num_queues_;
            }

            num_thread = select_active_pu(num_thread, allow_fallback);

            [[maybe_unused]] auto const* thrdptr = get_thread_id_data(thrd);
            switch (priority)
            {
            case thread_priority::high_recursive:
            case thread_priority::high:
                [[fallthrough]];
            case thread_priority::boost:
            {
                std::size_t num = num_thread % num_high_priority_queues_;
                LTM_(debug).format(
                    "local_priority_queue_scheduler::schedule_thread, "
                    "high priority queue: pool({}), scheduler({}), "
                    "worker_thread({}), thread({}), priority({}), "
                    "description({})",
                    *this->get_parent_pool(), *this, num,
                    thrdptr->get_thread_id(), priority,
                    thrdptr->get_description());

                queues_[num].high_priority_queue_->schedule_thread(
                    HPX_MOVE(thrd));
            }
            break;

            case thread_priority::low:
            {
                LTM_(debug).format(
                    "local_priority_queue_scheduler::schedule_thread, "
                    "low priority queue: pool({}), scheduler({}), "
                    "thread({}), priority({}), description({})",
                    *this->get_parent_pool(), *this, thrdptr->get_thread_id(),
                    priority, thrdptr->get_description());

                low_priority_queue_.schedule_thread(HPX_MOVE(thrd));
            }
            break;

            case thread_priority::default_:
                [[fallthrough]];
            case thread_priority::normal:
            {
                HPX_ASSERT(num_thread < num_queues_);

                LTM_(debug).format(
                    "local_priority_queue_scheduler::schedule_thread, "
                    "normal priority queue: pool({}), scheduler({}), "
                    "worker_thread({}), thread({}), priority({}), "
                    "description({})",
                    *this->get_parent_pool(), *this, num_thread,
                    thrdptr->get_thread_id(), priority,
                    thrdptr->get_description());

                queues_[num_thread].queue_->schedule_thread(HPX_MOVE(thrd));
            }
            break;

            case thread_priority::initially_bound:
                [[fallthrough]];
            case thread_priority::bound:
            {
                HPX_ASSERT(num_thread < num_queues_);

                LTM_(debug).format(
                    "local_priority_queue_scheduler::schedule_thread, "
                    "bound priority queue: pool({}), scheduler({}), "
                    "worker_thread({}), thread({}), priority({}), "
                    "description({})",
                    *this->get_parent_pool(), *this, num_thread,
                    thrdptr->get_thread_id(), priority,
                    thrdptr->get_description());

                queues_[num_thread].bound_queue_->schedule_thread(
                    HPX_MOVE(thrd));
            }
            break;

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_priority_queue_scheduler::schedule_thread",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
        }

        void schedule_thread_last(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint const schedulehint,
            bool allow_fallback = false,
            thread_priority priority = thread_priority::default_) override
        {
            // NOTE: This scheduler ignores NUMA hints.
            std::size_t num_thread = static_cast<std::size_t>(-1);

            // If the user specified a concrete thread to use, and the initial
            // priority is initially_bound, then schedule the thread as
            // initially bound to make sure the thread starts running on the
            // specified core.
            if (schedulehint.mode() == thread_schedule_hint_mode::thread)
            {
                HPX_ASSERT(schedulehint.hint >= 0);
                num_thread = schedulehint.hint;

                if (priority == thread_priority::initially_bound)
                {
                    priority = thread_priority::bound;
                }
            }
            else
            {
                allow_fallback = false;
            }

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_.data_++ % num_queues_;
            }
            else if (num_thread >= num_queues_)
            {
                num_thread %= num_queues_;
            }

            num_thread = select_active_pu(num_thread, allow_fallback);

            switch (priority)
            {
            case thread_priority::high_recursive:
            case thread_priority::high:
                [[fallthrough]];
            case thread_priority::boost:
            {
                std::size_t num = num_thread % num_high_priority_queues_;
                queues_[num].high_priority_queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
            }
            break;
            case thread_priority::low:
            {
                low_priority_queue_.schedule_thread(HPX_MOVE(thrd), true);
            }
            break;

            case thread_priority::default_:
                [[fallthrough]];
            case thread_priority::normal:
            {
                HPX_ASSERT(num_thread < num_queues_);
                queues_[num_thread].queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
            }
            break;

            case thread_priority::initially_bound:
                [[fallthrough]];
            case thread_priority::bound:
            {
                HPX_ASSERT(num_thread < num_queues_);
                queues_[num_thread].bound_queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
            }
            break;

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_priority_queue_scheduler::schedule_thread_last",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
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
                HPX_ASSERT(num_thread < num_queues_);

                if (num_thread < num_high_priority_queues_)
                {
                    count = queues_[num_thread]
                                .high_priority_queue_->get_queue_length();
                }
                if (num_thread == num_queues_ - 1)
                    count += low_priority_queue_.get_queue_length();

                count += queues_[num_thread].bound_queue_->get_queue_length();
                count += queues_[num_thread].queue_->get_queue_length();
                return count;
            }

            // Cumulative queue lengths of all queues.
            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                count += queues_[i].high_priority_queue_->get_queue_length();
            }
            count += low_priority_queue_.get_queue_length();

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                count += queues_[i].bound_queue_->get_queue_length();
                count += queues_[i].queue_->get_queue_length();
            }

            return count;
        }

        ///////////////////////////////////////////////////////////////////////
        // Queries the current thread count of the queues.
        std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown,
            thread_priority const priority = thread_priority::default_,
            std::size_t num_thread = static_cast<std::size_t>(-1),
            bool /* reset */ = false) const override
        {
            // Return thread count of one specific queue.
            std::int64_t count = 0;
            if (static_cast<std::size_t>(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < num_queues_);

                switch (priority)
                {
                case thread_priority::default_:
                {
                    if (num_thread < num_high_priority_queues_)
                    {
                        count =
                            queues_[num_thread]
                                .high_priority_queue_->get_thread_count(state);
                    }
                    if (num_queues_ - 1 == num_thread)
                        count += low_priority_queue_.get_thread_count(state);

                    count += queues_[num_thread].bound_queue_->get_thread_count(
                        state);
                    count +=
                        queues_[num_thread].queue_->get_thread_count(state);
                    return count;
                }

                case thread_priority::low:
                {
                    if (num_queues_ - 1 == num_thread)
                        return low_priority_queue_.get_thread_count(state);
                    break;
                }

                case thread_priority::initially_bound:
                    [[fallthrough]];
                case thread_priority::bound:
                    return queues_[num_thread].bound_queue_->get_thread_count(
                        state);

                case thread_priority::normal:
                    return queues_[num_thread].queue_->get_thread_count(state);

                case thread_priority::boost:
                case thread_priority::high:
                    [[fallthrough]];
                case thread_priority::high_recursive:
                {
                    if (num_thread < num_high_priority_queues_)
                    {
                        return queues_[num_thread]
                            .high_priority_queue_->get_thread_count(state);
                    }
                    break;
                }

                case thread_priority::unknown:
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "local_priority_queue_scheduler::get_thread_count",
                        "unknown thread priority value "
                        "(thread_priority::unknown)");
                }
                }
                return 0;
            }

            // Return the cumulative count for all queues.
            switch (priority)
            {
            case thread_priority::default_:
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    count += queues_[i].high_priority_queue_->get_thread_count(
                        state);
                }
                count += low_priority_queue_.get_thread_count(state);

                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    count += queues_[i].bound_queue_->get_thread_count(state);
                    count += queues_[i].queue_->get_thread_count(state);
                }
                break;
            }

            case thread_priority::low:
                return low_priority_queue_.get_thread_count(state);

            case thread_priority::initially_bound:
                [[fallthrough]];
            case thread_priority::bound:
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    count += queues_[i].bound_queue_->get_thread_count(state);
                }
                break;
            }

            case thread_priority::normal:
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    count += queues_[i].queue_->get_thread_count(state);
                }
                break;
            }

            case thread_priority::boost:
            case thread_priority::high:
                [[fallthrough]];
            case thread_priority::high_recursive:
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    count += queues_[i].high_priority_queue_->get_thread_count(
                        state);
                }
                break;
            }

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_priority_queue_scheduler::get_thread_count",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
            return count;
        }

        // Queries whether a given core is idle
        bool is_core_idle(std::size_t num_thread) const override
        {
            if (num_thread < num_queues_ &&
                queues_[num_thread].bound_queue_->get_queue_length() != 0 &&
                queues_[num_thread].queue_->get_queue_length() != 0)
            {
                return false;
            }
            if (num_thread < num_high_priority_queues_ &&
                queues_[num_thread].high_priority_queue_->get_queue_length() !=
                    0)
            {
                return false;
            }
            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        // Enumerate matching threads from all queues
        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state =
                thread_schedule_state::unknown) const override
        {
            bool result = true;
            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                result = result &&
                    queues_[i].high_priority_queue_->enumerate_threads(
                        f, state);
            }

            result = result && low_priority_queue_.enumerate_threads(f, state);

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                result = result &&
                    queues_[i].bound_queue_->enumerate_threads(f, state);
                result =
                    result && queues_[i].queue_->enumerate_threads(f, state);
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
                HPX_ASSERT(num_thread < num_queues_);

                if (num_thread < num_high_priority_queues_)
                {
                    wait_time = queues_[num_thread]
                                    .high_priority_queue_
                                    ->get_average_thread_wait_time();
                    ++count;
                }

                if (num_queues_ - 1 == num_thread)
                {
                    wait_time +=
                        low_priority_queue_.get_average_thread_wait_time();
                    ++count;
                }

                wait_time += queues_[num_thread]
                                 .bound_queue_->get_average_thread_wait_time();
                wait_time +=
                    queues_[num_thread].queue_->get_average_thread_wait_time();
                return wait_time / (count + 1);
            }

            // Return the cumulative average thread wait time for all queues.
            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                wait_time +=
                    queues_[i]
                        .high_priority_queue_->get_average_thread_wait_time();
                ++count;
            }

            wait_time += low_priority_queue_.get_average_thread_wait_time();

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                wait_time +=
                    queues_[i].bound_queue_->get_average_thread_wait_time();
                wait_time += queues_[i].queue_->get_average_thread_wait_time();
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
                HPX_ASSERT(num_thread < num_queues_);

                if (num_thread < num_high_priority_queues_)
                {
                    wait_time =
                        queues_[num_thread]
                            .high_priority_queue_->get_average_task_wait_time();
                    ++count;
                }

                if (num_queues_ - 1 == num_thread)
                {
                    wait_time +=
                        low_priority_queue_.get_average_task_wait_time();
                    ++count;
                }

                wait_time += queues_[num_thread]
                                 .bound_queue_->get_average_task_wait_time();
                wait_time +=
                    queues_[num_thread].queue_->get_average_task_wait_time();
                return wait_time / (count + 1);
            }

            // Return the cumulative average task wait time for all queues.
            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                wait_time +=
                    queues_[i]
                        .high_priority_queue_->get_average_task_wait_time();
                ++count;
            }

            wait_time += low_priority_queue_.get_average_task_wait_time();

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                wait_time +=
                    queues_[i].bound_queue_->get_average_task_wait_time();
                wait_time += queues_[i].queue_->get_average_task_wait_time();
                ++count;
            }

            return wait_time / (count + 1);
        }
#endif

        bool attempt_stealing(std::size_t num_thread, std::size_t& added,
            thread_queue_type* this_high_priority_queue,
            thread_queue_type* this_queue, std::int64_t const min_tasks)
        {
            constexpr auto increment_counters =
                []([[maybe_unused]] thread_queue_type* from_q,
                    [[maybe_unused]] thread_queue_type* to_q) noexcept {
#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                    from_q->increment_num_stolen_from_staged();
                    to_q->increment_num_stolen_to_staged();
#endif
                };

            std::vector<std::size_t> const& victim_threads =
                victims_[num_thread].data_.victim_threads_;
            std::size_t const victim_count = victim_threads.size();
            if (victim_count == 0)
                return false;

            bool result = true;

            auto const steal = [&](std::size_t idx) -> bool {
                HPX_ASSERT(idx != num_thread);

                // suppress contracts warning about 'num_thread' not implicitly
                // captured by a contract assertion
                (void) num_thread;

                thread_queue_type* q = queues_[idx].queue_;
                std::int64_t const victim_min =
                    victim_min_tasks(num_thread, idx, q, min_tasks, true);

                // a steal that ignores the minimum takes a single task, the
                // others stay with the victim
                result = this_queue->wait_or_add_new(true, added, q, false,
                             victim_min, victim_min < min_tasks ? 1 : -1) &&
                    result;

                if (0 != added)
                {
                    increment_counters(q, this_queue);
                    return true;
                }
                return false;
            };

            std::size_t const num_high = num_high_priority_queues_;
            auto const steal_hp = [&](std::size_t idx) -> bool {
                HPX_ASSERT(idx != num_thread);

                // suppress contracts warning about 'num_thread' not implicitly
                // captured by a contract assertion
                (void) num_thread;

                // If the stealing thread has a high-priority queue, try
                // high-priority victims first (and then their normal queues).
                // Otherwise, only try normal queues.
                if (idx < num_high)
                {
                    thread_queue_type* q = queues_[idx].high_priority_queue_;
                    result = this_high_priority_queue->wait_or_add_new(
                                 true, added, q, false, min_tasks) &&
                        result;

                    if (0 != added)
                    {
                        increment_counters(q, this_high_priority_queue);
                        return true;
                    }
                }

                return steal(idx);
            };

            std::size_t const offset =
                victims_[num_thread].data_.victim_offset_++;
            if (num_thread < num_high)
            {
                if (for_each_circular(offset % victim_count, victim_count,
                        [&](std::size_t const i) {
                            return steal_hp(victim_threads[i]);
                        }))
                    return result;
            }
            else
            {
                if (for_each_circular(offset % victim_count, victim_count,
                        [&](std::size_t const i) {
                            return steal(victim_threads[i]);
                        }))
                    return result;
            }

            return false;
        }

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
        void deadlock_detection(
            std::size_t num_thread, bool running, std::int64_t& idle_loop_count)
        {
            // no new work is available, are we deadlocked?
            if (HPX_UNLIKELY(get_minimal_deadlock_detection_enabled() &&
                    LHPX_ENABLED(error)))
            {
                bool suspended_only = true;

                for (std::size_t i = 0; suspended_only && i != num_queues_; ++i)
                {
                    suspended_only =
                        queues_[i].bound_queue_->dump_suspended_threads(
                            i, idle_loop_count, running);
                    suspended_only = queues_[i].queue_->dump_suspended_threads(
                                         i, idle_loop_count, running) ||
                        suspended_only;
                }

                if (HPX_UNLIKELY(suspended_only))
                {
                    if (running)
                    {
                        LTM_(error).format("pool({}), scheduler({}), "
                                           "worker_thread({}): no new work "
                                           "available, are we deadlocked?",
                            *this->get_parent_pool(), *this, num_thread);
                    }
                    else
                    {
                        LHPX_CONSOLE_(hpx::util::logging::level::error)
                            .format("  [TM] pool({}), scheduler({}), "
                                    "worker_thread({}): no new work available, "
                                    "are we deadlocked?\n",
                                *this->get_parent_pool(), *this, num_thread);
                    }
                }
            }
        }
#endif

        // This is a function which gets called periodically by the thread
        // manager to allow for maintenance tasks to be executed in the
        // scheduler. Returns true if the OS thread calling this function has to
        // be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(std::size_t num_thread, bool running,
            [[maybe_unused]] std::int64_t& idle_loop_count,
            bool const enable_stealing, std::size_t& added,
            thread_id_ref_type* = nullptr)
        {
            added = 0;

            bool result = true;
            thread_queue_type* this_high_priority_queue = nullptr;
            if (num_thread < num_high_priority_queues_)
            {
                this_high_priority_queue =
                    queues_[num_thread].high_priority_queue_;
                result =
                    this_high_priority_queue->wait_or_add_new(running, added) &&
                    result;
                if (0 != added)
                    return result;
            }

            thread_queue_type* this_queue = queues_[num_thread].queue_;
            auto f = [&](thread_queue_type* q) {
                result = q->wait_or_add_new(running, added) && result;
                return 0 != added;
            };

            if (f(queues_[num_thread].bound_queue_))
                return result;

            if (f(this_queue))
                return result;

            // Check if we have been disabled
            if (!running)
            {
                return true;
            }

            if (enable_stealing)
            {
                // Staged stealing starts early (see the constructor). Only
                // steal from workers that have more staged tasks than they will
                // convert soon themselves, leaving the others where they were
                // placed. The minimum is lifted per victim (see
                // victim_min_tasks), and for a worker that was just woken up
                // (free pass).
                std::int64_t min_tasks = 0;
                if (!has_free_pass(num_thread) &&
                    !has_scheduler_mode(
                        policies::scheduler_mode::fast_idle_mode, num_thread))
                {
                    min_tasks =
                        thread_queue_init_.min_tasks_to_steal_staged_early_;
                }

                result = attempt_stealing(num_thread, added,
                             this_high_priority_queue, this_queue, min_tasks) &&
                    result;
                if (0 != added)
                {
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
                    this->clear_free_pass(num_thread);
#endif
                    return result;
                }
            }

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
            deadlock_detection(num_thread, running, idle_loop_count);
#endif

            if (num_thread == num_queues_ - 1)
            {
                result = low_priority_queue_.wait_or_add_new(running, added) &&
                    result;
            }

            return result;
        }

        // True while a bound thread created on this worker has not
        // terminated. It runs only here, so the worker must not leave its
        // scheduling loop: a thread being woken is marked pending before it
        // is queued, and in between neither the suspended threads nor the
        // queue length show it (see scheduling_loop).
        bool has_live_bound_threads(std::size_t num_thread) const
        {
            HPX_ASSERT(num_thread < num_queues_);
            return queues_[num_thread].bound_queue_->get_thread_count() != 0;
        }

        // Called by the scheduling loop of the last worker to leave during
        // shutdown while it takes over the work left (see scheduling_loop).
        // Stealing can't reach all of it: the victims of a worker may not
        // include the workers of other NUMA domains, nobody but its owner
        // runs a worker's bound threads, and only the last worker converts the
        // low priority threads. Move the work left on the queues of workers
        // that have left their scheduling loop to the queues of this worker.
        // This returns true if any work was moved.
        bool take_over_work(std::size_t num_thread)
        {
            HPX_ASSERT(num_thread < num_queues_);

            std::size_t added = 0;

            // pending threads, without applying the stealing minimum
            auto const move_pending = [&added](thread_queue_type* from,
                                          thread_queue_type* to) {
                while (true)
                {
                    thread_id_ref_type thrd;
                    if (!from->get_next_thread(thrd, false, true))
                        return;
                    to->schedule_thread(HPX_MOVE(thrd), true);
                    ++added;
                }
            };

            thread_queue_type* this_queue = queues_[num_thread].queue_;
            thread_queue_type* this_high_priority_queue =
                num_thread < num_high_priority_queues_ ?
                queues_[num_thread].high_priority_queue_ :
                this_queue;

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                if (i == num_thread ||
                    this->get_state(i).load(std::memory_order_relaxed) !=
                        hpx::state::stopped)
                {
                    continue;
                }

                // staged threads (running == false skips the minimum)
                if (i < num_high_priority_queues_)
                {
                    thread_queue_type* q = queues_[i].high_priority_queue_;
                    this_high_priority_queue->wait_or_add_new(
                        false, added, q, true);
                    move_pending(q, this_high_priority_queue);
                }

                // bound threads stay bound, to this worker now
                thread_queue_type* this_bound_queue =
                    queues_[num_thread].bound_queue_;
                thread_queue_type* q = queues_[i].bound_queue_;
                this_bound_queue->wait_or_add_new(false, added, q, true);
                move_pending(q, this_bound_queue);

                q = queues_[i].queue_;
                this_queue->wait_or_add_new(false, added, q, true);
                move_pending(q, this_queue);

                if (i == num_queues_ - 1)
                {
                    low_priority_queue_.wait_or_add_new(true, added);
                }
            }
            return added != 0;
        }

        ///////////////////////////////////////////////////////////////////////
        void on_start_thread(std::size_t num_thread) override
        {
            hpx::threads::detail::set_local_thread_num_tss(num_thread);
            hpx::threads::detail::set_thread_pool_num_tss(
                parent_pool_->get_pool_id().index());

            if (nullptr == queues_[num_thread].queue_)
            {
                HPX_ASSERT(queues_[num_thread].bound_queue_ == nullptr);
                queues_[num_thread].bound_queue_ =
                    new thread_queue_type(thread_queue_init_);

                queues_[num_thread].queue_ =
                    new thread_queue_type(thread_queue_init_);

                if (num_thread < num_high_priority_queues_)
                {
                    queues_[num_thread].high_priority_queue_ =
                        new thread_queue_type(thread_queue_init_);
                }
            }
            else
            {
                HPX_ASSERT(queues_[num_thread].bound_queue_ != nullptr);
            }

            // forward this call to all queues etc.
            if (num_thread < num_high_priority_queues_)
            {
                queues_[num_thread].high_priority_queue_->on_start_thread(
                    num_thread);
            }

            if (num_thread == num_queues_ - 1)
                low_priority_queue_.on_start_thread(num_thread);

            queues_[num_thread].bound_queue_->on_start_thread(num_thread);
            queues_[num_thread].queue_->on_start_thread(num_thread);

            std::size_t const num_threads = num_queues_;
            auto const& topo = create_topology();

            // get NUMA domain masks of all queues...
            std::vector<mask_type> numa_masks(num_threads);
            std::vector<mask_type> core_masks(num_threads);
            std::vector<std::ptrdiff_t> numa_domains(num_threads);
            for (std::size_t i = 0; i != num_threads; ++i)
            {
                std::size_t const num_pu = affinity_data_.get_pu_num(i);
                numa_masks[i] = topo.get_numa_node_affinity_mask(num_pu);
                numa_domains[i] = static_cast<std::ptrdiff_t>(
                    topo.get_numa_node_number(num_pu));
                core_masks[i] = topo.get_core_affinity_mask(num_pu);
            }

            // iterate over the number of threads again to determine where to
            // steal from
            std::ptrdiff_t const radius =
                std::lround(static_cast<double>(num_threads) / 2.0);
            std::vector<std::size_t>& victim_threads =
                victims_[num_thread].data_.victim_threads_;
            victim_threads.reserve(num_threads);
            victims_[num_thread].data_.looks_.assign(num_threads, 0);

            // Record this thread's NUMA domain for use in create_thread().
            thread_numa_domain_[num_thread] =
                static_cast<std::size_t>(numa_domains[num_thread]);

            std::size_t const num_pu = affinity_data_.get_pu_num(num_thread);
            mask_cref_type pu_mask = topo.get_thread_affinity_mask(num_pu);
            mask_cref_type numa_mask = numa_masks[num_thread];
            mask_cref_type core_mask = core_masks[num_thread];

            // we allow the thread on the boundary of the NUMA domain to steal
            auto first_mask = mask_type();
            resize(first_mask, mask_size(pu_mask));

            std::size_t const first = find_first(numa_mask);
            if (first != static_cast<std::size_t>(-1))
                set(first_mask, first);
            else
                first_mask = pu_mask;

            auto iterate = [&](auto&& f) {
                // check our neighbors in a radial fashion (left and right
                // alternating, increasing distance each iteration)
                std::ptrdiff_t i = 1;
                for (/**/; i < radius; ++i)
                {
                    std::ptrdiff_t left =
                        (static_cast<std::ptrdiff_t>(num_thread) - i) %
                        static_cast<std::ptrdiff_t>(num_threads);
                    if (left < 0)
                        left = num_threads + left;

                    if (f(static_cast<std::size_t>(left)))
                    {
                        victim_threads.push_back(
                            static_cast<std::size_t>(left));
                    }

                    std::size_t const right = (num_thread + i) % num_threads;
                    if (f(right))
                    {
                        victim_threads.push_back(right);
                    }
                }
                if ((num_threads % 2) == 0)
                {
                    std::size_t const right = (num_thread + i) % num_threads;
                    if (f(right))
                    {
                        victim_threads.push_back(right);
                    }
                }
            };

            // check for threads which share the same core...
            iterate([&](std::size_t const other_num_thread) {
                return any(core_mask & core_masks[other_num_thread]);
            });

            // check for threads which share the same NUMA domain...
            iterate([&](std::size_t const other_num_thread) {
                return !any(core_mask & core_masks[other_num_thread]) &&
                    any(numa_mask & numa_masks[other_num_thread]);
            });

            // Check other NUMA domains in increasing distance order. Depending
            // on the configured sensitivity this includes all workers, only
            // boundary workers and their nearest domains, or no remote workers.
            if (has_scheduler_mode(
                    policies::scheduler_mode::enable_stealing_numa, num_thread))
            {
                std::size_t const my_domain =
                    static_cast<std::size_t>(numa_domains[num_thread]);
                std::size_t num_nodes = topo.get_number_of_numa_nodes();
                if (num_nodes == 0)
                    num_nodes = 1;

                auto const remote_domains =
                    detail::get_numa_stealing_domains(numa_sensitive_,
                        my_domain, num_nodes, any(first_mask & pu_mask),
                        [&topo](std::size_t from, std::size_t to) {
                            return topo.get_numa_distance(from, to);
                        });
                for (std::size_t domain : remote_domains)
                {
                    iterate([&](std::size_t const other_num_thread) {
                        return static_cast<std::size_t>(
                                   numa_domains[other_num_thread]) == domain;
                    });
                }
            }
        }

        void on_stop_thread(std::size_t num_thread) override
        {
            if (num_thread < num_high_priority_queues_)
            {
                queues_[num_thread].high_priority_queue_->on_stop_thread(
                    num_thread);
            }
            if (num_thread == num_queues_ - 1)
                low_priority_queue_.on_stop_thread(num_thread);

            if (num_thread < queues_.size())
            {
                queues_[num_thread].bound_queue_->on_stop_thread(num_thread);
                queues_[num_thread].queue_->on_stop_thread(num_thread);
            }
        }

        void on_error(
            std::size_t num_thread, std::exception_ptr const& e) override
        {
            if (num_thread < num_high_priority_queues_)
            {
                queues_[num_thread].high_priority_queue_->on_error(
                    num_thread, e);
            }
            if (num_thread == num_queues_ - 1)
                low_priority_queue_.on_error(num_thread, e);

            if (num_thread < queues_.size())
            {
                queues_[num_thread].bound_queue_->on_error(num_thread, e);
                queues_[num_thread].queue_->on_error(num_thread, e);
            }
        }

        void reset_thread_distribution() noexcept override
        {
            curr_queue_.data_.store(0, std::memory_order_release);
        }

    protected:
        // The members are grouped by who writes them. Each group that is
        // written while the workers run starts at a multiple of the
        // destructive interference size, so that a write to it does not
        // invalidate data that other workers read (on x86-64 the spatial
        // prefetcher fetches 128-byte pairs of cache lines together).
        //
        // Compared with padding every queue pointer to the destructive
        // interference size, this grouping raises spawnself by 6 to 15% at
        // 20 and 40 workers and spawn by 15% at 10 workers, and leaves the
        // other spawn and steal benchmarks within 4% (2 x 20 core Xeon,
        // clang 18, median over 16 heap layouts).

        // Round-robin counter for threads created without a placement hint.
        // It is written by every spawning thread; keep it apart from the
        // read-mostly members below.
        util::aligned_line_data<std::atomic<std::size_t>> curr_queue_;

        // Not written once the workers have started, read by all workers.
        detail::affinity_data const& affinity_data_;
        numa_sensitive_mode const numa_sensitive_;

        std::size_t const num_queues_;
        std::size_t const num_high_priority_queues_;

        // The queues of each worker. The pointers are not written once the
        // worker has started, so they are packed densely: a thief finds all
        // queues of a victim in one cache line.
        struct worker_queues
        {
            thread_queue_type* bound_queue_ = nullptr;
            thread_queue_type* queue_ = nullptr;
            thread_queue_type* high_priority_queue_ = nullptr;
        };
        std::vector<worker_queues> queues_;

        // NUMA domain index for each worker thread, indexed by thread number.
        // Populated during on_start_thread(); used for NUMA-hint routing in
        // create_thread().
        std::vector<std::size_t> thread_numa_domain_;

        // Written by the owning worker on every attempt to steal.
        struct victims
        {
            std::vector<std::size_t> victim_threads_;
            std::size_t victim_offset_ = 0;

            // consecutive looks at a short pending queue of each victim, see
            // victim_min_tasks
            std::vector<std::uint32_t> looks_;
        };
        std::vector<util::aligned_line_data<victims>> victims_;

        // Aligned to the destructive interference size (see thread_queue).
        thread_queue_type low_priority_queue_;
    };    // namespace hpx::threads::policies
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
