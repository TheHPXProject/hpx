//  Copyright (c) 2007-2026 Hartmut Kaiser
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
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_local.hpp>
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
#include <hpx/modules/schedulers.hpp>
#endif
#include <hpx/modules/threadmanager.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/manage_counter_type.hpp>
#include <hpx/performance_counters/threadmanager_counter_types.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::detail {

    using threadmanager_counter_func = std::int64_t (threads::threadmanager::*)(
        bool reset) const;
    using threadpool_counter_func = std::int64_t (threads::thread_pool_base::*)(
        std::size_t num_thread, bool reset);

    naming::gid_type locality_pool_thread_counter_creator(
        threads::threadmanager* tm, threadmanager_counter_func total_func,
        threadpool_counter_func pool_func, counter_info const& info,
        error_code& ec);

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
    naming::gid_type queue_wait_time_counter_creator(threads::threadmanager* tm,
        threadmanager_counter_func total_func,
        threadpool_counter_func pool_func, counter_info const& info,
        error_code& ec)
    {
        naming::gid_type gid = locality_pool_thread_counter_creator(
            tm, total_func, pool_func, info, ec);

        if (!ec)
        {
            threads::policies::set_maintain_queue_wait_times_enabled(true);
        }
        return gid;
    }
#endif

    naming::gid_type locality_pool_thread_counter_creator(
        threads::threadmanager* tm, threadmanager_counter_func total_func,
        threadpool_counter_func pool_func, counter_info const& info,
        error_code& ec)
    {
        // verify the validity of the counter instance name
        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }

        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "queue_length_counter_creator",
                "invalid counter instance parent name: {}",
                paths.parentinstancename_);
            return naming::invalid_gid;
        }

        threads::thread_pool_base& pool = tm->default_pool();
        if (paths.instancename_ == "total" && paths.instanceindex_ == -1)
        {
            // overall counter
            using detail::create_raw_counter;
            hpx::function<std::int64_t(bool)> f =
                hpx::bind_front(total_func, tm);
            return create_raw_counter(info, HPX_MOVE(f), ec);
        }
        else if (paths.instancename_ == "pool")
        {
            if (paths.instanceindex_ >= 0 &&
                static_cast<std::size_t>(paths.instanceindex_) <
                    hpx::resource::get_num_thread_pools())
            {
                // specific for given pool counter
                threads::thread_pool_base& pool_instance =
                    hpx::resource::get_thread_pool(paths.instanceindex_);

                using detail::create_raw_counter;
                hpx::function<std::int64_t(bool)> f =
                    hpx::bind_front(pool_func, &pool_instance,
                        static_cast<std::size_t>(paths.subinstanceindex_));
                return create_raw_counter(info, HPX_MOVE(f), ec);
            }
        }
        else if (paths.instancename_ == "worker-thread" &&
            paths.instanceindex_ >= 0 &&
            static_cast<std::size_t>(paths.instanceindex_) <
                pool.get_os_thread_count())
        {
            // specific counter from default
            using detail::create_raw_counter;
            hpx::function<std::int64_t(bool)> f = hpx::bind_front(pool_func,
                &pool, static_cast<std::size_t>(paths.instanceindex_));
            return create_raw_counter(info, HPX_MOVE(f), ec);
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "locality_pool_thread_counter_creator",
            "invalid counter instance name: {}", paths.instancename_);
        return naming::invalid_gid;
    }

    // scheduler utilization counter creation function
    naming::gid_type scheduler_utilization_counter_creator(
        threads::threadmanager const* tm, counter_info const& info,
        error_code& ec)
    {
        // verify the validity of the counter instance name
        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        // /scheduler{locality#%d/total}/utilization/instantaneous
        // /scheduler{locality#%d/pool#%s/total}/utilization/instantaneous
        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "scheduler_utilization_creator",
                "invalid counter instance parent name: {}",
                paths.parentinstancename_);
            return naming::invalid_gid;
        }

        using detail::create_raw_counter;

        threads::thread_pool_base& pool = tm->default_pool();
        if (paths.instancename_ == "total" && paths.instanceindex_ == -1)
        {
            // counter for default pool
            hpx::function<std::int64_t()> f = hpx::bind_back(
                &threads::thread_pool_base::get_scheduler_utilization, &pool);
            return create_raw_counter(info, HPX_MOVE(f), ec);
        }
        else if (paths.instancename_ == "pool")
        {
            if (paths.instanceindex_ < 0)
            {
                // counter for default pool
                hpx::function<std::int64_t()> f = hpx::bind_back(
                    &threads::thread_pool_base::get_scheduler_utilization,
                    &pool);
                return create_raw_counter(info, HPX_MOVE(f), ec);
            }
            else if (static_cast<std::size_t>(paths.instanceindex_) <
                hpx::resource::get_num_thread_pools())
            {
                // counter specific for given pool
                threads::thread_pool_base& pool_instance =
                    hpx::resource::get_thread_pool(paths.instanceindex_);

                hpx::function<std::int64_t()> f = hpx::bind_back(
                    &threads::thread_pool_base::get_scheduler_utilization,
                    &pool_instance);
                return create_raw_counter(info, HPX_MOVE(f), ec);
            }
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "scheduler_utilization_creator",
            "invalid counter instance name: {}", paths.instancename_);
        return naming::invalid_gid;
    }

    ///////////////////////////////////////////////////////////////////////
    // locality/pool/worker-thread counter creation function with no total
    // /threads{locality#%d/worker-thread#%d}/idle-loop-count/instantaneous
    // /threads{locality#%d/pool#%s/worker-thread#%d}/idle-loop-count/instantaneous
    naming::gid_type locality_pool_thread_no_total_counter_creator(
        threads::threadmanager const* tm, threadpool_counter_func pool_func,
        counter_info const& info, error_code& ec)
    {
        // verify the validity of the counter instance name
        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "locality_pool_thread_no_total_counter_creator",
                "invalid counter instance parent name: {}",
                paths.parentinstancename_);
            return naming::invalid_gid;
        }

        threads::thread_pool_base& pool = tm->default_pool();
        if (paths.instancename_ == "total" && paths.instanceindex_ == -1)
        {
            // overall counter, not supported
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "locality_pool_thread_no_total_counter_creator",
                "invalid counter instance name: {} 'total' is not supported",
                paths.instancename_);
        }
        else if (paths.instancename_ == "pool")
        {
            if (paths.instanceindex_ >= 0 &&
                static_cast<std::size_t>(paths.instanceindex_) <
                    hpx::resource::get_num_thread_pools())
            {
                // specific for given pool counter
                threads::thread_pool_base& pool_instance =
                    hpx::resource::get_thread_pool(paths.instanceindex_);

                using detail::create_raw_counter;
                hpx::function<std::int64_t(bool)> f =
                    hpx::bind_front(pool_func, &pool_instance,
                        static_cast<std::size_t>(paths.subinstanceindex_));
                return create_raw_counter(info, HPX_MOVE(f), ec);
            }
        }
        else if (paths.instancename_ == "worker-thread" &&
            paths.instanceindex_ >= 0 &&
            static_cast<std::size_t>(paths.instanceindex_) <
                pool.get_os_thread_count())
        {
            // specific counter
            using detail::create_raw_counter;
            hpx::function<std::int64_t(bool)> f = hpx::bind_front(pool_func,
                &pool, static_cast<std::size_t>(paths.instanceindex_));
            return create_raw_counter(info, HPX_MOVE(f), ec);
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "locality_pool_thread_no_total_counter_creator",
            "invalid counter instance name: {}", paths.instancename_);
        return naming::invalid_gid;
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type counter_creator(counter_info const& info,
        counter_path_elements const& paths,
        hpx::function<std::int64_t(bool)> const& total_creator,
        hpx::function<std::int64_t(bool)> const& individual_creator,
        char const* individual_name, std::size_t individual_count,
        error_code& ec)
    {
        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter, "counter_creator",
                "invalid counter instance parent name: {}",
                paths.parentinstancename_);
            return naming::invalid_gid;
        }

        if (!total_creator.empty() && paths.instancename_ == "total" &&
            paths.instanceindex_ == -1)
        {
            // overall counter
            using detail::create_raw_counter;
            return create_raw_counter(info, total_creator, ec);
        }
        else if (!individual_creator.empty() &&
            paths.instancename_ == individual_name &&
            paths.instanceindex_ >= 0 &&
            static_cast<std::size_t>(paths.instanceindex_) < individual_count)
        {
            // specific counter
            using detail::create_raw_counter;
            return create_raw_counter(info, individual_creator, ec);
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter, "counter_creator",
            "invalid counter instance name: {}", paths.instancename_);
        return naming::invalid_gid;
    }

    ///////////////////////////////////////////////////////////////////////
    // thread counts counter creation function
#if defined(HPX_HAVE_COROUTINE_COUNTERS)
    naming::gid_type thread_counts_counter_creator(
        counter_info const& info, error_code& ec)
    {
        // verify the validity of the counter instance name
        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }

        struct creator_data
        {
            char const* const countername;
            hpx::function<std::int64_t(bool)> total_func;
            hpx::function<std::int64_t(bool)> individual_func;
            char const* const individual_name;
            std::size_t individual_count;
        };

        creator_data data[] = {
            // /threads{locality#%d/total}/count/stack-recycles
            {"count/stack-recycles",
                hpx::bind_front(&threads::coroutine_type::impl_type::
                        get_stack_recycle_count),
                hpx::function<std::uint64_t(bool)>(), "", 0},
#if !defined(HPX_WINDOWS) && !defined(HPX_HAVE_GENERIC_CONTEXT_COROUTINES)
            // /threads{locality#%d/total}/count/stack-unbinds
            {"count/stack-unbinds",
                hpx::bind_front(&threads::coroutine_type::impl_type::
                        get_stack_unbind_count),
                hpx::function<std::uint64_t(bool)>(), "", 0},
#endif
        };
        std::size_t const data_size = sizeof(data) / sizeof(data[0]);

        for (creator_data const* d = data; d < &d[data_size]; ++d)
        {
            if (paths.countername_ == d->countername)
            {
                return counter_creator(info, paths, d->total_func,
                    d->individual_func, d->individual_name, d->individual_count,
                    ec);
            }
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "thread_counts_counter_creator",
            "invalid counter instance name: {}", paths.instancename_);
        return naming::invalid_gid;
    }
#endif
}    // namespace hpx::performance_counters::detail

namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    void register_threadmanager_counter_types(threads::threadmanager& tm)
    {
#if defined(HPX_HAVE_COROUTINE_COUNTERS)
        create_counter_func counts_creator(
            hpx::bind_front(&detail::thread_counts_counter_creator));
#endif

        generic_counter_type_data const counter_types[] = {
            // length of thread queue(s)
            {"/threadqueue/length", counter_type::raw,
                "returns the current queue length for the referenced queue",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_queue_length,
                    &threads::thread_pool_base::get_queue_length),
                &locality_pool_thread_counter_discoverer, ""},
#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
            // average thread wait time for queue(s)
            {"/threads/wait-time/pending", counter_type::average_timer,
                "returns the average wait time of pending threads for the "
                "referenced queue",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::queue_wait_time_counter_creator, &tm,
                    &threads::threadmanager::get_average_thread_wait_time,
                    &threads::thread_pool_base::get_average_thread_wait_time),
                &locality_pool_thread_counter_discoverer, "ns"},
            // average task wait time for queue(s)
            {"/threads/wait-time/staged", counter_type::average_timer,
                "returns the average wait time of staged threads (task "
                "descriptions) for the referenced queue",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::queue_wait_time_counter_creator, &tm,
                    &threads::threadmanager::get_average_task_wait_time,
                    &threads::thread_pool_base::get_average_task_wait_time),
                &locality_pool_thread_counter_discoverer, "ns"},
#endif
#ifdef HPX_HAVE_THREAD_IDLE_RATES
            // idle rate
            {"/threads/idle-rate", counter_type::average_count,
                "returns the idle rate for the referenced object",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::avg_idle_rate,
                    &threads::thread_pool_base::avg_idle_rate),
                &locality_pool_thread_counter_discoverer, "0.01%"},
#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
            {"/threads/creation-idle-rate", counter_type::average_count,
                "returns the % of idle-rate spent creating HPX-threads for the "
                "referenced object",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::avg_creation_idle_rate,
                    &threads::thread_pool_base::avg_creation_idle_rate),
                &locality_pool_thread_counter_discoverer, "0.01%"},
            {"/threads/cleanup-idle-rate", counter_type::average_count,
                "returns the % of time spent cleaning up terminated "
                "HPX-threads for the referenced object",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::avg_cleanup_idle_rate,
                    &threads::thread_pool_base::avg_cleanup_idle_rate),
                &locality_pool_thread_counter_discoverer, "0.01%"},
#endif
#endif
#ifdef HPX_HAVE_THREAD_CUMULATIVE_COUNTS
            // thread counts
            {"/threads/count/cumulative",
                counter_type::monotonically_increasing,
                "returns the overall number of executed (retired) HPX-threads "
                "for the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_executed_threads,
                    &threads::thread_pool_base::get_executed_threads),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/cumulative-phases",
                counter_type::monotonically_increasing,
                "returns the overall number of HPX-thread phases executed for "
                "the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_executed_thread_phases,
                    &threads::thread_pool_base::get_executed_thread_phases),
                &locality_pool_thread_counter_discoverer, ""},
#ifdef HPX_HAVE_THREAD_IDLE_RATES
            {"/threads/time/average", counter_type::average_timer,
                "returns the average time spent executing one HPX-thread",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_duration,
                    &threads::thread_pool_base::get_thread_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/time/average-phase", counter_type::average_timer,
                "returns the average time spent executing one HPX-thread phase",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_phase_duration,
                    &threads::thread_pool_base::get_thread_phase_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/time/average-overhead", counter_type::average_timer,
                "returns average overhead time executing one HPX-thread",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_overhead,
                    &threads::thread_pool_base::get_thread_overhead),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/time/average-phase-overhead",
                counter_type::average_timer,
                "returns average overhead time executing one HPX-thread phase",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_phase_overhead,
                    &threads::thread_pool_base::get_thread_phase_overhead),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/time/cumulative", counter_type::elapsed_time,
                "returns the cumulative time spent executing HPX-threads",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm,
                    &threads::threadmanager::get_cumulative_thread_duration,
                    &threads::thread_pool_base::get_cumulative_thread_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/time/cumulative-overhead", counter_type::elapsed_time,
                "returns the cumulative overhead time incurred by executing "
                "HPX threads",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm,
                    &threads::threadmanager::get_cumulative_thread_overhead,
                    &threads::thread_pool_base::get_cumulative_thread_overhead),
                &locality_pool_thread_counter_discoverer, "ns"},
#endif
#endif

#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
            {"/threads/time/background-work-duration",
                counter_type::elapsed_time,
                "returns the overall time spent running background work",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_background_work_duration,
                    &threads::thread_pool_base::get_background_work_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/background-overhead", counter_type::aggregating,
                "returns the overall background overhead",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_background_overhead,
                    &threads::thread_pool_base::get_background_overhead),
                &locality_pool_thread_counter_discoverer, "0.1%"},
            {"/threads/time/background-send-duration",
                counter_type::elapsed_time,
                "returns the overall time spent running background work "
                "related to sending parcels",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_background_send_duration,
                    &threads::thread_pool_base::get_background_send_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/background-send-overhead", counter_type::aggregating,
                "returns the overall background overhead "
                "related to sending parcels",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_background_send_overhead,
                    &threads::thread_pool_base::get_background_send_overhead),
                &locality_pool_thread_counter_discoverer, "0.1%"},
            {"/threads/time/background-receive-duration",
                counter_type::elapsed_time,
                "returns the overall time spent running background work "
                "related to receiving parcels",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm,
                    &threads::threadmanager::get_background_receive_duration,
                    &threads::thread_pool_base::
                        get_background_receive_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/background-receive-overhead", counter_type::aggregating,
                "returns the overall background overhead "
                "related to receiving parcels",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm,
                    &threads::threadmanager::get_background_receive_overhead,
                    &threads::thread_pool_base::
                        get_background_receive_overhead),
                &locality_pool_thread_counter_discoverer, "0.1%"},
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS

            {"/threads/time/overall", counter_type::elapsed_time,
                "returns the overall time spent running the scheduler on a "
                "core",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_cumulative_duration,
                    &threads::thread_pool_base::get_cumulative_duration),
                &locality_pool_thread_counter_discoverer, "ns"},
            {"/threads/count/instantaneous/all", counter_type::raw,
                "returns the overall current number of HPX-threads "
                "instantiated at the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_count_unknown,
                    &threads::thread_pool_base::get_thread_count_unknown),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/instantaneous/active", counter_type::raw,
                "returns the current number of active HPX-threads "
                "at the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_count_active,
                    &threads::thread_pool_base::get_thread_count_active),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/instantaneous/pending", counter_type::raw,
                "returns the current number of pending HPX-threads "
                "at the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_count_pending,
                    &threads::thread_pool_base::get_thread_count_pending),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/instantaneous/suspended", counter_type::raw,
                "returns the current number of suspended HPX-threads "
                "at the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_count_suspended,
                    &threads::thread_pool_base::get_thread_count_suspended),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/instantaneous/terminated", counter_type::raw,
                "returns the current number of terminated HPX-threads "
                "at the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_count_terminated,
                    &threads::thread_pool_base::get_thread_count_terminated),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/instantaneous/staged", counter_type::raw,
                "returns the current number of staged HPX-threads (task "
                "descriptions) "
                "at the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_thread_count_staged,
                    &threads::thread_pool_base::get_thread_count_staged),
                &locality_pool_thread_counter_discoverer, ""},
#if defined(HPX_HAVE_COROUTINE_COUNTERS)
            {"/threads/count/stack-recycles",
                counter_type::monotonically_increasing,
                "returns the total number of HPX-thread recycling operations "
                "performed for the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1, counts_creator,
                &locality_counter_discoverer, ""},
#if !defined(HPX_WINDOWS) && !defined(HPX_HAVE_GENERIC_CONTEXT_COROUTINES)
            {"/threads/count/stack-unbinds",
                counter_type::monotonically_increasing,
                "returns the total number of HPX-thread unbind (madvise) "
                "operations performed for the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1, counts_creator,
                &locality_counter_discoverer, ""},
#endif
#endif
#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
            {"/threads/count/pending-misses",
                counter_type::monotonically_increasing,
                "returns the number of times that the referenced worker-thread "
                "on the referenced locality failed to find pending HPX-threads "
                "in its associated queue",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_num_pending_misses,
                    &threads::thread_pool_base::get_num_pending_misses),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/pending-accesses",
                counter_type::monotonically_increasing,
                "returns the number of times that the referenced worker-thread "
                "on the referenced locality looked for pending HPX-threads "
                "in its associated queue",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_num_pending_accesses,
                    &threads::thread_pool_base::get_num_pending_accesses),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/stolen-from-pending",
                counter_type::monotonically_increasing,
                "returns the overall number of pending HPX-threads stolen by "
                "neighboring schedulers from &tm scheduler for the referenced "
                "locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_num_stolen_from_pending,
                    &threads::thread_pool_base::get_num_stolen_from_pending),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/stolen-from-staged",
                counter_type::monotonically_increasing,
                "returns the overall number of task descriptions stolen by "
                "neighboring schedulers from tm scheduler for the referenced "
                "locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_num_stolen_from_staged,
                    &threads::thread_pool_base::get_num_stolen_from_staged),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/stolen-to-pending",
                counter_type::monotonically_increasing,
                "returns the overall number of pending HPX-threads stolen from "
                "neighboring schedulers for the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_num_stolen_to_pending,
                    &threads::thread_pool_base::get_num_stolen_to_pending),
                &locality_pool_thread_counter_discoverer, ""},
            {"/threads/count/stolen-to-staged",
                counter_type::monotonically_increasing,
                "returns the overall number of task descriptions stolen from "
                "neighboring schedulers for the referenced locality",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(&detail::locality_pool_thread_counter_creator,
                    &tm, &threads::threadmanager::get_num_stolen_to_staged,
                    &threads::thread_pool_base::get_num_stolen_to_staged),
                &locality_pool_thread_counter_discoverer, ""},
#endif
            // scheduler utilization
            {"/scheduler/utilization/instantaneous", counter_type::raw,
                "returns the current scheduler utilization",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(
                    &detail::scheduler_utilization_counter_creator, &tm),
                &locality_pool_counter_discoverer, "%"},
            // idle-loop count
            {"/threads/idle-loop-count/instantaneous", counter_type::raw,
                "returns the current value of the scheduler idle-loop count",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(
                    &detail::locality_pool_thread_no_total_counter_creator, &tm,
                    &threads::thread_pool_base::get_idle_loop_count),
                &locality_pool_thread_no_total_counter_discoverer, ""},
            // busy-loop count
            {"/threads/busy-loop-count/instantaneous", counter_type::raw,
                "returns the current value of the scheduler busy-loop count",
                HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind_front(
                    &detail::locality_pool_thread_no_total_counter_creator, &tm,
                    &threads::thread_pool_base::get_busy_loop_count),
                &locality_pool_thread_no_total_counter_discoverer, ""}};

        install_counter_types(
            counter_types, sizeof(counter_types) / sizeof(counter_types[0]));
    }
}    // namespace hpx::performance_counters
