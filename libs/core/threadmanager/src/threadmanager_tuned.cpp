//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The local-priority-tuned-fifo scheduler is created here, apart from the
// other schedulers, so that its code doesn't change where the code for the
// other schedulers is placed.

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace hpx::threads {

    namespace detail {

        // defined in threadmanager.cpp
        void check_num_high_priority_queues(
            std::size_t num_threads, std::size_t num_high_priority_queues);
    }    // namespace detail

    namespace {

        // a task size given by its name (see get_task_size_name)
        threads::task_size get_task_size_entry(
            util::runtime_configuration const& cfg, char const* key,
            threads::task_size const default_size)
        {
            std::string const value = hpx::util::get_entry_as<std::string>(
                cfg, key, threads::get_task_size_name(default_size));
            for (threads::task_size const size : {threads::task_size::unknown,
                     threads::task_size::nano, threads::task_size::small_,
                     threads::task_size::medium, threads::task_size::large})
            {
                if (value == threads::get_task_size_name(size))
                {
                    return size;
                }
            }
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "threadmanager::get_scheduling_policy_init_parameters",
                "invalid value of {}: '{}' (expected unknown, nano, small, "
                "medium or large)",
                key, value);
        }
    }    // namespace

    policies::scheduling_policy_init_parameters
    threadmanager::get_scheduling_policy_init_parameters() const
    {
        // the switches and numbers of the rules of scheduling_policy
        policies::scheduling_policy_init_parameters policy_init;
        policy_init.wake_one_max_size_ = get_task_size_entry(rtcfg_,
            "hpx.scheduler_policy.wake_one_max_size",
            policy_init.wake_one_max_size_);
        policy_init.wake_target_only_ =
            hpx::util::get_entry_as<int>(rtcfg_,
                "hpx.scheduler_policy.wake_target_only",
                policy_init.wake_target_only_) != 0;
        policy_init.wake_if_nobody_spins_ =
            hpx::util::get_entry_as<int>(rtcfg_,
                "hpx.scheduler_policy.wake_if_nobody_spins",
                policy_init.wake_if_nobody_spins_) != 0;
        policy_init.wake_one_unless_placed_by_caller_ =
            hpx::util::get_entry_as<int>(rtcfg_,
                "hpx.scheduler_policy.wake_one_unless_placed_by_caller",
                policy_init.wake_one_unless_placed_by_caller_) != 0;
        policy_init.free_pass_after_wake_ =
            hpx::util::get_entry_as<int>(rtcfg_,
                "hpx.scheduler_policy.free_pass_after_wake",
                policy_init.free_pass_after_wake_) != 0;
        policy_init.idle_backoff_spin_round_pauses_ =
            hpx::util::get_entry_as<std::uint32_t>(rtcfg_,
                "hpx.idle_backoff_spin_round_pauses",
                policy_init.idle_backoff_spin_round_pauses_);
        return policy_init;
    }

    void threadmanager::create_scheduler_local_priority_tuned_fifo(
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
            hpx::threads::policies::local_priority_tuned_scheduler<>;

        local_sched_type::init_parameter_type init(
            local_sched_type::base_type::init_parameter_type(
                thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
                num_high_priority_queues, thread_queue_init,
                "core-local_priority_tuned_scheduler",
                static_cast<policies::numa_sensitive_mode>(numa_sensitive)),
            get_scheduling_policy_init_parameters());

        auto sched = std::make_unique<local_sched_type>(init);
        sched->set_tuner(policies::create_scheduler_tuner(
            hpx::util::get_entry_as<std::string>(
                rtcfg_, "hpx.scheduler_tuner", "static")));

        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                thread_pool_init.name_);

        // set the default scheduler flags
        sched->set_scheduler_mode(thread_pool_init.mode_, full_mask);

        // conditionally set/unset this flag
        sched->update_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, numa_sensitive != 2,
            full_mask);

        // instantiate the pool
        std::unique_ptr<thread_pool_base> pool = std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<local_sched_type>>(
            HPX_MOVE(sched), thread_pool_init);
        pools_.push_back(HPX_MOVE(pool));
    }
}    // namespace hpx::threads
