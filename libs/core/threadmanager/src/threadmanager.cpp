//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach, Katelyn Kufahl
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//  Copyright (c) 2015 Patricia Grubel
//  Copyright (c) 2017 Shoshana Jakobovits
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/thread_pool_util.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace hpx::threads {

    namespace detail {
        static void check_num_high_priority_queues(
            std::size_t const num_threads,
            std::size_t const num_high_priority_queues)
        {
            if (num_high_priority_queues > num_threads)
            {
                throw hpx::detail::command_line_error(
                    "Invalid command line option: number of high priority "
                    "threads (--hpx:high-priority-threads), should not be "
                    "larger than number of threads (--hpx:threads)");
            }
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    threadmanager::threadmanager(hpx::util::runtime_configuration& rtcfg,
#ifdef HPX_HAVE_TIMER_POOL
        util::io_service_pool& timer_pool,
#endif
        notification_policy_type& notifier,
        detail::network_background_callback_type network_background_callback)
      : rtcfg_(rtcfg)
#ifdef HPX_HAVE_TIMER_POOL
      , timer_pool_(timer_pool)
#endif
      , notifier_(notifier)
      , network_background_callback_(HPX_MOVE(network_background_callback))
    {
        using placeholders::_1;
        using placeholders::_3;

        // Add callbacks local to threadmanager.
        notifier.add_on_start_thread_callback(
            hpx::bind(&threadmanager::init_tss, _1));
        notifier.add_on_stop_thread_callback(
            hpx::bind(&threadmanager::deinit_tss));

        auto& rp = hpx::resource::get_partitioner();
        notifier.add_on_start_thread_callback(hpx::bind(
            &resource::detail::partitioner::assign_pu, std::ref(rp), _3, _1));
        notifier.add_on_stop_thread_callback(hpx::bind(
            &resource::detail::partitioner::unassign_pu, std::ref(rp), _3, _1));
    }

    policies::thread_queue_init_parameters threadmanager::get_init_parameters()
        const
    {
        std::int64_t const max_thread_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.max_thread_count",
                HPX_THREAD_QUEUE_MAX_THREAD_COUNT);
        std::int64_t const min_tasks_to_steal_pending =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.min_tasks_to_steal_pending",
                HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_PENDING);
        std::int64_t const min_tasks_to_steal_staged =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.min_tasks_to_steal_staged",
                HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_STAGED);
        std::int64_t const min_add_new_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.min_add_new_count",
                HPX_THREAD_QUEUE_MIN_ADD_NEW_COUNT);
        std::int64_t const max_add_new_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.max_add_new_count",
                HPX_THREAD_QUEUE_MAX_ADD_NEW_COUNT);
        std::int64_t const min_delete_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.min_delete_count",
                HPX_THREAD_QUEUE_MIN_DELETE_COUNT);
        std::int64_t const max_delete_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.max_delete_count",
                HPX_THREAD_QUEUE_MAX_DELETE_COUNT);
        std::int64_t const max_terminated_threads =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.max_terminated_threads",
                HPX_THREAD_QUEUE_MAX_TERMINATED_THREADS);
        std::uint64_t const init_threads_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.init_threads_count",
                HPX_THREAD_QUEUE_INIT_THREADS_COUNT);
        std::uint64_t const cached_threads_count =
            hpx::util::get_entry_as<std::int64_t>(rtcfg_,
                "hpx.thread_queue.cached_threads_count",
                HPX_THREAD_QUEUE_CACHED_THREADS_COUNT);
        double const max_idle_backoff_time = hpx::util::get_entry_as<double>(
            rtcfg_, "hpx.max_idle_backoff_time", HPX_IDLE_BACKOFF_TIME_MAX);

        std::ptrdiff_t const small_stacksize =
            rtcfg_.get_stack_size(thread_stacksize::small_);
        std::ptrdiff_t const medium_stacksize =
            rtcfg_.get_stack_size(thread_stacksize::medium);
        std::ptrdiff_t const large_stacksize =
            rtcfg_.get_stack_size(thread_stacksize::large);
        std::ptrdiff_t const huge_stacksize =
            rtcfg_.get_stack_size(thread_stacksize::huge);

        return policies::thread_queue_init_parameters(max_thread_count,
            min_tasks_to_steal_pending, min_tasks_to_steal_staged,
            min_add_new_count, max_add_new_count, min_delete_count,
            max_delete_count, max_terminated_threads, init_threads_count,
            cached_threads_count, max_idle_backoff_time, small_stacksize,
            medium_stacksize, large_stacksize, huge_stacksize);
    }

    void threadmanager::create_scheduler_user_defined(
        hpx::resource::scheduler_function const& pool_func,
        thread_pool_init_parameters const& thread_pool_init,
        policies::thread_queue_init_parameters const& thread_queue_init)
    {
        std::unique_ptr<thread_pool_base> pool(
            pool_func(thread_pool_init, thread_queue_init));
        pools_.push_back(HPX_MOVE(pool));
    }

    void threadmanager::create_scheduler_local(
        thread_pool_init_parameters const& thread_pool_init,
        policies::thread_queue_init_parameters const& thread_queue_init,
        std::size_t const numa_sensitive)
    {
        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_queue_scheduler<>;

        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            thread_queue_init, "core-local_queue_scheduler");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
    }

    void threadmanager::create_scheduler_local_priority_fifo(
        thread_pool_init_parameters const& thread_pool_init,
        policies::thread_queue_init_parameters const& thread_queue_init,
        std::size_t const numa_sensitive)
    {
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);

        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
                hpx::threads::policies::lockfree_fifo>;

        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-local_priority_queue_scheduler-fifo");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
    }

    void threadmanager::create_scheduler_local_priority_lifo(
        [[maybe_unused]] thread_pool_init_parameters const& thread_pool_init,
        [[maybe_unused]] policies::thread_queue_init_parameters const&
            thread_queue_init,
        [[maybe_unused]] std::size_t numa_sensitive)
    {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
                hpx::threads::policies::lockfree_lifo>;

        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-local_priority_queue_scheduler-lifo");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=local-priority-lifo "
            "is not configured in this build. Please make sure 128bit "
            "atomics are available.");
#endif
    }

    void threadmanager::create_scheduler_static(
        thread_pool_init_parameters const& thread_pool_init,
        policies::thread_queue_init_parameters const& thread_queue_init,
        std::size_t const numa_sensitive)
    {
        // instantiate the scheduler
        std::unique_ptr<thread_pool_base> pool;
        hpx::threads::policies::local_queue_scheduler<>::init_parameter_type
            init(thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
                thread_queue_init, "core-static_queue_scheduler");

        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        if (thread_pool_init.mode_ &
            policies::scheduler_mode::do_background_work_only)
        {
            using local_sched_type =
                hpx::threads::policies::background_scheduler<>;

            auto sched = std::make_unique<local_sched_type>(init);

            // set the default scheduler flags
            sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

            // instantiate the pool
            pool = std::make_unique<
                hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
                HPX_MOVE(sched), thread_pool_init);
        }
        else
        {
            using local_sched_type =
                hpx::threads::policies::static_queue_scheduler<>;

            auto sched = std::make_unique<local_sched_type>(init);

            // set the default scheduler flags
            sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

            // conditionally set/unset this flag
            sched->update_scheduler_mode(
                policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
                full_mask);

            // instantiate the pool
            pool = std::make_unique<
                hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
                HPX_MOVE(sched), thread_pool_init);
        }

        pools_.push_back(HPX_MOVE(pool));
    }

    void threadmanager::create_scheduler_static_priority(
        thread_pool_init_parameters const& thread_pool_init,
        policies::thread_queue_init_parameters const& thread_queue_init,
        std::size_t const)
    {
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::static_priority_queue_scheduler<>;

        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-static_priority_queue_scheduler");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->remove_scheduler_mode(policies::scheduler_mode::enable_stealing |
                policies::scheduler_mode::enable_stealing_numa,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
    }

    void threadmanager::create_scheduler_abp_priority_fifo(
        [[maybe_unused]] thread_pool_init_parameters const& thread_pool_init,
        [[maybe_unused]] policies::thread_queue_init_parameters const&
            thread_queue_init,
        [[maybe_unused]] std::size_t numa_sensitive)
    {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
                hpx::threads::policies::lockfree_fifo>;

        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-abp_priority_queue_scheduler-fifo");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=abp-priority-fifo "
            "is not configured in this build. Please make sure 128bit "
            "atomics are available.");
#endif
    }

    void threadmanager::create_scheduler_abp_priority_lifo(
        [[maybe_unused]] thread_pool_init_parameters const& thread_pool_init,
        [[maybe_unused]] policies::thread_queue_init_parameters const&
            thread_queue_init,
        [[maybe_unused]] std::size_t numa_sensitive)
    {
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
                hpx::threads::policies::lockfree_lifo>;

        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-abp_priority_queue_scheduler-lifo");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=abp-priority-lifo "
            "is not configured in this build. Please make sure 128bit "
            "atomics are available.");
#endif
    }

    void threadmanager::create_scheduler_shared_priority(
        thread_pool_init_parameters const& thread_pool_init,
        policies::thread_queue_init_parameters const& thread_queue_init,
        std::size_t const numa_sensitive)
    {
        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::shared_priority_queue_scheduler<>;
        local_sched_type::init_parameter_type init(
            thread_pool_init.num_threads_, {1, 1, 1},
            thread_pool_init.affinity_data_, thread_queue_init,
            "core-shared_priority_queue_scheduler");

        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);
        auto sched = std::make_unique<local_sched_type>(init);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
    }

    void threadmanager::create_scheduler_local_workrequesting_fifo(
        [[maybe_unused]] thread_pool_init_parameters const& thread_pool_init,
        [[maybe_unused]] policies::thread_queue_init_parameters const&
            thread_queue_init,
        [[maybe_unused]] std::size_t const numa_sensitive)
    {
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_workrequesting_scheduler<>;

        local_sched_type::init_parameter_type const init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-local_workrequesting_scheduler-fifo");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=local-workrequesting-fifo "
            "is not configured in this build. Please make sure "
            "HPX_WITH_WORK_REQUESTING_SCHEDULERS is set to ON");
#endif
    }

    void threadmanager::create_scheduler_local_workrequesting_mc(
        [[maybe_unused]] thread_pool_init_parameters const& thread_pool_init,
        [[maybe_unused]] policies::thread_queue_init_parameters const&
            thread_queue_init,
        [[maybe_unused]] std::size_t const numa_sensitive)
    {
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        // set parameters for scheduler and pool instantiation and perform
        // compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_workrequesting_scheduler<std::mutex,
                hpx::threads::policies::concurrentqueue_fifo>;

        local_sched_type::init_parameter_type const init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-local_workrequesting_scheduler-mc");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=local-workrequesting-mc "
            "is not configured in this build. Please make sure "
            "HPX_WITH_WORK_REQUESTING_SCHEDULERS is set to ON");
#endif
    }

    void threadmanager::create_scheduler_local_workrequesting_lifo(
        [[maybe_unused]] thread_pool_init_parameters const& thread_pool_init,
        [[maybe_unused]] policies::thread_queue_init_parameters const&
            thread_queue_init,
        [[maybe_unused]] std::size_t numa_sensitive)
    {
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        // set parameters for scheduler and pool instantiation and
        // perform compatibility checks
        std::size_t const num_high_priority_queues =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.thread_queue.high_priority_queues",
                thread_pool_init.num_threads_);
        detail::check_num_high_priority_queues(
            thread_pool_init.num_threads_, num_high_priority_queues);

        // instantiate the scheduler
        using local_sched_type =
            hpx::threads::policies::local_workrequesting_scheduler<std::mutex,
                hpx::threads::policies::lockfree_lifo>;

        local_sched_type::init_parameter_type const init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            num_high_priority_queues, thread_queue_init,
            "core-local_workrequesting_scheduler-lifo");

        auto sched = std::make_unique<local_sched_type>(init);
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, !numa_sensitive,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=local-workrequesting-lifo "
            "is not configured in this build. Please make sure 128bit "
            "atomics are available.");
#endif
#else
        throw hpx::detail::command_line_error(
            "Command line option --hpx:queuing=local-workrequesting-lifo "
            "is not configured in this build. Please make sure "
            "HPX_WITH_WORK_REQUESTING_SCHEDULERS is set to ON");
#endif
    }

    void threadmanager::create_pools()
    {
        auto& rp = hpx::resource::get_partitioner();
        size_t const num_pools = rp.get_num_pools();
        std::size_t thread_offset = 0;
        std::size_t const max_idle_loop_count =
            hpx::util::get_entry_as<std::int64_t>(
                rtcfg_, "hpx.max_idle_loop_count", HPX_IDLE_LOOP_COUNT_MAX);
        std::size_t const max_busy_loop_count =
            hpx::util::get_entry_as<std::int64_t>(
                rtcfg_, "hpx.max_busy_loop_count", HPX_BUSY_LOOP_COUNT_MAX);

        std::size_t const numa_sensitive = hpx::util::get_entry_as<std::size_t>(
            rtcfg_, "hpx.numa_sensitive", 0);

        policies::thread_queue_init_parameters const thread_queue_init =
            get_init_parameters();

        std::size_t max_background_threads =
            hpx::util::get_entry_as<std::size_t>(rtcfg_,
                "hpx.max_background_threads",
                (std::numeric_limits<std::size_t>::max)());

        if (!rtcfg_.enable_networking())
        {
            max_background_threads = 0;
        }

        // instantiate the pools
        for (size_t i = 0; i != num_pools; i++)
        {
            std::string name = rp.get_pool_name(i);
            resource::scheduling_policy const sched_type =
                rp.which_scheduler(name);
            std::size_t num_threads_in_pool = rp.get_num_threads(i);
            policies::scheduler_mode const scheduler_mode =
                rp.get_scheduler_mode(i);
            resource::background_work_function background_work =
                rp.get_background_work(i);

            // make sure the first thread-pool that gets instantiated is the
            // default one
            if (i == 0)
            {
                if (name != rp.get_default_pool_name())
                {
                    throw std::invalid_argument("Trying to instantiate pool " +
                        name +
                        " as first thread pool, but first thread pool must "
                        "be named " +
                        rp.get_default_pool_name());
                }
            }

            threads::detail::network_background_callback_type
                overall_background_work;
            if (!background_work.empty())
            {
                if (!network_background_callback_.empty())
                {
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
                    overall_background_work =
                        [this, background_work](std::size_t num_thread,
                            std::int64_t& t1, std::int64_t& t2) -> bool {
                        bool result = background_work(num_thread);
                        return network_background_callback_(
                                   num_thread, t1, t2) ||
                            result;
                    };
#else
                    overall_background_work =
                        [this, background_work](
                            std::size_t const num_thread) -> bool {
                        bool const result = background_work(num_thread);
                        return network_background_callback_(num_thread) ||
                            result;
                    };
#endif
                }
                else
                {
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
                    overall_background_work =
                        [background_work](std::size_t num_thread, std::int64_t&,
                            std::int64_t&) -> bool {
                        return background_work(num_thread);
                    };
#else
                    overall_background_work = background_work;
#endif
                }

                max_background_threads =
                    (std::max) (num_threads_in_pool, max_background_threads);
            }
            else
            {
                overall_background_work = network_background_callback_;
            }

            thread_pool_init_parameters thread_pool_init(name, i,
                scheduler_mode, num_threads_in_pool, thread_offset, notifier_,
                rp.get_affinity_data(), overall_background_work,
                max_background_threads, max_idle_loop_count,
                max_busy_loop_count);

            switch (sched_type)
            {
            case resource::scheduling_policy::user_defined:
                create_scheduler_user_defined(rp.get_pool_creator(i),
                    thread_pool_init, thread_queue_init);
                break;

            case resource::scheduling_policy::local:
                create_scheduler_local(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::local_priority_fifo:
                create_scheduler_local_priority_fifo(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::local_priority_lifo:
                create_scheduler_local_priority_lifo(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::static_:
                create_scheduler_static(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::static_priority:
                create_scheduler_static_priority(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::local_workrequesting_fifo:
                create_scheduler_local_workrequesting_fifo(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::local_workrequesting_lifo:
                create_scheduler_local_workrequesting_lifo(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::local_workrequesting_mc:
                create_scheduler_local_workrequesting_mc(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::abp_priority_fifo:
                create_scheduler_abp_priority_fifo(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::abp_priority_lifo:
                create_scheduler_abp_priority_lifo(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::shared_priority:
                create_scheduler_shared_priority(
                    thread_pool_init, thread_queue_init, numa_sensitive);
                break;

            case resource::scheduling_policy::unspecified:
                throw std::invalid_argument(
                    "cannot instantiate a thread-manager if the thread-pool" +
                    name + " has an unspecified scheduler type");
            }

            // update the thread_offset for the next pool
            thread_offset += num_threads_in_pool;
        }

        // fill the thread-lookup table
        for (auto const& pool_iter : pools_)
        {
            std::size_t const nt =
                rp.get_num_threads(pool_iter->get_pool_index());
            for (std::size_t i = 0; i < nt; i++)
            {
                threads_lookup_.emplace_back(pool_iter->get_pool_id());
            }
        }
    }

    threadmanager::~threadmanager() = default;

    void threadmanager::init() const
    {
        auto const& rp = hpx::resource::get_partitioner();
        std::size_t threads_offset = 0;

        // initialize all pools
        for (auto&& pool_iter : pools_)
        {
            std::size_t const num_threads_in_pool =
                rp.get_num_threads(pool_iter->get_pool_index());
            pool_iter->init(num_threads_in_pool, threads_offset);
            threads_offset += num_threads_in_pool;
        }
    }

    void threadmanager::print_pools(std::ostream& os) const
    {
        os << "The thread-manager owns " << pools_.size()    //-V128
           << " pool(s) : \n";

        for (auto&& pool_iter : pools_)
        {
            pool_iter->print_pool(os);
        }
    }

    thread_pool_base& threadmanager::default_pool() const
    {
        HPX_ASSERT(!pools_.empty());
        return *pools_[0];
    }

    thread_pool_base& threadmanager::get_pool(
        std::string const& pool_name) const
    {
        // if the given pool_name is default, we don't need to look for it,
        // we must always return pool 0
        if (pool_name == "default" ||
            pool_name == resource::get_partitioner().get_default_pool_name())
        {
            return default_pool();
        }

        // now check the other pools - no need to check pool 0 again, so ++begin
        auto const pool = std::find_if(++pools_.begin(), pools_.end(),
            [&pool_name](pool_type const& itp) -> bool {
                return (itp->get_pool_name() == pool_name);
            });

        if (pool != pools_.end())
        {
            return **pool;
        }

        //! FIXME Add names of available pools?
        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "threadmanager::get_pool",
            "the resource partitioner does not own a thread pool named '{}'.\n",
            pool_name);
    }

    thread_pool_base& threadmanager::get_pool(pool_id_type const& pool_id) const
    {
        return get_pool(pool_id.name());
    }

    thread_pool_base& threadmanager::get_pool(
        std::size_t const thread_index) const
    {
        return get_pool(threads_lookup_[thread_index]);
    }

    bool threadmanager::pool_exists(std::string const& pool_name) const
    {
        // if the given pool_name is default, we don't need to look for it,
        // we must always return pool 0
        if (pool_name == "default" ||
            pool_name == resource::get_partitioner().get_default_pool_name())
        {
            return true;
        }

        // now check the other pools - no need to check pool 0 again, so ++begin
        auto const pool = std::find_if(++pools_.begin(), pools_.end(),
            [&pool_name](pool_type const& itp) -> bool {
                return (itp->get_pool_name() == pool_name);
            });

        if (pool != pools_.end())
        {
            return true;
        }

        return false;
    }

    bool threadmanager::pool_exists(std::size_t const pool_index) const
    {
        return pool_index < pools_.size();
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t threadmanager::get_thread_count(
        thread_schedule_state const state, thread_priority const priority,
        std::size_t const num_thread, bool const reset) const
    {
        std::int64_t total_count = 0;
        std::lock_guard<mutex_type> lk(mtx_);

        for (auto const& pool_iter : pools_)
        {
            total_count +=
                pool_iter->get_thread_count(state, priority, num_thread, reset);
        }

        return total_count;
    }

    std::int64_t threadmanager::get_idle_core_count() const
    {
        std::int64_t total_count = 0;
        std::lock_guard<mutex_type> lk(mtx_);

        for (auto const& pool_iter : pools_)
        {
            total_count += pool_iter->get_idle_core_count();
        }

        return total_count;
    }

    mask_type threadmanager::get_idle_core_mask() const
    {
        mask_type mask = mask_type();
        resize(mask, hardware_concurrency());

        std::lock_guard<mutex_type> lk(mtx_);

        for (auto const& pool_iter : pools_)
        {
            pool_iter->get_idle_core_mask(mask);
        }

        return mask;
    }

    std::int64_t threadmanager::get_background_thread_count() const
    {
        std::int64_t total_count = 0;
        std::lock_guard<mutex_type> lk(mtx_);

        for (auto& pool_iter : pools_)
        {
            total_count += pool_iter->get_background_thread_count();
        }

        return total_count;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enumerate all matching threads
    bool threadmanager::enumerate_threads(
        hpx::function<bool(thread_id_type)> const& f,
        thread_schedule_state const state) const
    {
        std::lock_guard<mutex_type> lk(mtx_);
        bool result = true;

        for (auto& pool_iter : pools_)
        {
            result = result && pool_iter->enumerate_threads(f, state);
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Abort all threads which are in suspended state. This will set
    // the state of all suspended threads to \a pending while
    // supplying the wait_abort extended state flag
    void threadmanager::abort_all_suspended_threads() const
    {
        std::lock_guard<mutex_type> lk(mtx_);
        for (auto const& pool_iter : pools_)
        {
            pool_iter->abort_all_suspended_threads();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Clean up terminated threads. This deletes all threads which
    // have been terminated but which are still held in the queue
    // of terminated threads. Some schedulers might not do anything
    // here.
    bool threadmanager::cleanup_terminated(bool const delete_all) const
    {
        std::lock_guard<mutex_type> lk(mtx_);
        bool result = true;

        for (auto const& pool_iter : pools_)
        {
            result = pool_iter->cleanup_terminated(delete_all) && result;
        }

        return result;
    }

    std::size_t threadmanager::get_os_thread_count() const
    {
        std::lock_guard<mutex_type> lk(mtx_);
        std::size_t total = 0;
        for (auto& pool_iter : pools_)
        {
            total += pool_iter->get_os_thread_count();
        }
        return total;
    }

    std::thread& threadmanager::get_os_thread_handle(
        std::size_t const num_thread) const
    {
        std::lock_guard<mutex_type> lk(mtx_);
        pool_id_type const id = threads_lookup_[num_thread];
        thread_pool_base& pool = get_pool(id);
        return pool.get_os_thread_handle(num_thread);
    }

    void threadmanager::report_error(
        std::size_t const num_thread, std::exception_ptr const& e) const
    {
        // propagate the error reporting to all pools, which in turn
        // will propagate to schedulers
        for (auto& pool_iter : pools_)
        {
            pool_iter->report_error(num_thread, e);
        }
    }

    mask_type threadmanager::get_used_processing_units() const
    {
        auto total_used_processing_punits = mask_type();
        threads::resize(total_used_processing_punits,
            static_cast<std::size_t>(hardware_concurrency()));

        for (auto& pool_iter : pools_)
        {
            total_used_processing_punits |=
                pool_iter->get_used_processing_units();
        }

        return total_used_processing_punits;
    }

    hwloc_bitmap_ptr threadmanager::get_pool_numa_bitmap(
        std::string const& pool_name) const
    {
        return get_pool(pool_name).get_numa_domain_bitmap();
    }

    void threadmanager::set_scheduler_mode(
        threads::policies::scheduler_mode const mode,
        hpx::threads::mask_cref_type pu_mask) const noexcept
    {
        for (auto const& pool_iter : pools_)
        {
            pool_iter->get_scheduler()->set_scheduler_mode(mode, pu_mask);
        }
    }

    void threadmanager::add_scheduler_mode(
        threads::policies::scheduler_mode const mode,
        hpx::threads::mask_cref_type pu_mask) const noexcept
    {
        for (auto const& pool_iter : pools_)
        {
            pool_iter->get_scheduler()->add_scheduler_mode(mode, pu_mask);
        }
    }

    void threadmanager::add_remove_scheduler_mode(
        threads::policies::scheduler_mode const to_add_mode,
        threads::policies::scheduler_mode const to_remove_mode,
        hpx::threads::mask_cref_type pu_mask) const noexcept
    {
        for (auto const& pool_iter : pools_)
        {
            pool_iter->get_scheduler()->add_remove_scheduler_mode(
                to_add_mode, to_remove_mode, pu_mask);
        }
    }

    void threadmanager::remove_scheduler_mode(
        threads::policies::scheduler_mode const mode,
        hpx::threads::mask_cref_type pu_mask) const noexcept
    {
        for (auto const& pool_iter : pools_)
        {
            pool_iter->get_scheduler()->remove_scheduler_mode(mode, pu_mask);
        }
    }

    void threadmanager::reset_thread_distribution() const noexcept
    {
        for (auto const& pool_iter : pools_)
        {
            pool_iter->reset_thread_distribution();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void threadmanager::register_thread(
        thread_init_data& data, thread_id_ref_type& id, error_code& ec) const
    {
        thread_pool_base* pool;
        if (auto const* thrd_data = get_self_id_data())
        {
            pool = thrd_data->get_scheduler_base()->get_parent_pool();
        }
        else
        {
            pool = &default_pool();
        }
        pool->create_thread(data, id, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    thread_id_ref_type threadmanager::register_work(
        thread_init_data& data, error_code& ec) const
    {
        thread_pool_base* pool;
        if (auto const* thrd_data = get_self_id_data())
        {
            pool = thrd_data->get_scheduler_base()->get_parent_pool();
        }
        else
        {
            pool = &default_pool();
        }
        return pool->create_work(data, ec);
    }

    void threadmanager::init_tss(std::size_t const global_thread_num)
    {
        detail::set_global_thread_num_tss(global_thread_num);
    }

    void threadmanager::deinit_tss()
    {
        detail::set_global_thread_num_tss(static_cast<std::size_t>(-1));
    }

    ///////////////////////////////////////////////////////////////////////////
    inline constexpr std::size_t all_threads = static_cast<std::size_t>(-1);

    std::int64_t threadmanager::get_queue_length(bool const reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_queue_length(all_threads, reset);
        return result;
    }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
    std::int64_t threadmanager::get_average_thread_wait_time(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_average_thread_wait_time(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_average_task_wait_time(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_average_task_wait_time(all_threads, reset);
        return result;
    }
#endif

    std::int64_t threadmanager::get_cumulative_duration(bool const reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_cumulative_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_thread_count_unknown(bool const reset) const
    {
        return get_thread_count(thread_schedule_state::unknown,
            thread_priority::default_, static_cast<std::size_t>(-1), reset);
    }

    std::int64_t threadmanager::get_thread_count_active(bool const reset) const
    {
        return get_thread_count(thread_schedule_state::active,
            thread_priority::default_, static_cast<std::size_t>(-1), reset);
    }

    std::int64_t threadmanager::get_thread_count_pending(bool const reset) const
    {
        return get_thread_count(thread_schedule_state::pending,
            thread_priority::default_, static_cast<std::size_t>(-1), reset);
    }

    std::int64_t threadmanager::get_thread_count_suspended(
        bool const reset) const
    {
        return get_thread_count(thread_schedule_state::suspended,
            thread_priority::default_, static_cast<std::size_t>(-1), reset);
    }

    std::int64_t threadmanager::get_thread_count_terminated(
        bool const reset) const
    {
        return get_thread_count(thread_schedule_state::terminated,
            thread_priority::default_, static_cast<std::size_t>(-1), reset);
    }

    std::int64_t threadmanager::get_thread_count_staged(bool const reset) const
    {
        return get_thread_count(thread_schedule_state::staged,
            thread_priority::default_, static_cast<std::size_t>(-1), reset);
    }

#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
    std::int64_t threadmanager::get_background_work_duration(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_background_work_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_background_overhead(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_background_overhead(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_background_send_duration(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_background_send_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_background_send_overhead(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_background_send_overhead(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_background_receive_duration(
        bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_background_receive_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_background_receive_overhead(
        bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_background_receive_overhead(all_threads, reset);
        return result;
    }
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS

#ifdef HPX_HAVE_THREAD_IDLE_RATES
    std::int64_t threadmanager::avg_idle_rate(bool reset) const noexcept
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->avg_idle_rate(all_threads, reset);
        return result;
    }

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
    std::int64_t threadmanager::avg_creation_idle_rate(
        bool reset) const noexcept
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->avg_creation_idle_rate(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::avg_cleanup_idle_rate(bool reset) const noexcept
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->avg_cleanup_idle_rate(all_threads, reset);
        return result;
    }
#endif
#endif

#ifdef HPX_HAVE_THREAD_CUMULATIVE_COUNTS
    std::int64_t threadmanager::get_executed_threads(bool reset) const noexcept
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_executed_threads(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_executed_thread_phases(
        bool reset) const noexcept
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_executed_thread_phases(all_threads, reset);
        return result;
    }

#ifdef HPX_HAVE_THREAD_IDLE_RATES
    std::int64_t threadmanager::get_thread_duration(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_thread_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_thread_phase_duration(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_thread_phase_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_thread_overhead(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_thread_overhead(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_thread_phase_overhead(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_thread_phase_overhead(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_cumulative_thread_duration(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_cumulative_thread_duration(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_cumulative_thread_overhead(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_cumulative_thread_overhead(all_threads, reset);
        return result;
    }
#endif
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
    std::int64_t threadmanager::get_num_pending_misses(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_num_pending_misses(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_num_pending_accesses(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_num_pending_accesses(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_num_stolen_from_pending(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result +=
                pool_iter->get_num_stolen_from_pending(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_num_stolen_from_staged(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_num_stolen_from_staged(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_num_stolen_to_pending(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_num_stolen_to_pending(all_threads, reset);
        return result;
    }

    std::int64_t threadmanager::get_num_stolen_to_staged(bool reset) const
    {
        std::int64_t result = 0;
        for (auto const& pool_iter : pools_)
            result += pool_iter->get_num_stolen_to_staged(all_threads, reset);
        return result;
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    bool threadmanager::run() const
    {
        std::unique_lock<mutex_type> lk(mtx_);

        // the main thread needs to have a unique thread_num worker threads are
        // numbered 0 to N-1, so we can use N for this thread
        auto const& rp = hpx::resource::get_partitioner();
        init_tss(rp.get_num_threads());

#ifdef HPX_HAVE_TIMER_POOL
        LTM_(info).format("run: running timer pool");
        timer_pool_.run(false);
#endif

        for (auto const& pool_iter : pools_)
        {
            std::size_t const num_threads_in_pool =
                rp.get_num_threads(pool_iter->get_pool_name());

            if (pool_iter->get_os_thread_count() != 0 ||
                pool_iter->has_reached_state(hpx::state::running))
            {
                return true;    // do nothing if already running
            }

            if (!pool_iter->run(lk, num_threads_in_pool))
            {
#ifdef HPX_HAVE_TIMER_POOL
                timer_pool_.stop();
#endif
                return false;
            }

            // set all states of all schedulers to "running"
            if (policies::scheduler_base* sched = pool_iter->get_scheduler())
                sched->set_all_states(hpx::state::running);
        }

        LTM_(info).format("run: running");
        return true;
    }

    void threadmanager::stop(bool const blocking) const
    {
        LTM_(info).format("stop: blocking({})", blocking ? "true" : "false");

        std::unique_lock<mutex_type> lk(mtx_);
        for (auto const& pool_iter : pools_)
        {
            pool_iter->stop(lk, blocking);
        }
        deinit_tss();
    }

    bool threadmanager::is_busy() const
    {
        bool busy = false;
        for (auto const& pool_iter : pools_)
        {
            busy = busy || pool_iter->is_busy();
        }
        return busy;
    }

    bool threadmanager::is_idle() const
    {
        bool idle = true;
        for (auto const& pool_iter : pools_)
        {
            idle = idle && pool_iter->is_idle();
        }
        return idle;
    }

    void threadmanager::wait() const
    {
        auto const shutdown_check_count = util::get_entry_as<std::size_t>(
            rtcfg_, "hpx.shutdown_check_count", 10);
        hpx::util::detail::yield_while_count(
            [this]() { return is_busy(); }, shutdown_check_count);
    }

    bool threadmanager::wait_for(
        hpx::chrono::steady_duration const& rel_time) const
    {
        auto const shutdown_check_count = util::get_entry_as<std::size_t>(
            rtcfg_, "hpx.shutdown_check_count", 10);
        return hpx::util::detail::yield_while_count_timeout(
            [this]() { return is_busy(); }, shutdown_check_count, rel_time);
    }

    void threadmanager::suspend() const
    {
        wait();

        if (threads::get_self_ptr())
        {
            std::vector<hpx::future<void>> fs;

            for (auto& pool_iter : pools_)
            {
                fs.emplace_back(suspend_pool(*pool_iter));
            }

            hpx::wait_all(fs);
        }
        else
        {
            for (auto const& pool_iter : pools_)
            {
                pool_iter->suspend_direct();
            }
        }
    }

    void threadmanager::resume() const
    {
        if (threads::get_self_ptr())
        {
            std::vector<hpx::future<void>> fs;

            for (auto& pool_iter : pools_)
            {
                fs.emplace_back(resume_pool(*pool_iter));
            }
            hpx::wait_all(fs);
        }
        else
        {
            for (auto const& pool_iter : pools_)
            {
                pool_iter->resume_direct();
            }
        }
    }

    hpx::state threadmanager::status() const
    {
        hpx::state result(hpx::state::last_valid_runtime_state);

        for (auto& pool_iter : pools_)
        {
            hpx::state s = pool_iter->get_state();
            result = (std::min) (result, s);
        }

        return result;
    }
}    // namespace hpx::threads
