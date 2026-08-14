//  Copyright (c) 2017-2018 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(HPX_DEBUG)
//# define SHARED_PRIORITY_SCHEDULER_DEBUG 1
#endif

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/schedulers/lockfree_queue_backends.hpp>
#include <hpx/schedulers/macros.hpp>
#include <hpx/schedulers/queue_holder_numa.hpp>
#include <hpx/schedulers/queue_holder_thread.hpp>
#include <hpx/schedulers/thread_queue_mc.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <string_view>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>
#if defined(__linux) || defined(linux) || defined(__linux__)
#include <linux/unistd.h>
#include <sys/mman.h>
#endif

// #define SHARED_PRIORITY_SCHEDULER_DEBUG_NUMA

namespace hpx {

    HPX_CXX_CORE_EXPORT using print_onoff =
        hpx::debug::enable_print<SHARED_PRIORITY_SCHEDULER_DEBUG>;
    HPX_CXX_CORE_EXPORT using print_on = hpx::debug::enable_print<false>;

    HPX_CXX_CORE_EXPORT inline constexpr print_onoff spq_deb("SPQUEUE");
    HPX_CXX_CORE_EXPORT inline constexpr print_on spq_arr("SPQUEUE");
}    // namespace hpx

namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT using default_shared_priority_queue_scheduler_terminated_queue =
        lockfree_lifo;
#else
    HPX_CXX_CORE_EXPORT using default_shared_priority_queue_scheduler_terminated_queue =
        lockfree_fifo;
#endif

    // Holds core/queue ratios used by schedulers.
    HPX_CXX_CORE_EXPORT struct core_ratios
    {
        constexpr core_ratios(std::size_t high_priority,
            std::size_t normal_priority, std::size_t low_priority) noexcept
          : high_priority(high_priority)
          , normal_priority(normal_priority)
          , low_priority(low_priority)
        {
        }

        std::size_t high_priority;
        std::size_t normal_priority;
        std::size_t low_priority;
    };

    ///////////////////////////////////////////////////////////////////////////
    // The shared_priority_queue_scheduler maintains a set of high, normal, and
    // low priority queues. For each priority level there is a core/queue ratio
    // which determines how many cores share a single queue. If the high
    // priority core/queue ratio is 4 the first 4 cores will share a single high
    // priority queue, the next 4 will share another one and so on. In addition,
    // the shared_priority_queue_scheduler is NUMA-aware and takes NUMA
    // scheduling hints into account when creating and scheduling work.
    //
    // Warning: PendingQueuing lifo causes lockup on termination
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_shared_priority_queue_scheduler_terminated_queue>
    class shared_priority_queue_scheduler final : public scheduler_base
    {
    public:
        using has_periodic_maintenance = std::false_type;

        using thread_queue_type = thread_queue_mc<Mutex, PendingQueuing,
            PendingQueuing, TerminatedQueuing>;
        using thread_holder_type = queue_holder_thread<thread_queue_type>;

        struct init_parameter
        {
            init_parameter(std::size_t num_worker_threads,
                core_ratios const& cores_per_queue,
                detail::affinity_data const& affinity_data,
                thread_queue_init_parameters const& thread_queue_init,
                char const* description = "shared_priority_queue_scheduler")
              : num_worker_threads_(num_worker_threads)
              , cores_per_queue_(cores_per_queue)
              , thread_queue_init_(thread_queue_init)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            init_parameter(std::size_t num_worker_threads,
                core_ratios const& cores_per_queue,
                detail::affinity_data const& affinity_data,
                char const* description)
              : num_worker_threads_(num_worker_threads)
              , cores_per_queue_(cores_per_queue)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            std::size_t num_worker_threads_;
            core_ratios cores_per_queue_;
            thread_queue_init_parameters thread_queue_init_;
            detail::affinity_data const& affinity_data_;
            char const* description_;
        };
        using init_parameter_type = init_parameter;

        explicit shared_priority_queue_scheduler(init_parameter const& init)
          : scheduler_base(init.num_worker_threads_, init.description_,
                init.thread_queue_init_)
#if !defined(HPX_HAVE_MAX_CPU_COUNT)
          , d_lookup_(hpx::threads::hardware_concurrency())
          , q_lookup_(hpx::threads::hardware_concurrency())
#ifdef SHARED_PRIORITY_SCHEDULER_LINUX
          , schedcpu_(hpx::threads::hardware_concurrency())
#endif
#endif
          , cores_per_queue_(init.cores_per_queue_)
          , num_workers_(init.num_worker_threads_)
          , num_domains_(1)
          , affinity_data_(init.affinity_data_)
          , queue_parameters_(init.thread_queue_init_)
          , initialized_(false)
          , debug_init_(false)
          , thread_init_counter_(0)
          , pool_index_(static_cast<std::size_t>(-1))
        {
            HPX_ASSERT(num_workers_ != 0);
        }

        shared_priority_queue_scheduler(
            shared_priority_queue_scheduler const&) = delete;
        shared_priority_queue_scheduler(
            shared_priority_queue_scheduler&&) = delete;
        shared_priority_queue_scheduler& operator=(
            shared_priority_queue_scheduler const&) = delete;
        shared_priority_queue_scheduler& operator=(
            shared_priority_queue_scheduler&&) = delete;

        virtual ~shared_priority_queue_scheduler() = default;

        static std::string_view get_scheduler_name()
        {
            return "shared_priority_queue_scheduler";
        }

        // get/set scheduler mode
        void set_scheduler_mode(scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) noexcept override
        {
            // clang-format off
            scheduler_base::set_scheduler_mode(mode, pu_mask);
            round_robin_ = mode & policies::scheduler_mode::assign_work_round_robin;
            steal_hp_first_ =
                mode & policies::scheduler_mode::steal_high_priority_first;
            core_stealing_ = mode & policies::scheduler_mode::enable_stealing;
            numa_stealing_ = mode & policies::scheduler_mode::enable_stealing_numa;
            spq_deb.debug(debug::str<>("scheduler_mode")
                , round_robin_ ? "round_robin" : "thread parent", ','
                , steal_hp_first_ ? "steal_hp_first" : "steal after local", ','
                , core_stealing_ ? "stealing" : "no stealing", ','
                , numa_stealing_ ? "numa stealing" : "no numa stealing");
            // clang-format on
        }

        // ------------------------------------------------------------
        void abort_all_suspended_threads() override
        {
            // process all cores if -1 was sent in
            for (std::size_t d = 0; d < num_domains_; ++d)
            {
                numa_holder_[d].abort_all_suspended_threads();
            }
        }

        // ------------------------------------------------------------
        // access thread local storage to determine correct thread
        // and pool identification. This is used internally by the scheduler
        // to compute correct queue indexes and offsets relative to a numa
        // node. It should not be used without care as the thread numbering
        // internal to the scheduler is not a simple linear indexing
        // returns -1 to indicate an invalid thread/value/state
        inline std::size_t local_thread_number() const
        {
            using namespace hpx::threads::detail;
            std::size_t const thread_pool_num = get_thread_pool_num_tss();
            // if the thread belongs to this pool return local Id
            if (pool_index_ == thread_pool_num)
                return get_local_thread_num_tss();
            return static_cast<std::size_t>(-1);
        }

        // ------------------------------------------------------------
        bool cleanup_terminated(bool delete_all) override
        {
            // just cleanup the thread we were called by rather than all threads
            static auto cleanup = spq_deb.make_timer(
                1, debug::str<>("Cleanup"), "Global version");
            spq_deb.timed(cleanup);

            std::size_t local_num = local_thread_number();
            if (local_num == static_cast<std::size_t>(-1))
            {
                // clang-format off
                        using namespace hpx::threads::detail;
                        spq_deb.debug(debug::str<>("cleanup_terminated")
                            , "v1 aborted"
                            , "num_workers_", num_workers_
                            , "thread_number"
                            , "global", get_global_thread_num_tss()
                            , "local", get_local_thread_num_tss()
                            , "pool", get_thread_pool_num_tss()
                            , "parent offset", parent_pool_->get_thread_offset()
                            , parent_pool_->get_pool_name());
                // clang-format on
                return false;
            }

            std::size_t domain_num = d_lookup_[local_num];
            std::size_t const q_index = q_lookup_[local_num];

            spq_deb.debug(debug::str<>("cleanup_terminated"), "v1", "D",
                debug::dec<2>(domain_num), "Q", debug::dec<3>(q_index),
                "thread_num", debug::dec<3>(local_num));

            return numa_holder_[domain_num]
                .thread_queue(static_cast<std::size_t>(q_index))
                ->cleanup_terminated(local_num, delete_all);
        }

        // ------------------------------------------------------------
        bool cleanup_terminated(
            std::size_t /* thread_num */, bool delete_all) override
        {
            std::size_t local_num = local_thread_number();
            HPX_ASSERT(local_num < num_workers_);

            // find the numa domain from the local thread index
            std::size_t domain_num = d_lookup_[local_num];
            std::size_t const q_index = q_lookup_[local_num];

            spq_deb.debug(debug::str<>("cleanup_terminated"), "v2", "D",
                debug::dec<2>(domain_num), "Q", debug::dec<3>(q_index),
                "thread_num", debug::dec<3>(local_num));

            // cleanup the queues assigned to this thread
            return numa_holder_[domain_num]
                .thread_queue(static_cast<std::size_t>(q_index))
                ->cleanup_terminated(local_num, delete_all);
        }

        // ------------------------------------------------------------
        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(thread_init_data& data, thread_id_ref_type* thrd,
            error_code& ec) override
        {
            // safety check that task was created by this thread/scheduler
            HPX_ASSERT(data.scheduler_base == this);

            // this scheduler assumes thread bindings for the scheduled
            // threads in certain ways preventing direct execution
            HPX_ASSERT(data.schedulehint.runs_as_child_mode() ==
                hpx::threads::thread_execution_hint::none);

            std::size_t const local_num = local_thread_number();

            std::size_t thread_num = local_num;
            std::size_t domain_num;
            std::size_t q_index;

            auto msg = spq_deb.declare_variable<char const*>(nullptr);

            using threads::thread_schedule_hint_mode;
            switch (data.schedulehint.mode)
            {
            case thread_schedule_hint_mode::none:
            {
                spq_deb.set(msg, "HINT_NONE  ");
                // Create thread on this worker thread if possible
                if (local_num == static_cast<std::size_t>(-1))
                {
                    // clang-format off
                    using namespace hpx::threads::detail;
                    spq_deb.debug(debug::str<>("create_thread")
                        , "x-pool", "num_workers_", num_workers_
                        , "thread_number"
                        , "global", get_thread_nums_tss().global_thread_num
                        , "local", get_thread_nums_tss().local_thread_num
                        , "pool", get_thread_nums_tss().thread_pool_num
                        , "parent offset", parent_pool_->get_thread_offset()
                        , parent_pool_->get_pool_name());
                    // clang-format on

                    // This is a task being injected from a thread on another
                    // pool - we can schedule on any thread available
                    thread_num = numa_holder_[0].thread_queue(0)->worker_next(
                        static_cast<std::size_t>(num_workers_));
                }
                else if (!round_robin_) /* thread parent */
                {
                    if constexpr (spq_deb.is_enabled())
                    {
                        domain_num = d_lookup_[thread_num];
                        q_index = q_lookup_[thread_num];

                        spq_deb.debug(debug::str<>("create_thread"),
                            "assign_work_thread_parent", "thread_num",
                            thread_num, "pool", parent_pool_->get_pool_name());
                    }
                }
                else /*(round_robin)*/
                {
                    domain_num = d_lookup_[thread_num];
                    q_index = q_lookup_[thread_num];
                    spq_deb.debug(debug::str<>("create_thread"),
                        "assign_work_round_robin", "thread_num", thread_num,
                        "pool", parent_pool_->get_pool_name(),
                        typename thread_holder_type::queue_data_print(
                            numa_holder_[domain_num].thread_queue(
                                static_cast<std::size_t>(q_index))));
                    thread_num =
                        numa_holder_[domain_num]
                            .thread_queue(static_cast<std::size_t>(q_index))
                            ->worker_next(
                                static_cast<std::size_t>(num_workers_));
                }
                thread_num = select_active_pu(thread_num);
                // cppcheck-suppress redundantAssignment
                domain_num = d_lookup_[thread_num];    //-V519
                // cppcheck-suppress redundantAssignment
                q_index = q_lookup_[thread_num];    //-V519
                break;
            }
            case thread_schedule_hint_mode::thread:
            {
                spq_deb.set(msg, "HINT_THREAD");
                // Create thread on requested worker thread
                thread_num = select_active_pu(
                    fast_mod(static_cast<std::size_t>(data.schedulehint.hint),
                        num_workers_));
                domain_num = d_lookup_[thread_num];
                q_index = q_lookup_[thread_num];
                break;
            }
            case thread_schedule_hint_mode::numa:
            {
                // Create thread on requested NUMA domain
                spq_deb.set(msg, "HINT_NUMA  ");
                domain_num = fast_mod(data.schedulehint.hint, num_domains_);
                // if the thread creating the new task is on the domain
                // assigned to the new task - try to reuse the core as well
                if (local_num != static_cast<std::size_t>(-1) &&
                    d_lookup_[local_num] == domain_num)
                {
                    thread_num = local_num;    //-V1048
                }
                else
                {
                    // first queue on this domain
                    thread_num = q_offset_[domain_num];
                    // offset by some counter, modules num queues on domain
                    thread_num +=
                        numa_holder_[domain_num].thread_queue(0)->worker_next(
                            q_counts_[domain_num]);
                }
                // Ensure the selected PU is active, consistent with the
                // handling of thread and none scheduling hint modes.
                thread_num = select_active_pu(thread_num);
                // cppcheck-suppress redundantAssignment
                domain_num = d_lookup_[thread_num];    //-V519
                // cppcheck-suppress redundantAssignment
                q_index = q_lookup_[thread_num];    //-V519
                break;
            }
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "shared_priority_queue_scheduler::create_thread",
                    "Invalid schedule hint mode: {}",
                    static_cast<std::size_t>(data.schedulehint.mode));
            }

            // we do not allow threads created on other queues to 'run now'
            // as this causes cross-thread allocations and map accesses
            if (local_num != thread_num &&
                (data.initial_state == thread_schedule_state::pending ||
                    data.initial_state == thread_schedule_state::pending_boost))
            {
                data.run_now = false;

                // clang-format off
                spq_deb.debug(debug::str<>("create_thread")
                    , "pool", parent_pool_->get_pool_name()
                    , "hint", msg
                    , "dest"
                    , "D", debug::dec<2>(domain_num)
                    , "Q", debug::dec<3>(q_index)
                    , "this"
                    , "D", debug::dec<2>(d_lookup_[thread_num])
                    , "Q", debug::dec<3>(thread_num)
                    , "run_now OVERRIDE ", data.run_now
                    , debug::threadinfo<thread_init_data>(data));
                // clang-format on
            }
            else
            {
                // clang-format off
                spq_deb.debug(debug::str<>("create_thread")
                    , "pool", parent_pool_->get_pool_name()
                    , "hint", msg
                    , "dest"
                    , "D", debug::dec<2>(domain_num)
                    , "Q", debug::dec<3>(q_index)
                    , "this"
                    , "D", debug::dec<2>(d_lookup_[thread_num])
                    , "Q", debug::dec<3>(thread_num)
                    , "run_now", data.run_now
                    , debug::threadinfo<thread_init_data>(data));
                // clang-format on
            }
            numa_holder_[domain_num]
                .thread_queue(static_cast<std::size_t>(q_index))
                ->create_thread(data, thrd, local_num, ec);
        }

        template <typename T>
        bool steal_by_function(std::size_t domain, std::size_t q_index,
            bool steal_numa, bool steal_core, thread_holder_type* origin,
            T& var, char const* prefix,
            hpx::function<bool(
                std::size_t, std::size_t, thread_holder_type*, T&, bool, bool)>
                operation_HP,
            hpx::function<bool(
                std::size_t, std::size_t, thread_holder_type*, T&, bool, bool)>
                operation)
        {
            bool result;

            // All stealing disabled
            if (!steal_core)
            {
                // try only the queues on this thread, in order BP,HP,NP,LP
                result =
                    operation_HP(domain, q_index, origin, var, false, false);
                result = result ||
                    operation(domain, q_index, origin, var, false, false);
                if (result)
                {
                    spq_deb.debug(debug::str<>(prefix), "local no stealing",
                        "D", debug::dec<2>(domain), "Q",
                        debug::dec<3>(q_index));
                    return result;
                }
            }
            // High priority tasks first
            else if (steal_hp_first_)
            {
                for (std::size_t d = 0; d < num_domains_; ++d)
                {
                    std::size_t dom = fast_mod(domain + d, num_domains_);
                    q_index = fast_mod(q_index, q_counts_[dom]);
                    result =
                        operation_HP(dom, q_index, origin, var, (d > 0), true);
                    if (result)
                    {
                        spq_deb.debug(debug::str<>(prefix),
                            "steal_high_priority_first BP/HP",
                            (d == 0 ? "taken" : "stolen"), "D",
                            debug::dec<2>(domain), "Q", debug::dec<3>(q_index));
                        return result;
                    }
                    // if no numa stealing, skip other domains
                    if (!steal_numa)
                        break;
                }
                for (std::size_t d = 0; d < num_domains_; ++d)
                {
                    std::size_t dom = fast_mod(domain + d, num_domains_);
                    q_index = fast_mod(q_index, q_counts_[dom]);
                    result =
                        operation(dom, q_index, origin, var, (d > 0), true);
                    if (result)
                    {
                        spq_deb.debug(debug::str<>(prefix),
                            "steal_high_priority_first NP/LP",
                            (d == 0 ? "taken" : "stolen"), "D",
                            debug::dec<2>(domain), "Q", debug::dec<3>(q_index));
                        return result;
                    }
                    // if no numa stealing, skip other domains
                    if (!steal_numa)
                        break;
                }
            }
            else /*steal_after_local*/
            {
                // do this local core/queue
                result =
                    operation_HP(domain, q_index, origin, var, false, false);
                result = result ||
                    operation(domain, q_index, origin, var, false, false);
                if (result)
                {
                    spq_deb.debug(debug::str<>(prefix),
                        "steal_after_local local taken", "D",
                        debug::dec<2>(domain), "Q", debug::dec<3>(q_index));
                    return result;
                }

                if (q_counts_[domain] > 1)
                {
                    // steal from other cores on this numa domain?
                    // use q+1 to avoid testing the same local queue again
                    q_index = fast_mod((q_index + 1), q_counts_[domain]);
                    result =
                        operation_HP(domain, q_index, origin, var, true, true);
                    result = result ||
                        operation(domain, q_index, origin, var, true, true);
                    if (result)
                    {
                        spq_deb.debug(debug::str<>(prefix),
                            "steal_after_local this numa", "stolen", "D",
                            debug::dec<2>(domain), "Q", debug::dec<3>(q_index));
                        return result;
                    }
                }

                if (steal_numa)
                {
                    // try other numa domains BP/HP
                    for (std::size_t d = 1; d < num_domains_; ++d)
                    {
                        std::size_t dom = fast_mod(domain + d, num_domains_);
                        q_index = fast_mod(q_index, q_counts_[dom]);
                        result =
                            operation_HP(dom, q_index, origin, var, true, true);
                        if (result)
                        {
                            spq_deb.debug(debug::str<>(prefix),
                                "steal_after_local other numa BP/HP", "stolen",
                                "D", debug::dec<2>(domain), "Q",
                                debug::dec<3>(q_index));
                            return result;
                        }
                    }
                    // try other numa domains NP/LP
                    for (std::size_t d = 1; d < num_domains_; ++d)
                    {
                        std::size_t dom = fast_mod(domain + d, num_domains_);
                        q_index = fast_mod(q_index, q_counts_[dom]);
                        result =
                            operation(dom, q_index, origin, var, true, true);
                        if (result)
                        {
                            spq_deb.debug(debug::str<>(prefix),
                                "steal_after_local other numa NP/LP", "stolen",
                                "D", debug::dec<2>(domain), "Q",
                                debug::dec<3>(q_index));
                            return result;
                        }
                    }
                }
            }
            return false;
        }

        // Return the next thread to be executed, return false if none available
        bool get_next_thread(std::size_t thread_num, bool running,
            threads::thread_id_ref_type& thrd, bool enable_stealing)
        {
            std::size_t const this_thread = local_thread_number();
            HPX_ASSERT(this_thread < num_workers_);

            // just cleanup the thread we were called by rather than all threads
            static auto getnext =
                spq_deb.make_timer(1, debug::str<>("get_next_thread"));

            spq_deb.timed(getnext, debug::dec<>(thread_num));

            auto get_next_thread_function_HP =
                [&](std::size_t domain, std::size_t q_index,
                    thread_holder_type* /* receiver */,
                    threads::thread_id_ref_type& th, bool stealing,
                    bool allow_stealing) {
                    return numa_holder_[domain].get_next_thread_HP(
                        q_index, th, stealing, allow_stealing);
                };

            auto get_next_thread_function =
                [&](std::size_t domain, std::size_t q_index,
                    thread_holder_type* /* receiver */,
                    threads::thread_id_ref_type& th, bool stealing,
                    bool allow_stealing) {
                    return numa_holder_[domain].get_next_thread(
                        q_index, th, stealing, allow_stealing);
                };

            std::size_t domain = d_lookup_[this_thread];
            std::size_t q_index = q_lookup_[this_thread];

            // first try a high priority task, allow stealing if stealing of HP
            // tasks in on, this will be fine but send a null function for
            // normal tasks

            if (bool const result =
                    steal_by_function<threads::thread_id_ref_type>(domain,
                        q_index, numa_stealing_, core_stealing_, nullptr, thrd,
                        "SBF-get_next_thread", get_next_thread_function_HP,
                        get_next_thread_function))
            {
                return result;
            }

            // if we did not get a task at all, then try converting tasks in the
            // pending queue into staged ones
            std::size_t added = 0;
            std::int64_t idle_loop_count = 0;
            wait_or_add_new(thread_num, true, idle_loop_count, true, added);
            if (added > 0)
            {
                return get_next_thread(
                    this_thread, running, thrd, enable_stealing);
            }
            return false;
        }

        // Return the next thread to be executed, return false if none available
        bool wait_or_add_new(std::size_t /* thread_num */, bool /* running */,
            std::int64_t& /* idle_loop_count */, bool /*enable_stealing*/,
            std::size_t& added, thread_id_ref_type* = nullptr)
        {
            std::size_t const this_thread = local_thread_number();
            HPX_ASSERT(this_thread < num_workers_);

            // just cleanup the thread we were called by rather than all threads
            static auto w_or_add_n =
                spq_deb.make_timer(1, debug::str<>("wait_or_add_new"));

            added = 0;

            auto add_new_function_HP =
                [&](std::size_t domain, std::size_t q_index,
                    thread_holder_type* receiver, std::size_t& add,
                    bool stealing, bool allow_stealing) {
                    return numa_holder_[domain].add_new_HP(
                        receiver, q_index, add, stealing, allow_stealing);
                };

            auto add_new_function = [&](std::size_t domain, std::size_t q_index,
                                        thread_holder_type* receiver,
                                        std::size_t& add, bool stealing,
                                        bool allow_stealing) {
                return numa_holder_[domain].add_new(
                    receiver, q_index, add, stealing, allow_stealing);
            };

            std::size_t domain = d_lookup_[this_thread];
            std::size_t q_index = q_lookup_[this_thread];

            thread_holder_type* receiver =
                numa_holder_[domain].queues_[q_index];
            spq_deb.timed(w_or_add_n, "thread_num", this_thread, "q_index",
                q_index, "numa_stealing ", numa_stealing_, "core_stealing ",
                core_stealing_);

            bool const added_tasks = steal_by_function<std::size_t>(domain,
                q_index, numa_stealing_, core_stealing_, receiver, added,
                "wait_or_add_new", add_new_function_HP, add_new_function);

            return !added_tasks;
        }

        // Schedule the passed thread
        void schedule_thread(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint, bool allow_fallback,
            thread_priority priority = thread_priority::default_) override
        {
            HPX_ASSERT(get_thread_id_data(thrd)->get_scheduler_base() == this);

            std::size_t local_num = local_thread_number();
            std::size_t thread_num = local_num;
            std::size_t domain_num = 0;
            auto q_index = static_cast<std::size_t>(-1);

            auto msg = spq_deb.declare_variable<char const*>(nullptr);

            std::unique_lock<pu_mutex_type> l;

            using threads::thread_schedule_hint_mode;

            switch (schedulehint.mode)
            {
            case thread_schedule_hint_mode::none:
            {
                // Create thread on this worker thread if possible
                spq_deb.set(msg, "HINT_NONE  ");
                if (local_num == static_cast<std::size_t>(-1))
                {
                    // This is a task being injected from a thread on another
                    // pool - we can schedule on any thread available
                    thread_num = numa_holder_[0].thread_queue(0)->worker_next(
                        num_workers_);
                    q_index = 0;
                    // clang-format off
                    using namespace hpx::threads::detail;
                    spq_deb.debug(debug::str<>("schedule_thread")
                        , "x-pool thread schedule"
                        , "num_workers_", num_workers_
                        , "thread_number"
                        , "global", get_thread_nums_tss().global_thread_num
                        , "local", get_thread_nums_tss().local_thread_num
                        , "pool", get_thread_nums_tss().thread_pool_num
                        , "parent offset", parent_pool_->get_thread_offset()
                        , parent_pool_->get_pool_name(),
                        debug::threadinfo<threads::thread_id_ref_type*>(&thrd));
                    // clang-format on
                }
                else if (!round_robin_) /*assign_parent*/
                {
                    domain_num = d_lookup_[thread_num];
                    q_index = q_lookup_[thread_num];
                    spq_deb.debug(debug::str<>("schedule_thread"),
                        "assign_work_thread_parent", "thread_num", thread_num,
                        debug::threadinfo<threads::thread_id_ref_type*>(&thrd));
                }
                else /*(round_robin_)*/
                {
                    domain_num = d_lookup_[thread_num];
                    q_index = q_lookup_[thread_num];
                    thread_num = numa_holder_[domain_num]
                                     .thread_queue(q_index)
                                     ->worker_next(num_workers_);
                    spq_deb.debug(debug::str<>("schedule_thread"),
                        "assign_work_round_robin", "thread_num", thread_num,
                        debug::threadinfo<threads::thread_id_ref_type*>(&thrd));
                }
                thread_num = select_active_pu(thread_num, allow_fallback);
                // cppcheck-suppress redundantAssignment
                domain_num = d_lookup_[thread_num];    //-V519
                // cppcheck-suppress redundantAssignment
                q_index = q_lookup_[thread_num];    //-V519
                break;
            }
            case thread_schedule_hint_mode::thread:
            {
                spq_deb.set(msg, "HINT_THREAD");
                spq_deb.debug(debug::str<>("schedule_thread"),
                    "received HINT_THREAD", debug::dec<3>(schedulehint.hint));
                thread_num = select_active_pu(
                    fast_mod(static_cast<std::size_t>(schedulehint.hint),
                        num_workers_),
                    allow_fallback);
                domain_num = d_lookup_[thread_num];
                q_index = q_lookup_[thread_num];
                break;
            }
            case thread_schedule_hint_mode::numa:
            {
                // Schedule thread on requested NUMA domain
                spq_deb.set(msg, "HINT_NUMA  ");
                domain_num = fast_mod(schedulehint.hint, num_domains_);
                // if the current thread is on the requested domain, reuse it;
                // otherwise fall back to the first queue on that domain
                if (d_lookup_[thread_num] != domain_num)
                {
                    thread_num = q_offset_[domain_num];
                    thread_num +=
                        numa_holder_[domain_num].thread_queue(0)->worker_next(
                            q_counts_[domain_num]);
                }
                // Ensure the selected PU is active, consistent with the
                // handling of thread and none scheduling hint modes.
                thread_num = select_active_pu(thread_num, allow_fallback);
                // cppcheck-suppress redundantAssignment
                domain_num = d_lookup_[thread_num];    //-V519
                // cppcheck-suppress redundantAssignment
                q_index = q_lookup_[thread_num];    //-V519
                break;
            }

            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "shared_priority_queue_scheduler::schedule_thread",
                    "Invalid schedule hint mode: {}",
                    static_cast<std::size_t>(schedulehint.mode));
            }

            spq_deb.debug(debug::str<>("thread scheduled"), msg, "Thread",
                debug::dec<3>(thread_num), "D", debug::dec<2>(domain_num), "Q",
                debug::dec<3>(q_index));

            if (priority == thread_priority::default_)
            {
                priority = get_thread_id_data(thrd)->get_priority();
            }

            numa_holder_[domain_num].thread_queue(q_index)->schedule_thread(
                thrd, priority, false);
        }

        // Put task on the back of the queue : not yet implemented just put it
        // on the normal queue for now
        void schedule_thread_last(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint, bool allow_fallback,
            thread_priority priority = thread_priority::default_) override
        {
            spq_deb.debug(debug::str<>("schedule_thread_last"));
            schedule_thread(thrd, schedulehint, allow_fallback, priority);
        }

        //---------------------------------------------------------------------
        // Destroy the passed thread - as it has been terminated
        //---------------------------------------------------------------------
        void destroy_thread(threads::thread_data* thrd) override
        {
            HPX_ASSERT(thrd->get_scheduler_base() == this);
            auto d1 = thrd->get_queue<queue_holder_thread<thread_queue_type>>()
                          .domain_index_;
            auto q1 = thrd->get_queue<queue_holder_thread<thread_queue_type>>()
                          .queue_index_;

            std::size_t this_thread = local_thread_number();
            HPX_ASSERT(this_thread < num_workers_);

            auto d2 = d_lookup_[this_thread];
            auto q2 = q_lookup_[this_thread];
            bool xthread = ((q1 != q2) || (d1 != d2));
            spq_deb.debug(debug::str<>("destroy_thread"), "xthread", xthread,
                "task owned by", "D", debug::dec<2>(d1), "Q", debug::dec<3>(q1),
                "this thread", "D", debug::dec<2>(d2), "Q", debug::dec<3>(q2),
                debug::threadinfo<threads::thread_data*>(thrd));

            // the cleanup of a task should be done by the original owner of the
            // task, so return it to the queue it came from before it was stolen
            thrd->get_queue<queue_holder_thread<thread_queue_type>>()
                .destroy_thread(thrd, this_thread, xthread);
        }

        //---------------------------------------------------------------------
        // This returns the current length of the queues
        // (work items and new items)
        //---------------------------------------------------------------------
        std::int64_t get_queue_length(std::size_t thread_num) const override
        {
            spq_deb.debug(debug::str<>("get_queue_length"), "thread_num ",
                debug::dec<>(thread_num));

            HPX_ASSERT(thread_num != static_cast<std::size_t>(-1));

            std::int64_t count = 0;
            if (thread_num != static_cast<std::size_t>(-1))
            {
                std::size_t domain_num = d_lookup_[thread_num];
                std::size_t q_index = q_lookup_[thread_num];
                count += numa_holder_[domain_num]
                             .thread_queue(q_index)
                             ->get_queue_length();
            }
            else
            {
                throw std::runtime_error("unhandled get_queue_length with -1");
            }
            return count;
        }

        //---------------------------------------------------------------------
        // Queries the current thread count of the queues.
        //---------------------------------------------------------------------
        std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown,
            thread_priority priority = thread_priority::default_,
            std::size_t thread_num = static_cast<std::size_t>(-1),
            bool /* reset */ = false) const override
        {
            spq_deb.debug(debug::str<>("get_thread_count"), "thread_num ",
                debug::dec<3>(thread_num));

            if (thread_num != static_cast<std::size_t>(-1))
            {
                std::size_t domain_num = d_lookup_[thread_num];
                std::size_t q_index = q_lookup_[thread_num];
                return numa_holder_[domain_num]
                    .thread_queue(q_index)
                    ->get_thread_count(state, priority);
            }
            else
            {
                std::int64_t count = 0;
                for (std::size_t d = 0; d < num_domains_; ++d)
                {
                    count += numa_holder_[d].get_thread_count(state, priority);
                }
                spq_deb.debug(debug::str<>("get_thread_count"), "thread_num ",
                    debug::dec<3>(thread_num), "count ", debug::dec<4>(count));
                return count;
            }
        }

        // Queries whether a given core is idle
        bool is_core_idle(std::size_t num_thread) const override
        {
            std::size_t domain_num = d_lookup_[num_thread];
            std::size_t q_index = q_lookup_[num_thread];
            return numa_holder_[domain_num]
                       .thread_queue(q_index)
                       ->get_queue_length() == 0;
        }

        //---------------------------------------------------------------------
        // Enumerate matching threads from all queues
        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state =
                thread_schedule_state::unknown) const override
        {
            bool result = true;

            spq_deb.debug(debug::str<>("enumerate_threads"));

            for (std::size_t d = 0; d < num_domains_; ++d)
            {
                result = numa_holder_[d].enumerate_threads(f, state) && result;
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////
        void on_start_thread(std::size_t local_thread) override
        {
            spq_deb.debug(
                debug::str<>("start_thread"), "local_thread", local_thread);

            auto const& topo = create_topology();
            // the main initialization can be done by any one thread
            std::unique_lock<Mutex> lock(init_mutex);
            if (!initialized_)
            {
                // make sure no other threads enter when mutex is unlocked
                initialized_ = true;

                // used to make sure thread ids are valid for this scheduler
                pool_index_ =
                    static_cast<std::size_t>(parent_pool_->get_pool_index());

                // For each worker thread, count which numa domain each
                // belongs to and build lists of useful indexes/refs
                num_domains_ = 1;
                std::fill(d_lookup_.begin(), d_lookup_.end(), 0);
                std::fill(q_lookup_.begin(), q_lookup_.end(), 0);
                std::fill(q_offset_.begin(), q_offset_.end(), 0);
                std::fill(q_counts_.begin(), q_counts_.end(), 0);

                std::map<std::size_t, std::size_t> domain_map;
                for (std::size_t local_id = 0; local_id != num_workers_;
                    ++local_id)
                {
                    std::size_t const global_id =
                        local_to_global_thread_index(local_id);
                    std::size_t const pu_num =
                        affinity_data_.get_pu_num(global_id);
                    std::size_t domain = topo.get_numa_node_number(pu_num);
#if defined(SHARED_PRIORITY_SCHEDULER_DEBUG_NUMA)
                    if (local_id >= (num_workers_ + 1) / 2)
                    {
                        domain += 1;
                    }
#endif
                    d_lookup_[local_id] = domain;

                    // each time a _new_ domain is added increment the offset
                    domain_map.insert({domain, domain_map.size()});
                }
                num_domains_ = domain_map.size();

                // if we have zero threads on a numa domain, reindex the domains
                // to be sequential otherwise it messes up counting as an
                // indexing operation. This can happen on nodes that have
                // unusual numa topologies with (e.g.) High Bandwidth Memory on
                // numa nodes with no processors
                for (std::size_t local_id = 0; local_id < num_workers_;
                    ++local_id)
                {
                    d_lookup_[local_id] = static_cast<std::size_t>(
                        domain_map[d_lookup_[local_id]]);

                    // increment the count for the domain
                    q_counts_[d_lookup_[local_id]]++;
                }

                // compute queue offsets for each domain
                std::partial_sum(
                    &q_counts_[0], &q_counts_[num_domains_ - 1], &q_offset_[1]);
            }

            // all threads should now complete their initialization by creating
            // the queues that are local to their own thread
            lock.unlock();

            // ...for each domain, count up the pus assigned, by core (when
            // enable_hyperthread_core_sharing is on, pus on the same core will
            // always share queues) NB. we must do this _after_ remapping
            // domains (above)

            // tuple of : domain, core, pu, local_id
            using dcp_tuple =
                std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>;
            std::vector<dcp_tuple> locations;
            for (std::size_t local_id = 0; local_id != num_workers_; ++local_id)
            {
                std::size_t const global_id =
                    local_to_global_thread_index(local_id);
                std::size_t pu_num = affinity_data_.get_pu_num(global_id);
                std::size_t core = topo.get_core_number(pu_num);
                std::size_t domain = d_lookup_[local_id];
                //
                locations.emplace_back(domain, core, pu_num, local_id);
            }

            // sort by 1) domain, 2) core, 3) pu so that we can iterate over
            // worker threads and assign them into groups based on locality even
            // if th thread numbering is arbitrary
            std::sort(locations.begin(), locations.end(),
                [](dcp_tuple const& a, dcp_tuple const& b) -> bool {
                    return (std::get<0>(a) == std::get<0>(b)) ?
                        ((std::get<1>(a) == std::get<1>(b)) ?
                                (std::get<2>(a) < std::get<2>(b)) :
                                (std::get<1>(a) < std::get<1>(b))) :
                        (std::get<0>(a) < std::get<0>(b));
                });

            // if cores_per_queue > 1 then some queues are shared between
            // threads, one thread will be the 'owner' of the queue. allow one
            // thread at a time to enter this section in incrementing thread
            // number ordering so that we can init queues and assign them
            // guaranteeing that references to threads with lower ids are valid.
            while (local_thread != std::get<3>(locations[thread_init_counter_]))
            {
                // std::thread because we cannot suspend HPX threads here
                std::this_thread::yield();
            }

            // store local thread number and pool index in thread local storage,
            // the global number has already been set
            hpx::threads::detail::set_local_thread_num_tss(local_thread);
            hpx::threads::detail::set_thread_pool_num_tss(
                parent_pool_->get_pool_id().index());

            // one thread holder per core (shared by PUs)

            // queue pointers we will assign to each thread
            thread_queue_type* bp_queue = nullptr;
            thread_queue_type* hp_queue = nullptr;
            thread_queue_type* np_queue = nullptr;
            thread_queue_type* lp_queue = nullptr;

            // for each worker thread, assign queues
            auto previous_domain = static_cast<std::size_t>(-1);
            std::size_t index = 0;

            for (auto& tup : locations)
            {
                std::size_t domain = std::get<0>(tup);
                std::size_t local_id = std::get<3>(tup);
                std::size_t numa_id = local_id - q_offset_[domain];

                // on each new numa domain, restart queue indexing
                if (previous_domain != domain)
                {
                    index = 0;
                    previous_domain = domain;
                    // initialize numa domain holder
                    if (numa_holder_[domain].size() == 0)
                    {
                        numa_holder_[domain].init(
                            static_cast<std::size_t>(domain),
                            q_counts_[domain]);
                    }
                }

                if (local_thread == local_id)
                {
                    std::int16_t owner_mask = 0;
                    q_lookup_[local_thread] = static_cast<std::size_t>(index);

                    // bound queues are never shared
                    bp_queue = new thread_queue_type(queue_parameters_, index);
                    owner_mask |= 1;
                    //
                    if (cores_per_queue_.high_priority > 0)
                    {
                        if ((index % cores_per_queue_.high_priority) == 0)
                        {
                            // if we will own the queue, create it
                            hp_queue =
                                new thread_queue_type(queue_parameters_, index);
                            owner_mask |= 2;
                        }
                        else
                        {
                            // share the queue with our next lowest neighbor
                            HPX_ASSERT(index != 0);
                            hp_queue =
                                numa_holder_[domain]
                                    .thread_queue(
                                        static_cast<std::size_t>(index - 1))
                                    ->hp_queue_;
                        }
                    }
                    // Normal priority
                    if ((index % cores_per_queue_.normal_priority) == 0)
                    {
                        // if we will own the queue, create it
                        np_queue =
                            new thread_queue_type(queue_parameters_, index);
                        owner_mask |= 4;    //-V112
                    }
                    else
                    {
                        // share the queue with our next lowest neighbor
                        HPX_ASSERT(index != 0);
                        np_queue = numa_holder_[domain]
                                       .thread_queue(
                                           static_cast<std::size_t>(index - 1))
                                       ->np_queue_;
                    }
                    // Low priority
                    if (cores_per_queue_.low_priority > 0)
                    {
                        if ((index % cores_per_queue_.low_priority) == 0)
                        {
                            // if we will own the queue, create it
                            lp_queue =
                                new thread_queue_type(queue_parameters_, index);
                            owner_mask |= 8;
                        }
                        else
                        {
                            // share the queue with our next lowest neighbor
                            HPX_ASSERT(index != 0);
                            lp_queue =
                                numa_holder_[domain]
                                    .thread_queue(
                                        static_cast<std::size_t>(index - 1))
                                    ->lp_queue_;
                        }
                    }

                    spq_deb.debug(debug::str<>("thread holder"), "local_thread",
                        local_thread, "domain", domain, "index", index,
                        "local_id", local_id, "owner_mask", owner_mask);

                    thread_holder_type* thread_holder =
                        new queue_holder_thread<thread_queue_type>(bp_queue,
                            hp_queue, np_queue, lp_queue,
                            static_cast<std::size_t>(domain),
                            static_cast<std::size_t>(index),
                            static_cast<std::size_t>(local_id),
                            static_cast<std::size_t>(owner_mask),
                            queue_parameters_);

                    numa_holder_[domain].queues_[numa_id] = thread_holder;
                }

#ifdef SHARED_PRIORITY_SCHEDULER_LINUX
                // for debugging
                schedcpu_[local_thread] = sched_getcpu();
#endif
                // increment thread index counter
                index++;
            }

            // increment the thread counter and allow the next thread to init
            ++thread_init_counter_;

            // we do not want to allow threads to start stealing from others
            // until all threads have initialized their structures. We therefore
            // block at this point until all threads are here.
            while (thread_init_counter_ < num_workers_)
            {
                // std::thread because we cannot suspend HPX threads yet
                std::this_thread::yield();
            }

            lock.lock();
            if (!debug_init_)
            {
                debug_init_ = true;
                spq_arr.array("# d_lookup_  ", d_lookup_.data(), num_workers_);
                spq_arr.array("# q_lookup_  ", q_lookup_.data(), num_workers_);
                spq_arr.array("# q_counts_  ", q_counts_.data(), num_domains_);
                spq_arr.array("# q_offset_  ", q_offset_.data(), num_domains_);
#ifdef SHARED_PRIORITY_SCHEDULER_LINUX
                spq_arr.array("# schedcpu_  ", &schedcpu_[0], num_workers_);
#endif
                for (std::size_t d = 0; d < num_domains_; ++d)
                {
                    numa_holder_[d].debug_info();
                }
            }
        }

        void on_stop_thread(std::size_t thread_num) override
        {
            if (thread_num > num_workers_)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "shared_priority_queue_scheduler::on_stop_thread",
                    "Invalid thread number: {}", thread_num);
            }
            // @TODO Do we need to do any queue related cleanup here?
        }

        void on_error(
            std::size_t thread_num, std::exception_ptr const& /* e */) override
        {
            if (thread_num > num_workers_)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "shared_priority_queue_scheduler::on_error",
                    "Invalid thread number: {}", thread_num);
            }
            // @TODO Do we need to do any queue related cleanup here?
        }

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_creation_time",
                "the shared_priority_scheduler does not support the "
                "get_creation_time performance counter");
            return 0;
        }
        std::uint64_t get_cleanup_time(bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_cleanup_time",
                "the shared_priority_scheduler does not support the "
                "get_cleanup_time performance counter");
            return 0;
        }
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(
            std::size_t /* num */, bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_num_pending_misses",
                "the shared_priority_scheduler does not support the "
                "get_num_pending_misses performance counter");
            return 0;
        }

        std::int64_t get_num_pending_accesses(
            std::size_t /* num */, bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_num_pending_accesses",
                "the shared_priority_scheduler does not support the "
                "get_num_pending_accesses performance counter");
            return 0;
        }

        std::int64_t get_num_stolen_from_pending(
            std::size_t /* num */, bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_num_stolen_from_pending",
                "the shared_priority_scheduler does not support the "
                "get_num_stolen_from_pending performance counter");
            return 0;
        }

        std::int64_t get_num_stolen_to_pending(
            std::size_t /* num */, bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_creation_time",
                "the shared_priority_scheduler does not support the "
                "get_creation_time performance counter");
            return 0;
        }

        std::int64_t get_num_stolen_from_staged(
            std::size_t /* num */, bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_num_stolen_from_staged",
                "the shared_priority_scheduler does not support the "
                "get_num_stolen_from_staged performance counter");
            return 0;
        }

        std::int64_t get_num_stolen_to_staged(
            std::size_t /* num */, bool /* reset */) override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_num_stolen_to_staged",
                "the shared_priority_scheduler does not support the "
                "get_num_stolen_to_staged performance counter");
            return 0;
        }
#endif

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        std::int64_t get_average_thread_wait_time(
            std::size_t /* num_thread */ = std::size_t(-1)) const override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_average_thread_wait_time",
                "the shared_priority_scheduler does not support the "
                "get_average_thread_wait_time performance counter");
            return 0;
        }
        std::int64_t get_average_task_wait_time(
            std::size_t /* num_thread */ = std::size_t(-1)) const override
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "shared_priority_scheduler::get_average_task_wait_time",
                "the shared_priority_scheduler does not support the "
                "get_average_task_wait_time performance counter");
            return 0;
        }
#endif
        // this scheduler assumes thread bindings for the scheduled threads in
        // certain ways preventing direct execution
        bool supports_direct_execution() const noexcept override
        {
            return false;
        }

    protected:
        typedef queue_holder_numa<thread_queue_type> numa_queues;

        // for each numa domain, the number of queues available
        std::array<std::size_t, HPX_HAVE_MAX_NUMA_DOMAIN_COUNT> q_counts_ = {};
        // index of first queue on each numa domain
        std::array<std::size_t, HPX_HAVE_MAX_NUMA_DOMAIN_COUNT> q_offset_ = {};
        // one item per numa domain of a container for queues on that domain
        std::array<numa_queues, HPX_HAVE_MAX_NUMA_DOMAIN_COUNT> numa_holder_ =
            {};

        // lookups for local thread_num into arrays
#if !defined(HPX_HAVE_MAX_CPU_COUNT)
        std::vector<std::size_t> d_lookup_;    // numa domain
        std::vector<std::size_t> q_lookup_;    // queue on domain
#ifdef SHARED_PRIORITY_SCHEDULER_LINUX
        std::vector<std::size_t> schedcpu_;    // cpu_id
#endif
#else
        std::array<std::size_t, HPX_HAVE_MAX_CPU_COUNT>
            d_lookup_;    // numa domain
        std::array<std::size_t, HPX_HAVE_MAX_CPU_COUNT>
            q_lookup_;    // queue on domain
#ifdef SHARED_PRIORITY_SCHEDULER_LINUX
        std::array<std::size_t, HPX_HAVE_MAX_CPU_COUNT> schedcpu_;    // cpu_id
#endif
#endif

        // number of cores per queue for HP, NP, LP queues
        core_ratios cores_per_queue_;

        // when true, new tasks are added round robing to thread queues
        bool round_robin_ = true;

        // when true, tasks are
        bool steal_hp_first_ = true;

        // when true, numa_stealing permits stealing across numa domains,
        // when false, no stealing takes place across numa domains,
        bool numa_stealing_ = true;

        // when true, core_stealing permits stealing between cores(queues),
        // when false, no stealing takes place between any cores(queues)
        bool core_stealing_ = true;

        // number of worker threads assigned to this pool
        std::size_t num_workers_;

        // number of numa domains that the threads are occupying
        std::size_t num_domains_;

        detail::affinity_data const& affinity_data_;

        thread_queue_init_parameters const queue_parameters_;

        // used to make sure the scheduler is only initialized once on a thread
        std::mutex init_mutex;
        bool initialized_;
        // a flag to ensure startup debug info is only printed on one thread
        bool debug_init_;
        std::atomic<std::size_t> thread_init_counter_;
        // used in thread pool checks
        std::size_t pool_index_;
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
