//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file fork_join_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/hardware.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental {

    /// \brief An executor with fork-join (blocking) semantics.
    ///
    /// The fork_join_executor creates on construction a set of worker threads
    /// that are kept alive for the duration of the executor. Copying the
    /// executor has reference semantics, i.e. copies of a fork_join_executor
    /// hold a reference to the worker threads of the original instance.
    /// Scheduling work through the executor concurrently from different threads
    /// is undefined behaviour.
    ///
    /// The executor keeps a set of worker threads alive for the lifetime of the
    /// executor, meaning other work will not be executed while the executor is
    /// busy or waiting for work. The executor has a customizable delay after
    /// which it will yield to other work.  Since starting and resuming the
    /// worker threads is a slow operation the executor should be reused
    /// whenever possible for multiple adjacent parallel algorithms or
    /// invocations of bulk_(a)sync_execute.
    HPX_CXX_CORE_EXPORT class fork_join_executor
    {
    public:
        /// Type of loop schedule for use with the fork_join_executor.
        /// loop_schedule::static_ implies no work-stealing;
        /// loop_schedule::dynamic allows stealing when a worker has finished
        /// its local work.
        enum class loop_schedule : std::uint8_t
        {
            static_,
            dynamic,
        };

        /// \cond NOINTERNAL
        using execution_category = hpx::execution::parallel_execution_tag;
        using executor_parameters_type =
            hpx::execution::experimental::default_parameters;
        /// \endcond

    private:
        /// This struct implements the actual functionality of the executor.
        /// This is separated to allow for reference semantics of the executor.
        struct shared_data
        {
            // Type definitions.
            enum class thread_state : std::uint8_t
            {
                starting = 0,
                idle = 1,
                partitioning_work = 2,
                active = 3,
                stopping = 4,
                stopped = 5,
            };

            using queue_type =
                hpx::concurrency::detail::contiguous_index_queue<std::uint32_t>;
            using queues_type =
                std::vector<hpx::util::cache_aligned_data<queue_type>>;

            struct region_data_type;
            using thread_function_helper_type = void(region_data_type&,
                std::size_t, std::size_t, queues_type&, hpx::spinlock&,
                std::exception_ptr&) noexcept;

            // Members that change for each parallel region.
            struct region_data
            {
                // the thread state for each of the executed threads
                std::atomic<thread_state> state_;

                // The helper function that does the actual work for a single
                // parallel region.
                thread_function_helper_type* thread_function_helper_;

                // Pointers to inputs to bulk_sync_execute.
                void* element_function_;
                void const* shape_;
                void* argument_pack_;
                void* results_;
                hpx::latch* sync_with_main_thread_;
            };

            // Can't apply 'using' here as the type needs to be forward
            // declared
            struct region_data_type
              : std::vector<hpx::util::cache_aligned_data<region_data>>
            {
                using base_type =
                    std::vector<hpx::util::cache_aligned_data<region_data>>;
                using base_type::base_type;
            };

            // Members that are used for all parallel regions executed through
            // this executor.
            threads::thread_pool_base* pool_ = nullptr;
            threads::thread_priority priority_ =
                threads::thread_priority::default_;
            threads::thread_stacksize stacksize_ =
                threads::thread_stacksize::small_;
            loop_schedule schedule_ = loop_schedule::static_;
            std::uint64_t yield_delay_;

            std::size_t main_thread_;
            std::size_t num_threads_;
            hpx::threads::mask_type pu_mask_;
            hpx::spinlock exception_mutex_;
            std::exception_ptr exception_;

            threads::thread_priority main_priority_ =
                threads::thread_priority::default_;

            // Data for each parallel region.
            region_data_type region_data_;

            // The current queues for each worker HPX thread.
            queues_type queues_;

            // executor properties
            char const* annotation_ = nullptr;

            template <typename Op>
            static thread_state wait_state_this_thread_while(
                std::atomic<thread_state> const& tstate, thread_state state,
                std::uint64_t const yield_delay, Op&& op, bool allow_yielding)
            {
                auto const context = hpx::execution_base::this_thread::agent();

                auto current = tstate.load(std::memory_order_acquire);
                if (HPX_UNLIKELY(op(current, state)))
                {
                    HPX_SMT_PAUSE;

                    std::uint64_t base_time = util::hardware::timestamp();
                    current = tstate.load(std::memory_order_acquire);
                    while (HPX_LIKELY(op(current, state)))
                    {
                        bool continue_outer = false;
                        for (int i = 0; i < 256; ++i)
                        {
                            HPX_SMT_PAUSE;

                            // Use atomic acquire only after atomic relaxed
                            // suggests that we should stop iterating.
                            if (HPX_UNLIKELY(
                                    !op(tstate.load(std::memory_order_relaxed),
                                        state)))
                            {
                                continue_outer = true;
                                break;
                            }
                        }

                        if (allow_yielding && HPX_UNLIKELY(!continue_outer))
                        {
                            std::uint64_t const base_time2 =
                                util::hardware::timestamp();
                            if ((base_time2 - base_time) > yield_delay)
                            {
                                base_time = base_time2;
                                context.yield();
                            }
                        }

                        current = tstate.load(std::memory_order_acquire);
                    }
                }
                return current;
            }

            std::string generate_annotation(
                std::size_t const index, char const* default_name) const
            {
                return hpx::util::format("{}: thread ({})",
                    annotation_ ? annotation_ : default_name, index);
            }

            static std::uint32_t get_first_core(
                hpx::threads::mask_cref_type mask)
            {
                auto const size = hpx::threads::mask_size(mask);
                for (std::uint32_t i = 0; i != size; ++i)
                {
                    if (hpx::threads::test(mask, i))
                        return i;
                }
                return 0;
            }

            // Entry point for each worker HPX thread. Holds references to the
            // member variables of fork_join_executor.
            struct thread_function
            {
                // Fixed data for the duration of the executor.
                std::size_t const num_threads_;
                std::size_t const thread_index_;
                loop_schedule const schedule_;
                hpx::spinlock& exception_mutex_;
                std::exception_ptr& exception_;
                std::uint64_t yield_delay_;

                // Changing data for each parallel region.
                region_data_type& region_data_;
                queues_type& queues_;

                // The threads are bound to the current core.
                bool const priority_bound_;
                bool const allow_yielding_;

                static void set_state_this_thread(
                    region_data& data, thread_state const state) noexcept
                {
                    data.state_.store(state, std::memory_order_release);
                }

                [[nodiscard]] static thread_state get_state_this_thread(
                    region_data const& data) noexcept
                {
                    return data.state_.load(std::memory_order_relaxed);
                }

                void operator()() const noexcept
                {
                    region_data& data = region_data_[thread_index_].data_;

                    HPX_ASSERT(
                        get_state_this_thread(data) == thread_state::starting);
                    set_state_this_thread(data, thread_state::idle);

                    // wait as long the state is 'idle'
                    auto state = shared_data::wait_state_this_thread_while(
                        data.state_, thread_state::idle, yield_delay_,
                        std::equal_to<>(), allow_yielding_);

                    HPX_ASSERT(!priority_bound_ ||
                        thread_index_ == hpx::get_worker_thread_num());
                    while (HPX_LIKELY(state != thread_state::stopping))
                    {
                        {
#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
                            static hpx::util::itt::event notify_event(
                                "fork_join_executor::invoke_work");

                            hpx::util::itt::mark_event e(notify_event);
#endif
                            data.thread_function_helper_(region_data_,
                                thread_index_, num_threads_, queues_,
                                exception_mutex_, exception_);
                        }

                        // wait as long the state is 'idle'
                        state = shared_data::wait_state_this_thread_while(
                            data.state_, thread_state::idle, yield_delay_,
                            std::equal_to<>(), allow_yielding_);

                        HPX_ASSERT(!priority_bound_ ||
                            thread_index_ == hpx::get_worker_thread_num());
                    }

                    HPX_ASSERT(
                        get_state_this_thread(data) == thread_state::stopping);
                    set_state_this_thread(data, thread_state::stopped);
                }
            };

            void set_state_main_thread(thread_state const state) noexcept
            {
                region_data_[main_thread_].data_.state_.store(
                    state, std::memory_order_relaxed);
            }

            void set_state_all(thread_state const state) noexcept
            {
                for (std::size_t t = 0; t != num_threads_; ++t)
                {
                    region_data_[t].data_.state_.store(
                        state, std::memory_order_release);
                    HPX_SMT_PAUSE;
                }
            }

            void wait_state_all(thread_state const state) const noexcept
            {
                bool const allow_yielding =
                    stacksize_ != threads::thread_stacksize::nostack;
                for (std::size_t t = 0; t != region_data_.size(); ++t)
                {
                    if (t != main_thread_)
                    {
                        // wait for thread-state to be equal to 'state'
                        wait_state_this_thread_while(
                            region_data_[t].data_.state_, state, yield_delay_,
                            std::not_equal_to<>(), allow_yielding);
                    }
                    else
                    {
                        // the main thread should have already reached the
                        // required state
                        HPX_ASSERT(region_data_[t].data_.state_.load(
                                       std::memory_order_acquire) == state);
                    }
                }
            }

            void reschedule_with_new_priority(
                threads::thread_priority const priority) const
            {
                // Make sure the main thread runs with the required priority
                // as well. Yield with the intent to be resumed with the
                // required settings.
                hpx::this_thread::set_affinity(
                    static_cast<std::int16_t>(main_thread_), priority);
            }

            void init_threads()
            {
                auto const& rp = hpx::resource::get_partitioner();
                bool priority_bound = false;

                // The current thread could be either part of the PU-mask for
                // this executor or not. If it is part of the PU-mask, then it
                // will be associated with the corresponding PU. If the current
                // thread is not part of the PU-mask, then it will be associated
                // with the first queue.

                // Note the current thread could also be the main thread, in
                // which case its number will be equal to the number of worker
                // threads.
                main_thread_ = hpx::get_worker_thread_num();

                if (rp.get_affinity_data().affinities_disabled() ||
                    priority_ != threads::thread_priority::bound)
                {
                    main_priority_ = priority_;
                }
                else
                {
                    auto const* thread_data = threads::get_self_id_data();
                    main_priority_ =
                        thread_data ? thread_data->get_priority() : priority_;
                    if (main_priority_ != priority_)
                    {
                        // Make sure the main thread runs with the required
                        // priority as well. Yield with the intent to be resumed
                        // with the required settings.
                        reschedule_with_new_priority(priority_);

                        // The main thread should still run on the core it was
                        // running on before the priority change.
                        HPX_ASSERT(
                            main_thread_ == hpx::get_worker_thread_num());
                        HPX_ASSERT(priority_ ==
                            threads::get_self_id_data()->get_priority());
                    }
                    priority_bound = true;
                }

                // the array of queues is needed only if work-stealing was
                // enabled
                if (schedule_ == loop_schedule::dynamic)
                {
                    queues_.resize(num_threads_);
                }

                // go over all available PUs and for each one given in the
                // PU-mask create a new HPX thread
                std::size_t t = 0;
                bool main_thread_ok = false;

                bool const main_thread_is_waiting =
                    main_thread_ >= num_threads_;
                std::size_t main_pu_num = get_first_core(pu_mask_);
                if (main_thread_ != static_cast<std::size_t>(-1) &&
                    !main_thread_is_waiting)
                {
                    main_pu_num = rp.get_pu_num(main_thread_);
                }

                if (!hpx::threads::test(pu_mask_, main_pu_num) ||
                    num_threads_ == 1)
                {
                    main_thread_ok = true;
                    main_thread_ = t++;
                    main_pu_num = rp.get_pu_num(main_thread_);
                    set_state_main_thread(thread_state::idle);
                }

                // explicitly make the main thread ready, if needed
                if (main_thread_is_waiting)
                {
                    set_state_main_thread(thread_state::idle);
                }

                if (num_threads_ > 1)
                {
                    std::size_t const num_pus = pool_->get_os_thread_count();

                    for (std::size_t pu = 0; t != num_threads_ && pu != num_pus;
                        ++pu)
                    {
                        std::size_t const pu_num = rp.get_pu_num(pu);
                        if (!main_thread_ok && pu == main_thread_)
                        {
                            // the initializing thread is expected to
                            // participate in evaluating parallel regions
                            HPX_ASSERT(hpx::threads::test(pu_mask_, pu_num));
                            main_thread_ok = true;
                            main_thread_ = t++;
                            main_pu_num = rp.get_pu_num(main_thread_);

                            set_state_main_thread(thread_state::idle);
                            continue;
                        }

                        // don't double-book core that runs main thread
                        if (main_thread_ok && main_pu_num == pu_num)
                        {
                            continue;
                        }

                        // create an HPX thread only for cores in the given
                        // PU-mask
                        if (!hpx::threads::test(pu_mask_, pu_num))
                        {
                            continue;
                        }

                        region_data_[t].data_.state_.store(
                            thread_state::starting, std::memory_order_relaxed);

                        auto const policy =
                            launch::async_policy(priority_, stacksize_,
                                threads::thread_schedule_hint{
                                    static_cast<std::int16_t>(t)});

                        hpx::threads::thread_description desc(
                            generate_annotation(pu_num, "fork_join_executor"));
                        hpx::detail::post_policy_dispatch<
                            launch::async_policy>::call(policy, desc, pool_,
                            thread_function{.num_threads_ = num_threads_,
                                .thread_index_ = t,
                                .schedule_ = schedule_,
                                .exception_mutex_ = exception_mutex_,
                                .exception_ = exception_,
                                .yield_delay_ = yield_delay_,
                                .region_data_ = region_data_,
                                .queues_ = queues_,
                                .priority_bound_ = priority_bound,
                                .allow_yielding_ = stacksize_ !=
                                    threads::thread_stacksize::nostack});

                        ++t;
                    }
                }

                // the main thread should have been associated with a queue
                HPX_ASSERT(main_thread_ok || main_thread_ >= num_threads_);

                // there have to be as many HPX threads as there are set bits in
                // the PU-mask
                HPX_ASSERT(t == num_threads_);

                wait_state_all(thread_state::idle);
            }

            static constexpr void init_local_work_queue(queue_type& queue,
                std::size_t const thread_index, std::size_t const num_threads,
                std::size_t const size) noexcept
            {
                auto const part_begin = (thread_index * size) / num_threads;
                auto const part_end = ((thread_index + 1) * size) / num_threads;

                // Guard:the static scheduling also uses
                //  contiguous_index_queue internally.

                HPX_ASSERT_MSG(
                    size <= static_cast<std::size_t>(
                                (std::numeric_limits<std::uint32_t>::max)()),
                    "fork_join_executor: ranges larger than"
                    " UINT32_MAX are not supported");

                queue.reset(static_cast<std::uint32_t>(part_begin),
                    static_cast<std::uint32_t>(part_end));
            }

            static hpx::threads::mask_type full_mask(
                std::size_t const num_threads)
            {
                auto const& rp = hpx::resource::get_partitioner();

                hpx::threads::mask_type mask(
                    hpx::threads::hardware_concurrency());
                for (std::size_t i = 0; i != num_threads; ++i)
                {
                    hpx::threads::set(mask, rp.get_pu_num(i));
                }
                return mask;
            }

            static std::size_t get_region_data_size(
                std::size_t const num_threads,
                threads::thread_pool_base const* pool)
            {
                return hpx::get_worker_thread_num() ==
                        pool->get_os_thread_count() ?
                    num_threads + 1 :
                    num_threads;
            }

        public:
            /// \cond NOINTERNAL
            explicit shared_data(threads::thread_priority const priority,
                threads::thread_stacksize const stacksize,
                loop_schedule const sched,
                std::chrono::nanoseconds const yield_delay)
              : pool_(threads::detail::get_self_or_default_pool())
              , priority_(priority)
              , stacksize_(stacksize)
              , schedule_(sched)
              , yield_delay_(static_cast<std::uint64_t>(
                    static_cast<double>(yield_delay.count()) /
                    pool_->timestamp_scale()))
              , num_threads_(pool_->get_os_thread_count())
              , pu_mask_(full_mask(num_threads_))
              , region_data_(get_region_data_size(num_threads_, pool_))
            {
                HPX_ASSERT(pool_);

                init_threads();
            }

            explicit shared_data(threads::thread_priority const priority,
                threads::thread_stacksize const stacksize,
                loop_schedule const sched,
                std::chrono::nanoseconds const yield_delay,
                hpx::threads::mask_cref_type pu_mask)
              : pool_(threads::detail::get_self_or_default_pool())
              , priority_(priority)
              , stacksize_(stacksize)
              , schedule_(sched)
              , yield_delay_(static_cast<std::uint64_t>(
                    static_cast<double>(yield_delay.count()) /
                    pool_->timestamp_scale()))
              , num_threads_(hpx::threads::count(pu_mask))
              , pu_mask_(pu_mask)
              , region_data_(get_region_data_size(num_threads_, pool_))
            {
                HPX_ASSERT(pool_);
                if (pool_ == nullptr || num_threads_ == 0 ||
                    num_threads_ > pool_->get_os_thread_count())
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "fork_join_executor::shared_data::shared_data",
                        "unexpected number of PUs in given mask: {}, available "
                        "threads: {}",
                        pu_mask, pool_ ? pool_->get_os_thread_count() : -1);
                }

                init_threads();
            }

            shared_data(shared_data const&) = delete;
            shared_data(shared_data&&) = delete;
            shared_data& operator=(shared_data const&) = delete;
            shared_data& operator=(shared_data&&) = delete;

            ~shared_data()
            {
                set_state_all(thread_state::stopping);
                set_state_main_thread(thread_state::stopped);
                wait_state_all(thread_state::stopped);

                // Make sure the main thread's priority is reset, if needed.
                // Yield with the intent to be resumed with the required
                // settings.
                if (priority_ == threads::thread_priority::bound &&
                    main_priority_ != priority_)
                {
                    reschedule_with_new_priority(main_priority_);
                }
            }

            bool operator==(shared_data const& rhs) const noexcept
            {
                return pool_ == rhs.pool_ && priority_ == rhs.priority_ &&
                    stacksize_ == rhs.stacksize_ &&
                    schedule_ == rhs.schedule_ &&
                    yield_delay_ == rhs.yield_delay_ &&
                    pu_mask_ == rhs.pu_mask_;
            }

            bool operator!=(shared_data const& rhs) const noexcept
            {
                return !(*this == rhs);
            }

        private:
            // This struct implements the main work loop for a single parallel
            // for loop. The indirection through this struct is done to allow
            // passing the original template parameters F, S, and Tuple
            // (additional arguments packed into a tuple) given to
            // bulk_sync_execute without wrapping it into hpx::function or
            // similar.
            template <typename Result, typename F, typename S, typename Tuple>
            struct thread_function_helper
            {
                using argument_pack_type = std::decay_t<Tuple>;
                using index_pack_type = hpx::detail::fused_index_pack_t<Tuple>;

                template <std::size_t... Is_, typename F_, typename A_,
                    typename Tuple_>
                static constexpr decltype(auto) invoke_helper(
                    hpx::util::index_pack<Is_...>, F_&& f, A_&& a, Tuple_&& t)
                {
                    // NOLINTBEGIN(bugprone-use-after-move)
                    return HPX_INVOKE(HPX_FORWARD(F_, f), HPX_FORWARD(A_, a),
                        hpx::get<Is_>(HPX_FORWARD(Tuple_, t))...);
                    // NOLINTEND(bugprone-use-after-move)
                }

                static void set_state(std::atomic<thread_state>& tstate,
                    thread_state const state) noexcept
                {
                    tstate.store(state, std::memory_order_release);
                }

                // Main entry point for a single parallel region (static
                // scheduling).
                static void call_static(region_data_type& rdata,
                    std::size_t const thread_index,
                    std::size_t const num_threads, queues_type&,
                    hpx::spinlock& exception_mutex,
                    std::exception_ptr& exception) noexcept
                {
                    region_data& data = rdata[thread_index].data_;
                    hpx::detail::try_catch_exception_ptr(
                        [&] {
                            // Cast void pointers back to the actual types given
                            // to bulk_sync_execute.
                            auto& element_function =
                                *static_cast<F*>(data.element_function_);
                            auto& shape = *static_cast<S const*>(data.shape_);
                            auto& argument_pack =
                                *static_cast<Tuple*>(data.argument_pack_);

                            // Set up the local queues and state.
                            std::size_t const size = hpx::util::size(shape);

                            auto part_begin =
                                (thread_index * size) / num_threads;
                            auto const part_end =
                                ((thread_index + 1) * size) / num_threads;

                            set_state(data.state_, thread_state::active);

                            // Process local items.
                            for (; part_begin != part_end; ++part_begin)
                            {
                                auto it = std::next(
                                    hpx::util::begin(shape), part_begin);
                                if constexpr (std::is_void_v<Result>)
                                {
                                    invoke_helper(index_pack_type{},
                                        element_function, *it, argument_pack);
                                }
                                else
                                {
                                    auto& results =
                                        *static_cast<Result*>(data.results_);
                                    results[part_begin] = invoke_helper(
                                        index_pack_type{}, element_function,
                                        *it, argument_pack);
                                }
                            }
                        },
                        [&](std::exception_ptr&& ep) {
                            std::lock_guard<decltype(exception_mutex)> l(
                                exception_mutex);
                            if (!exception)
                            {
                                exception = HPX_MOVE(ep);
                            }
                        });

                    set_state(data.state_, thread_state::idle);
                    if (data.sync_with_main_thread_)
                    {
                        data.sync_with_main_thread_->count_down(1);
                    }
                }

                // Main entry point for a single parallel region (dynamic
                // scheduling).
                static void call_dynamic(region_data_type& rdata,
                    std::size_t const thread_index,
                    std::size_t const num_threads, queues_type& queues,
                    hpx::spinlock& exception_mutex,
                    std::exception_ptr& exception) noexcept
                {
                    region_data& data = rdata[thread_index].data_;
                    hpx::detail::try_catch_exception_ptr(
                        [&] {
                            // Cast void pointers back to the actual types given
                            // to bulk_sync_execute.
                            auto& element_function =
                                *static_cast<F*>(data.element_function_);
                            auto& shape = *static_cast<S const*>(data.shape_);
                            auto& argument_pack =
                                *static_cast<Tuple*>(data.argument_pack_);

                            // Set up the local queues and state.
                            queue_type& local_queue =
                                queues[thread_index].data_;
                            std::size_t const size = hpx::util::size(shape);
                            init_local_work_queue(
                                local_queue, thread_index, num_threads, size);

                            set_state(data.state_, thread_state::active);

                            // Process local items first.
                            hpx::optional<std::uint32_t> index;
                            while ((index = local_queue.pop_left()))
                            {
                                auto it =
                                    std::next(hpx::util::begin(shape), *index);
                                if constexpr (std::is_void_v<Result>)
                                {
                                    invoke_helper(index_pack_type{},
                                        element_function, *it, argument_pack);
                                }
                                else
                                {
                                    auto& results =
                                        *static_cast<Result*>(data.results_);
                                    results[*index] = invoke_helper(
                                        index_pack_type{}, element_function,
                                        *it, argument_pack);
                                }
                            }

                            // As loop schedule is dynamic, steal from neighboring
                            // threads.
                            for (std::size_t offset = 1; offset < num_threads;
                                ++offset)
                            {
                                std::size_t const neighbor_index =
                                    (thread_index + offset) % num_threads;

                                if (rdata[neighbor_index].data_.state_.load(
                                        std::memory_order_acquire) !=
                                    thread_state::active)
                                {
                                    continue;
                                }

                                queue_type& neighbor_queue =
                                    queues[neighbor_index].data_;

                                while ((index = neighbor_queue.pop_right()))
                                {
                                    auto it = std::next(
                                        hpx::util::begin(shape), *index);

                                    if constexpr (std::is_void_v<Result>)
                                    {
                                        invoke_helper(index_pack_type{},
                                            element_function, *it,
                                            argument_pack);
                                    }
                                    else
                                    {
                                        auto& results = *static_cast<Result*>(
                                            data.results_);
                                        results[*index] = invoke_helper(
                                            index_pack_type{}, element_function,
                                            *it, argument_pack);
                                    }
                                }
                            }
                        },
                        [&](std::exception_ptr&& ep) {
                            std::lock_guard<decltype(exception_mutex)> l(
                                exception_mutex);
                            if (!exception)
                            {
                                exception = HPX_MOVE(ep);
                            }
                        });

                    set_state(data.state_, thread_state::idle);
                    if (data.sync_with_main_thread_)
                    {
                        data.sync_with_main_thread_->count_down(1);
                    }
                }
            };

            template <typename Fs, typename Args>
            struct thread_function_helper_invoke
            {
                using function_pack_type = std::decay_t<Fs>;
                using index_pack_type = hpx::detail::fused_index_pack_t<Fs>;

                static constexpr std::uint32_t Size = hpx::tuple_size_v<Fs>;

                static void set_state(std::atomic<thread_state>& tstate,
                    thread_state const state) noexcept
                {
                    tstate.store(state, std::memory_order_release);
                }

                // Main entry point for a single parallel invoke region
                static void call(region_data_type& rdata,
                    std::size_t const thread_index, std::size_t, queues_type&,
                    hpx::spinlock& exception_mutex,
                    std::exception_ptr& exception) noexcept
                {
                    region_data& data = rdata[thread_index].data_;
                    hpx::detail::try_catch_exception_ptr(
                        [&] {
                            // Cast void pointers back to the actual types given
                            // to sync_invoke.
                            auto& fs =
                                *static_cast<Fs*>(data.element_function_);
                            auto& args =
                                *static_cast<Args*>(data.argument_pack_);

                            auto part_begin = hpx::get<0>(args);
                            auto const part_end = hpx::get<1>(args);

                            set_state(data.state_, thread_state::active);

                            // Process local items.
                            for (/**/; part_begin != part_end; ++part_begin)
                            {
                                hpx::visit([](auto&& f) { f(); },
                                    hpx::detail::runtime_get(fs, part_begin));
                            }
                        },
                        [&](std::exception_ptr&& ep) {
                            std::lock_guard<decltype(exception_mutex)> l(
                                exception_mutex);
                            if (!exception)
                            {
                                exception = HPX_MOVE(ep);
                            }
                        });

                    set_state(data.state_, thread_state::idle);
                    if (data.sync_with_main_thread_)
                    {
                        data.sync_with_main_thread_->count_down(1);
                    }
                }
            };

            template <typename Result, typename F, typename S, typename Args>
            thread_function_helper_type* set_all_states_and_region_data(
                void* results, thread_state const state, F& f, S const& shape,
                Args& argument_pack, hpx::latch* sync_with_main_thread) noexcept
            {
                thread_function_helper_type* func;
                if (schedule_ == loop_schedule::static_ || num_threads_ == 1)
                {
                    func = &thread_function_helper<Result, F, S,
                        Args>::call_static;
                }
                else
                {
                    func = &thread_function_helper<Result, F, S,
                        Args>::call_dynamic;
                }

                for (std::size_t t = 0; t != num_threads_; ++t)
                {
                    region_data& data = region_data_[t].data_;

                    // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
                    data.element_function_ = &f;
                    data.shape_ = &shape;
                    data.argument_pack_ = &argument_pack;
                    data.thread_function_helper_ = func;
                    data.results_ = results;
                    data.sync_with_main_thread_ = sync_with_main_thread;
                    // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

                    data.state_.store(state, std::memory_order_release);
                }
                return func;
            }

            template <typename Fs, typename Args>
            std::size_t set_all_states_and_region_data_invoke(
                thread_state const state, Fs& function_pack, Args& args,
                hpx::latch* sync_with_main_thread) noexcept
            {
                constexpr thread_function_helper_type* func =
                    &thread_function_helper_invoke<Fs, Args>::call;

                for (std::size_t t = 0; t != num_threads_; ++t)
                {
                    if (t == main_thread_)
                    {
                        continue;    // don't run sync task on main thread
                    }

                    region_data& data = region_data_[t].data_;
                    if (data.state_.load(std::memory_order_acquire) ==
                        thread_state::idle)
                    {
                        // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
                        data.element_function_ = &function_pack;
                        data.shape_ = nullptr;
                        data.argument_pack_ = &args;
                        data.thread_function_helper_ = func;
                        data.sync_with_main_thread_ = sync_with_main_thread;
                        // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

                        data.state_.store(state, std::memory_order_release);
                        return t;
                    }
                }

                return static_cast<std::size_t>(-1);
            }

            template <typename F>
            void invoke_work(F&& f)
            {
                // Start work on the main thread.
                f(region_data_, main_thread_, num_threads_, queues_,
                    exception_mutex_, exception_);

                // Wait for all threads to finish their work assigned to
                // them in this parallel region.
                wait_state_all(thread_state::idle);

                // rethrow exception, if any
                if (exception_)
                {
                    std::rethrow_exception(HPX_MOVE(exception_));
                }
            }

        public:
            template <typename F, typename S, typename... Ts>
            decltype(auto) bulk_sync_execute(F&& f, S const& shape, Ts&&... ts)
            {
                // protect against nested use of this executor instance
                if (region_data_[main_thread_].data_.state_.load(
                        std::memory_order_relaxed) != thread_state::idle)
                {
                    HPX_THROW_EXCEPTION(error::bad_request, "bulk_sync_execute",
                        "unexpected state, is this instance of "
                        "fork_join_executor being used in nested ways?");
                }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                hpx::scoped_annotation annotate(
                    generate_annotation(hpx::get_worker_thread_num(),
                        "fork_join_executor::bulk_sync_execute"));
#endif
                exception_ = std::exception_ptr();

                // Set the data for this parallel region
                auto argument_pack =
                    hpx::forward_as_tuple(HPX_FORWARD(Ts, ts)...);

                using result_type =
                    hpx::parallel::execution::detail::bulk_execute_result_t<F,
                        S, Ts...>;

                // do things differently if the main thread is not participating
                hpx::latch* sync_with_main_thread = nullptr;
                auto on_exit = hpx::experimental::scope_exit(
                    [&] { delete sync_with_main_thread; });

                if (main_thread_ >= num_threads_)
                {
                    sync_with_main_thread = new hpx::latch(
                        static_cast<std::ptrdiff_t>(num_threads_ + 1));
                }

                if constexpr (std::is_void_v<result_type>)
                {
                    // Signal all worker threads to start partitioning work
                    // for themselves, and then starting the actual work.
                    thread_function_helper_type* func =
                        set_all_states_and_region_data<void>(nullptr,
                            thread_state::partitioning_work, f, shape,
                            argument_pack, sync_with_main_thread);

                    if (sync_with_main_thread == nullptr)
                    {
                        invoke_work(func);
                    }
                    else
                    {
                        // the main thread must be put to sleep to avoid
                        // over-subscription of the cores
                        sync_with_main_thread->arrive_and_wait();
                    }
                }
                else
                {
                    result_type results(hpx::util::size(shape));

                    // Signal all worker threads to start partitioning work
                    // for themselves, and then starting the actual work.
                    thread_function_helper_type* func =
                        set_all_states_and_region_data<result_type>(&results,
                            thread_state::partitioning_work, f, shape,
                            argument_pack, sync_with_main_thread);

                    if (sync_with_main_thread == nullptr)
                    {
                        invoke_work(func);
                    }
                    else
                    {
                        // the main thread must be put to sleep to avoid
                        // over-subscription of the cores
                        sync_with_main_thread->arrive_and_wait();
                    }

                    return results;
                }
            }

            template <typename F, typename S, typename... Ts>
            decltype(auto) bulk_async_execute(F&& f, S const& shape, Ts&&... ts)
            {
                using result_type =
                    hpx::parallel::execution::detail::bulk_execute_result_t<F,
                        S, Ts...>;

                // Forward to the synchronous version as we can't create
                // futures to the completion of the parallel region (this HPX
                // thread participates in computation).
                return hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if constexpr (std::is_void_v<result_type>)
                        {
                            bulk_sync_execute(HPX_FORWARD(F, f), shape,
                                HPX_FORWARD(Ts, ts)...);
                            return hpx::make_ready_future();
                        }
                        else
                        {
                            auto&& result = bulk_sync_execute(HPX_FORWARD(F, f),
                                shape, HPX_FORWARD(Ts, ts)...);
                            return hpx::make_ready_future(HPX_MOVE(result));
                        }
                    },
                    [&](std::exception_ptr&& ep) {
                        return hpx::make_exceptional_future<result_type>(
                            HPX_MOVE(ep));
                    });
            }

            template <typename FunctionPack>
            void sync_invoke_helper(FunctionPack& function_pack,
                std::size_t first, std::size_t size)
            {
                // protect against nested use of this executor instance
                if (region_data_[main_thread_].data_.state_.load(
                        std::memory_order_relaxed) != thread_state::idle)
                {
                    HPX_THROW_EXCEPTION(error::bad_request,
                        "sync_invoke_helper",
                        "unexpected state, is this instance of "
                        "fork_join_executor being used in nested ways?");
                }

                // Set the data for this parallel region
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                hpx::scoped_annotation annotate(
                    generate_annotation(hpx::get_worker_thread_num(),
                        "fork_join_executor::sync_invoke"));
#endif
                exception_ = std::exception_ptr();

                auto args = hpx::make_tuple(first, size);

                // do things differently if the main thread is not participating
                hpx::latch* sync_with_main_thread = nullptr;
                if (main_thread_ >= num_threads_)
                {
                    sync_with_main_thread =
                        new hpx::latch(static_cast<std::ptrdiff_t>(2));
                }

                // Find a worker thread and signal it to start partitioning work
                // for itself, and then start the actual work.
                std::size_t const worker_thread =
                    set_all_states_and_region_data_invoke(
                        thread_state::partitioning_work, function_pack, args,
                        sync_with_main_thread);

                if (worker_thread == static_cast<std::size_t>(-1))
                {
                    delete sync_with_main_thread;

                    HPX_THROW_EXCEPTION(error::bad_request,
                        "sync_invoke_helper",
                        "no available worker threads, is this instance of "
                        "fork_join_executor being used in nested ways?");
                }

                if (sync_with_main_thread == nullptr)
                {
                    // Wait for the thread to finish their work assigned to
                    // them in this parallel region.
                    wait_state_this_thread_while(
                        region_data_[worker_thread].data_.state_,
                        thread_state::idle, yield_delay_, std::not_equal_to<>(),
                        stacksize_ != threads::thread_stacksize::nostack);
                }
                else
                {
                    // the main thread must be put to sleep to avoid
                    // over-subscription of the cores
                    sync_with_main_thread->arrive_and_wait();
                    delete sync_with_main_thread;
                }

                // rethrow exception, if any
                if (exception_)
                {
                    std::rethrow_exception(HPX_MOVE(exception_));
                }
            }

            template <typename... Fs>
            void sync_invoke(Fs&&... fs)
            {
                auto function_pack =
                    hpx::forward_as_tuple(HPX_FORWARD(Fs, fs)...);

                sync_invoke_helper(function_pack, 0, sizeof...(Fs));
            }

            template <typename... Fs>
            hpx::future<void> async_invoke(Fs&&... fs)
            {
                // Forward to the synchronous version as we can't create
                // futures to the completion of the parallel region (this HPX
                // thread participates in computation).
                return hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        sync_invoke(HPX_FORWARD(Fs, fs)...);
                        return hpx::make_ready_future();
                    },
                    [&](std::exception_ptr&& ep) {
                        return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
                    });
            }
        };

    public:
        template <typename FunctionPack>
        void sync_invoke_helper(FunctionPack& function_pack, std::size_t first,
            std::size_t size) const
        {
            shared_data_->sync_invoke_helper(function_pack, first, size);
        }

    private:
        std::shared_ptr<shared_data> shared_data_ = nullptr;

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_sync_execute_t,
            fork_join_executor const& exec, F&& f, S const& shape, Ts&&... ts)
        {
            return exec.shared_data_->bulk_sync_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_async_execute_t,
            fork_join_executor const& exec, F&& f, S const& shape, Ts&&... ts)
        {
            return exec.shared_data_->bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Fs>
            requires(std::invocable<F> && (std::invocable<Fs> && ...))
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::async_invoke_t,
            fork_join_executor const& exec, F&& f, Fs&&... fs)
        {
            return exec.shared_data_->async_invoke(
                HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        }

        template <typename F, typename... Fs>
            requires(std::invocable<F> && (std::invocable<Fs> && ...))
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::sync_invoke_t,
            fork_join_executor const& exec, F&& f, Fs&&... fs)
        {
            return exec.shared_data_->sync_invoke(
                HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        }

    public:
        bool operator==(fork_join_executor const& rhs) const noexcept
        {
            return *shared_data_ == *rhs.shared_data_;
        }

        bool operator!=(fork_join_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        [[nodiscard]] fork_join_executor const& context() const noexcept
        {
            return *this;
        }
        /// \endcond

        /// \brief Construct a fork_join_executor.
        ///
        /// \param priority  The priority of the worker threads.
        /// \param stacksize The stacksize of the worker threads.
        /// \param sched     The loop schedule of the parallel regions.
        /// \param yield_delay The time after which the executor yields to other
        ///        work if it has not received any new work for execution.
        explicit fork_join_executor(
            threads::thread_priority priority = threads::thread_priority::bound,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::small_,
            loop_schedule sched = loop_schedule::dynamic,
            std::chrono::nanoseconds yield_delay = std::chrono::microseconds(
                300))
        {
            shared_data_ = std::make_shared<shared_data>(
                priority, stacksize, sched, yield_delay);
        }

        /// \brief Construct a fork_join_executor.
        ///
        /// \param pu_mask   The PU-mask to use for placing the created threads
        /// \param priority  The priority of the worker threads.
        /// \param stacksize The stacksize of the worker threads.
        /// \param sched     The loop schedule of the parallel regions.
        /// \param yield_delay The time after which the executor yields to other
        ///        work if it has not received any new work for execution.
        explicit fork_join_executor(hpx::threads::mask_cref_type pu_mask,
            threads::thread_priority priority = threads::thread_priority::bound,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::small_,
            loop_schedule sched = loop_schedule::dynamic,
            std::chrono::nanoseconds yield_delay = std::chrono::microseconds(
                300))
        {
            shared_data_ = std::make_shared<shared_data>(
                priority, stacksize, sched, yield_delay, pu_mask);
        }

        friend fork_join_executor tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            fork_join_executor const& exec, char const* annotation) noexcept
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.shared_data_->annotation_ = annotation;
            return exec_with_annotation;
        }

        friend fork_join_executor tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            fork_join_executor const& exec, std::string annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.shared_data_->annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        friend char const* tag_invoke(
            hpx::execution::experimental::get_annotation_t,
            fork_join_executor const& exec) noexcept
        {
            return exec.shared_data_->annotation_;
        }

        friend auto tag_invoke(
            hpx::execution::experimental::get_processing_units_mask_t,
            fork_join_executor const& exec) noexcept
        {
            return exec.shared_data_->pu_mask_;
        }

        friend auto tag_invoke(hpx::execution::experimental::get_cores_mask_t,
            fork_join_executor const& exec) noexcept
        {
            return exec.shared_data_->pu_mask_;
        }

        friend std::size_t tag_invoke(
            hpx::execution::experimental::get_first_core_t,
            fork_join_executor const& exec) noexcept
        {
            return shared_data::get_first_core(exec.shared_data_->pu_mask_);
        }

        template <typename Parameters>
            requires(hpx::traits::is_executor_parameters_v<Parameters>)
        friend std::size_t tag_invoke(
            hpx::execution::experimental::processing_units_count_t,
            Parameters&&, fork_join_executor const& exec,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0) noexcept
        {
            return exec.shared_data_->num_threads_;
        }

        /// \cond NOINTERNAL
        enum class init_mode : std::uint8_t
        {
            no_init
        };

        explicit fork_join_executor(init_mode) {}
        /// \endcond
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, fork_join_executor::loop_schedule schedule);

    /// \cond NOINTERNAL
    template <>
    struct is_bulk_one_way_executor<fork_join_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<fork_join_executor> : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    // Create a fork_join_executor from another executor
    namespace detail {

        template <typename Executor>
        decltype(auto) fork_join_executor_from(Executor&& exec)
        {
            hpx::threads::mask_type const pu_mask =
                get_processing_units_mask(exec);

            hpx::launch const policy = exec.policy();
            hpx::threads::thread_priority const priority =
                policy.get_priority();
            hpx::threads::thread_stacksize const stacksize =
                policy.get_stacksize();

            fork_join_executor result(pu_mask, priority, stacksize);

            char const* annotation = get_annotation(exec);
            if (annotation == nullptr)
                return result;

            return with_annotation(result, annotation);
        }
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    decltype(auto) fork_join_executor_from(
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>&
            exec)
    {
        return detail::fork_join_executor_from(exec);
    }

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    decltype(auto) fork_join_executor_from(
        hpx::execution::parallel_policy_executor<Policy,
            HierarchicalSpawning> const& exec)
    {
        return detail::fork_join_executor_from(exec);
    }

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    decltype(auto) fork_join_executor_from(
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>&&
            exec)
    {
        return detail::fork_join_executor_from(HPX_MOVE(exec));
    }

    // fallback for everything but parallel_executor
    HPX_CXX_CORE_EXPORT template <typename Executor>
    decltype(auto) fork_join_executor_from(Executor&& exec)
    {
        return HPX_FORWARD(Executor, exec);
    }
}    // namespace hpx::execution::experimental
