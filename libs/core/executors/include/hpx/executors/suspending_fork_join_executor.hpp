//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file suspending_fork_join_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/executor_scheduler.hpp>
#include <hpx/executors/fork_join_executor.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::execution::experimental {

    namespace detail {

        // The state shared by all copies of an executor and by its worker
        // threads: the PU mask, if one was given, and the worker threads. It
        // is created by the constructor that takes a PU mask, or on first
        // use. The worker threads are created on first use (see start).
        HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT
            suspending_fork_join_shared_data
        {
            using loop_schedule = fork_join_executor::loop_schedule;
            using region_function_type = void(
                void*, suspending_fork_join_shared_data&, std::size_t) noexcept;
            using queue_type =
                hpx::concurrency::detail::contiguous_index_queue<std::uint32_t>;

            // pu_mask: the PUs to use, all PUs of the pool if nullptr
            suspending_fork_join_shared_data(threads::thread_priority priority,
                threads::thread_stacksize stacksize, loop_schedule sched,
                std::chrono::nanoseconds spin_duration,
                hpx::threads::mask_type const* pu_mask);

            suspending_fork_join_shared_data(
                suspending_fork_join_shared_data const&) = delete;
            suspending_fork_join_shared_data(
                suspending_fork_join_shared_data&&) = delete;
            suspending_fork_join_shared_data& operator=(
                suspending_fork_join_shared_data const&) = delete;
            suspending_fork_join_shared_data& operator=(
                suspending_fork_join_shared_data&&) = delete;

            ~suspending_fork_join_shared_data();

            // Create the worker threads, on the pool of the calling thread
            // (or the default pool), if that wasn't done yet. Returns true if
            // this call created them.
            bool start(char const* annotation);

            bool started() const noexcept
            {
                return started_.load(std::memory_order_acquire);
            }

            // Run one parallel region of size indices on count slots,
            // starting at slot first (wrapping around, count == 0 means all
            // slots): f(body, data, index) runs the share with the given
            // index, from 0 to count - 1. The worker threads of the other
            // slots are not woken.
            void execute_region(region_function_type* f, void* body,
                std::size_t size, std::size_t first, std::size_t count);

            // Run one parallel region as execute_region does, if the worker
            // threads are free: returns false, and does nothing, if they run
            // a region already (another one, or the one this is called from)
            // or if they have exited.
            bool try_execute_region(region_function_type* f, void* body,
                std::size_t size, std::size_t first, std::size_t count);

            // Run the share with the given index of the current region,
            // calling body(first, last) for ranges of its indices.
            template <typename Body>
            void run_share(std::size_t const index, Body const& body) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&] {
                        // Each share works on its own copy of the body (and
                        // of the function it calls), as each task of a
                        // parallel_executor does: functions with state, such
                        // as the ones of a for_loop with inductions, can't
                        // be called by several threads at the same time.
                        Body share_body(body);
                        if (dynamic_)
                        {
                            // Items are taken in chunks of 1/region_threads
                            // of the items left in a queue (but at least
                            // one), one atomic operation per chunk. The
                            // chunks shrink as a queue drains, so the other
                            // shares can still take a part of an unevenly
                            // loaded queue.
                            std::size_t const n = region_threads_;
                            auto const divisor = static_cast<std::uint32_t>(n);
                            hpx::optional<
                                std::pair<std::uint32_t, std::uint32_t>>
                                chunk;

                            // process the own items first
                            queue_type& local_queue = queues_[index].data_;
                            while (
                                (chunk = local_queue.pop_left_chunk(divisor)))
                            {
                                share_body(
                                    static_cast<std::size_t>(chunk->first),
                                    static_cast<std::size_t>(chunk->second));
                            }

                            // then steal from the other shares
                            for (std::size_t offset = 1; offset != n; ++offset)
                            {
                                queue_type& queue =
                                    queues_[(index + offset) % n].data_;
                                while ((chunk = queue.pop_right_chunk(divisor)))
                                {
                                    share_body(
                                        static_cast<std::size_t>(chunk->first),
                                        static_cast<std::size_t>(
                                            chunk->second));
                                }
                            }
                        }
                        else
                        {
                            std::size_t const size = region_size_;
                            std::size_t const n = region_threads_;
                            share_body(
                                (index * size) / n, ((index + 1) * size) / n);
                        }
                    },
                    [&](std::exception_ptr&& ep) {
                        store_exception(HPX_MOVE(ep));
                    });
            }

            void store_exception(std::exception_ptr&& ep) noexcept;

            // the main loop of the worker thread of the given slot, position
            // is its place among the worker threads
            void worker_main(std::size_t slot, std::size_t position) noexcept;

            // Wake the worker threads and wait until they have exited. The
            // executor can't be used afterwards. This is done by the
            // destructor, and when the runtime starts to shut down: it can't
            // stop while the worker threads are suspended.
            void stop_workers() noexcept;

            // Stop the worker threads of the given shared data when the
            // runtime starts to shut down, if it still exists then.
            static void stop_on_shutdown(
                std::weak_ptr<suspending_fork_join_shared_data> data);

            // the shared data registered with stop_on_shutdown, and the
            // shared instances (see suspending_fork_join_shared_instance)
            struct shutdown_registry;

            // Members that are fixed once the worker threads were created
            // (see start).
            threads::thread_pool_base* pool_ = nullptr;
            threads::thread_priority priority_;
            threads::thread_stacksize stacksize_;
            loop_schedule schedule_;
            std::chrono::nanoseconds spin_duration_;
            std::uint64_t spin_rounds_ = 0;
            std::size_t num_threads_ = 0;
            std::size_t num_workers_ = 0;
            std::size_t caller_slot_ = static_cast<std::size_t>(-1);

            // the given PU mask, or all PUs of the pool once the worker
            // threads were created
            hpx::threads::mask_type pu_mask_;
            bool has_pu_mask_ = false;

            // set once the worker threads were created
            std::mutex start_mtx_;
            std::atomic<bool> started_ = false;

            // Members that change for each parallel region. They are written
            // by the calling thread before it starts the region.
            region_function_type* region_function_ = nullptr;
            void* region_body_ = nullptr;
            std::size_t region_size_ = 0;
            std::size_t region_threads_ = 0;
            bool dynamic_ = false;

            // Set while a region runs, and once the worker threads were
            // stopped.
            std::atomic<bool> in_region_ = false;

            hpx::spinlock exception_mtx_;
            std::exception_ptr exception_;

            // the queues for dynamic scheduling, one per slot
            std::vector<hpx::util::cache_aligned_data<queue_type>> queues_;

            // Changed for each parallel region (and to stop the worker
            // threads): a counter (the upper 32 bits), the first slot of the
            // region (16 bits) and the number of its slots (the lower 16
            // bits, 0 to stop the worker threads). A worker thread reads them
            // together, so that it knows whether it takes part in the region
            // it sees.
            hpx::util::cache_aligned_data<std::atomic<std::uint64_t>> epoch_;

            // set once the worker threads were asked to stop
            std::atomic<bool> stopped_ = false;

            // the number of worker threads that still work on the current
            // region
            hpx::util::cache_aligned_data<std::atomic<std::size_t>> remaining_;

            // the number of worker threads that have not exited yet
            std::atomic<std::size_t> alive_ = 0;

            // Where a worker thread suspends while it waits for a new region.
            // Each one has its own, so that the suspended worker threads are
            // woken in a tree (see wake_workers) instead of all by the
            // calling thread.
            struct parking
            {
                hpx::spinlock mtx;
                hpx::lcos::local::detail::condition_variable cv;
                std::atomic<bool> parked = false;
            };
            std::unique_ptr<hpx::util::cache_aligned_data<parking>[]> parking_;

            // The calling thread suspends on this condition variable when it
            // waits for the worker threads to finish a region.
            hpx::spinlock caller_mtx_;
            hpx::lcos::local::detail::condition_variable caller_cv_;
            std::atomic<bool> caller_parked_ = false;

        private:
            // The slots of a region: count slots, starting at slot first
            // (wrapping around).
            struct region_slots
            {
                std::size_t first;
                std::size_t count;
            };

            static constexpr region_slots decode_slots(
                std::uint64_t const epoch) noexcept
            {
                return {static_cast<std::size_t>((epoch >> 16) & 0xffff),
                    static_cast<std::size_t>(epoch & 0xffff)};
            }

            // the slots whose worker threads are woken: all of them to stop
            region_slots wake_slots(region_slots const slots) const noexcept
            {
                return slots.count == 0 ? region_slots{0, num_threads_} : slots;
            }

            // The worker threads that take part in a region, in the order of
            // their slots in the region, are woken in a tree (see
            // publish_region). Returns the place in that order of the worker
            // thread of the given slot, or the number of these worker threads
            // if it doesn't take part.
            std::size_t wake_index(
                region_slots slots, std::size_t slot) const noexcept;

            // the position of the worker thread at the given place in that
            // order
            std::size_t wake_position(
                region_slots slots, std::size_t index) const noexcept;

            // the number of worker threads that take part in a region
            std::size_t region_workers(region_slots slots) const noexcept;

            // whether the calling thread takes part in a region
            bool caller_takes_part(region_slots const slots) const noexcept
            {
                return caller_slot_ != static_cast<std::size_t>(-1) &&
                    share_index(slots, caller_slot_) < slots.count;
            }

            // the index of the share of the given slot in a region
            std::size_t share_index(
                region_slots slots, std::size_t slot) const noexcept
            {
                return (slot + num_threads_ - slots.first) % num_threads_;
            }

            void publish_region(region_slots slots) noexcept;
            void wake_workers(region_slots slots, std::size_t first) noexcept;
            bool wait_for_region(
                std::uint64_t& seen, std::size_t position) noexcept;
            void finish_share() noexcept;
            void wait_for_workers() noexcept;
            void init_threads(char const* annotation);
            void run_locked_region(region_function_type* f, void* body,
                std::size_t size, std::size_t first, std::size_t count);
        };

        // The shared data of a shared_suspending_fork_join_executor: the one
        // instance for the pool of the calling thread and the given
        // configuration, created on first use. Returns nullptr if the
        // calling thread is not an HPX thread, or if it can't be created.
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT
            std::shared_ptr<suspending_fork_join_shared_data>
            suspending_fork_join_shared_instance(
                threads::thread_priority priority,
                threads::thread_stacksize stacksize,
                fork_join_executor::loop_schedule sched,
                std::chrono::nanoseconds spin_duration,
                char const* annotation) noexcept;

        // all PUs of the pool of the calling thread (or the default pool)
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::threads::mask_type
        suspending_fork_join_default_pu_mask();
    }    // namespace detail

    /// \brief An executor with fork-join (blocking) semantics whose waiting
    ///        threads suspend instead of spinning.
    ///
    /// The suspending_fork_join_executor does the same job as the
    /// fork_join_executor: it runs each parallel region on a fixed set of
    /// worker threads, one per processing unit (PU), and the calling thread
    /// takes part in the region if it runs on one of these PUs. Its waiting
    /// threads (the workers between two regions and the caller at the end of
    /// a region) spin for a short time only (the spin duration), and then
    /// suspend until they are woken up. Other work can run on their cores
    /// meanwhile, and idle cores can go to sleep.
    ///
    /// The executor is constexpr default constructible. It creates its worker
    /// threads on first use, on the pool of the thread that uses it first (or
    /// the default pool). Copies have reference semantics: copying an
    /// executor on an HPX thread creates the worker threads first if needed,
    /// and the copy shares them. Copies of an executor constructed with a PU
    /// mask always share its worker threads. Copies of a default constructed
    /// executor made before the HPX runtime runs (or on a thread that is not
    /// an HPX thread) share nothing with the original until it is used; each
    /// of them creates its own worker threads on first use. The worker
    /// threads exit when the last copy that shares them is destroyed, or when
    /// the runtime starts to shut down (in hpx::finalize, see
    /// hpx::register_pre_shutdown_function), whichever
    /// comes first: suspended threads would keep the runtime from stopping.
    /// Using the executor after its worker threads exited throws
    /// invalid_status.
    ///
    /// The calling thread takes part in the parallel regions if, when the
    /// worker threads are created, it is a worker thread of the same pool
    /// running on a PU in the executor's PU mask. Otherwise all PUs get a
    /// worker thread, and the calling thread suspends until the region has
    /// finished. Scheduling work through the executor concurrently from
    /// different threads, or from inside one of its own parallel regions, is
    /// not supported. shared_suspending_fork_join_executor lifts these
    /// restrictions.
    ///
    /// \tparam Shared true for a shared_suspending_fork_join_executor, false
    ///         for a suspending_fork_join_executor.
    HPX_CXX_CORE_EXPORT template <bool Shared>
    class basic_suspending_fork_join_executor
    {
    public:
        /// Type of loop schedule, see fork_join_executor::loop_schedule.
        using loop_schedule = fork_join_executor::loop_schedule;

        /// \cond NOINTERNAL
        using execution_category = hpx::execution::parallel_execution_tag;
        using executor_parameters_type =
            hpx::execution::experimental::default_parameters;
        /// \endcond

        /// The default time the waiting threads spin before they suspend.
        static constexpr std::chrono::nanoseconds default_spin_duration =
            std::chrono::microseconds(20);

    private:
        /// \cond NOINTERNAL

        using shared_data = detail::suspending_fork_join_shared_data;

        template <typename Body>
        static void call_region(
            void* body, shared_data& data, std::size_t const index) noexcept
        {
            data.run_share(index, *static_cast<Body const*>(body));
        }

        template <std::size_t... Is, typename F, typename A, typename Tuple>
        static decltype(auto) invoke_element(
            hpx::util::index_pack<Is...>, F& f, A&& a, Tuple& t)
        {
            return HPX_INVOKE(f, HPX_FORWARD(A, a), hpx::get<Is>(t)...);
        }

        // Create the shared data and the worker threads, if needed.
        shared_data& data() const
        {
            if (!shared_data_.get())
            {
                shared_data_.assign(std::make_shared<shared_data>(
                    worker_priority(), worker_stacksize(), schedule_,
                    spin_duration_, nullptr));
            }

            shared_data& data = *shared_data_.get();
            if (!data.started() && data.start(annotation()))
            {
                // The worker threads would keep the runtime from stopping if
                // it doesn't stop them: it is too late to start them if this
                // fails.
                hpx::detail::try_catch_exception_ptr(
                    [&] {
                        shared_data::stop_on_shutdown(shared_data_.shared());
                    },
                    [&](std::exception_ptr&& ep) {
                        data.stop_workers();
                        std::rethrow_exception(HPX_MOVE(ep));
                    });
            }
            return data;
        }

        // The shared data a copy of this executor refers to. A shared
        // executor never holds any (see shared_instance).
        std::shared_ptr<shared_data> data_for_copy() const
        {
            if constexpr (!Shared)
            {
                if (!shared_data_.get() &&
                    threads::get_self_id_data() != nullptr)
                {
                    (void) data();
                }
            }
            return shared_data_.shared();
        }

        // The shared data of a shared executor: the one instance for the
        // pool of the calling thread and the configuration of this executor.
        // Returns nullptr if the calling thread is not an HPX thread, or if
        // it can't be created.
        std::shared_ptr<shared_data> shared_instance() const noexcept
        {
            return detail::suspending_fork_join_shared_instance(
                worker_priority(), worker_stacksize(), schedule_,
                spin_duration_, annotation());
        }

        template <typename Body>
        void run_region(Body& body, std::size_t const size) const
        {
            // A hint that tasks must not be combined says that the iterations
            // depend on each other: each one needs a task of its own, which
            // the parallel_executor gives them.
            if (hpx::threads::do_not_combine_tasks(
                    experimental::get_hint(parallel_exec_).sharing_mode()))
            {
                run_independent_region(body, size);
                return;
            }

            if constexpr (Shared)
            {
                // Use the shared instance if it is free, run the region
                // through a parallel_executor otherwise.
                if (auto const data = shared_instance(); data &&
                    data->try_execute_region(&call_region<Body>, &body, size,
                        region_first_core(), region_num_cores()))
                {
                    return;
                }
                run_fallback_region(body, size);
            }
            else
            {
                data().execute_region(&call_region<Body>, &body, size,
                    region_first_core(), region_num_cores());
            }
        }

        // the slots of the regions, see parallel_exec_
        std::size_t region_first_core() const
        {
            return experimental::get_first_core(parallel_exec_);
        }

        std::size_t region_num_cores() const
        {
            return experimental::processing_units_count(parallel_exec_);
        }

        // Run a region through the parallel_executor (see parallel_exec_),
        // one task for each index.
        template <typename Body>
        void run_independent_region(Body& body, std::size_t const size) const
        {
            hpx::parallel::execution::bulk_sync_execute(
                hpx::execution::to_hierarchical_spawning(parallel_exec_),
                [body](std::size_t const i) mutable { body(i, i + 1); },
                hpx::util::counting_shape(size));
        }

        // Run a region through the parallel_executor (see parallel_exec_):
        // one task for each of its cores, each with a contiguous range of
        // indices.
        template <typename Body>
        void run_fallback_region(Body& body, std::size_t const size) const
        {
            if (size == 0)
            {
                return;
            }

            std::size_t const num_chunks = (std::min) (size,
                hpx::execution::experimental::processing_units_count(
                    parallel_exec_));
            hpx::parallel::execution::bulk_sync_execute(
                parallel_exec_,
                [body, size, num_chunks](std::size_t const chunk) mutable {
                    body((chunk * size) / num_chunks,
                        ((chunk + 1) * size) / num_chunks);
                },
                hpx::util::counting_shape(num_chunks));
        }

        template <typename... Fs>
        void sync_invoke_impl(Fs&&... fs) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            char const* const annotation = this->annotation();
            hpx::scoped_annotation annotate(annotation ?
                    annotation :
                    "suspending_fork_join_executor::sync_invoke");
#endif
            auto function_pack = hpx::forward_as_tuple(HPX_FORWARD(Fs, fs)...);
            auto body = [&](std::size_t first, std::size_t const last) {
                for (/**/; first != last; ++first)
                {
                    hpx::visit([](auto&& f) { HPX_INVOKE(f); },
                        hpx::detail::runtime_get(function_pack, first));
                }
            };
            run_region(body, sizeof...(Fs));
        }

        static hpx::threads::mask_type default_pu_mask()
        {
            return detail::suspending_fork_join_default_pu_mask();
        }
        /// \endcond

    public:
        /// \cond NOINTERNAL
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            HPX_TRACING_MARK_EVENT(
                "suspending_fork_join_executor::bulk_sync_execute");

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            char const* const annotation = this->annotation();
            hpx::scoped_annotation annotate(annotation ?
                    annotation :
                    "suspending_fork_join_executor::bulk_sync_execute");
#endif
            using result_type =
                hpx::parallel::execution::detail::bulk_execute_result_t<F, S,
                    Ts...>;
            using index_pack_type = hpx::util::make_index_pack_t<sizeof...(Ts)>;

            auto argument_pack = hpx::forward_as_tuple(HPX_FORWARD(Ts, ts)...);
            auto const first = hpx::util::begin(shape);
            using difference_type = typename std::iterator_traits<
                std::decay_t<decltype(first)>>::difference_type;

            std::size_t const size = hpx::util::size(shape);
            if constexpr (std::is_void_v<result_type>)
            {
                auto body = [&, f = f](
                                std::size_t i, std::size_t const last) mutable {
                    auto it = std::next(first, static_cast<difference_type>(i));
                    for (/**/; i != last; ++i, ++it)
                    {
                        invoke_element(
                            index_pack_type{}, f, *it, argument_pack);
                    }
                };
                run_region(body, size);
            }
            else
            {
                result_type results(size);
                auto body = [&, f = f](
                                std::size_t i, std::size_t const last) mutable {
                    auto it = std::next(first, static_cast<difference_type>(i));
                    for (/**/; i != last; ++i, ++it)
                    {
                        results[i] = invoke_element(
                            index_pack_type{}, f, *it, argument_pack);
                    }
                };
                run_region(body, size);
                return results;
            }
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            using result_type =
                hpx::parallel::execution::detail::bulk_execute_result_t<F, S,
                    Ts...>;

            // Forward to the synchronous version: this thread takes part in
            // the region, so the future can be made ready only at its end.
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if constexpr (std::is_void_v<result_type>)
                    {
                        bulk_sync_execute(
                            HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
                        return hpx::make_ready_future();
                    }
                    else
                    {
                        auto&& result = bulk_sync_execute(
                            HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
                        return hpx::make_ready_future(HPX_MOVE(result));
                    }
                },
                [&](std::exception_ptr&& ep) {
                    return hpx::make_exceptional_future<result_type>(
                        HPX_MOVE(ep));
                });
        }

        template <typename F, typename... Fs>
            requires(std::invocable<F> && (std::invocable<Fs> && ...))
        void sync_invoke(F&& f, Fs&&... fs) const
        {
            sync_invoke_impl(HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        }

        template <typename F, typename... Fs>
            requires(std::invocable<F> && (std::invocable<Fs> && ...))
        hpx::future<void> async_invoke(F&& f, Fs&&... fs) const
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    sync_invoke_impl(HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
                    return hpx::make_ready_future();
                },
                [&](std::exception_ptr&& ep) {
                    return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
                });
        }

        bool operator==(
            basic_suspending_fork_join_executor const& rhs) const noexcept
        {
            if (schedule_ != rhs.schedule_ ||
                spin_duration_ != rhs.spin_duration_ ||
                parallel_exec_ != rhs.parallel_exec_ ||
                experimental::get_first_core(parallel_exec_) !=
                    experimental::get_first_core(rhs.parallel_exec_) ||
                experimental::processing_units_count(parallel_exec_) !=
                    experimental::processing_units_count(rhs.parallel_exec_))
            {
                return false;
            }
            auto const* lhs_mask = given_pu_mask();
            auto const* rhs_mask = rhs.given_pu_mask();
            if (lhs_mask == rhs_mask)
            {
                return true;
            }
            return lhs_mask && rhs_mask && *lhs_mask == *rhs_mask;
        }

        bool operator!=(
            basic_suspending_fork_join_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        [[nodiscard]] basic_suspending_fork_join_executor const& context()
            const noexcept
        {
            return *this;
        }
        /// \endcond

        /// \brief Construct an executor with the default configuration.
        constexpr basic_suspending_fork_join_executor() noexcept
          : basic_suspending_fork_join_executor(
                threads::thread_priority::default_)
        {
        }

        /// \brief Construct an executor that uses all PUs of the pool. The
        ///        worker threads are created on first use.
        ///
        /// \param priority  The priority of the worker threads and of the
        ///        tasks of the embedded parallel_executor. The worker threads
        ///        get thread_priority::bound if it is
        ///        thread_priority::default_, so that they stay on their
        ///        cores.
        /// \param stacksize The stacksize of the worker threads. It can't be
        ///        thread_stacksize::nostack, as the worker threads suspend.
        /// \param sched     The loop schedule of the parallel regions.
        /// \param spin_duration The time a waiting thread spins before it
        ///        suspends.
        explicit constexpr basic_suspending_fork_join_executor(
            threads::thread_priority priority,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::small_,
            loop_schedule sched = loop_schedule::dynamic,
            std::chrono::nanoseconds spin_duration =
                default_spin_duration) noexcept
          : spin_duration_(spin_duration)
          , schedule_(sched)
          , parallel_exec_(priority, stacksize)
        {
        }

        /// \brief Construct a suspending_fork_join_executor that uses the
        ///        given PUs. The worker threads are created on first use.
        ///
        /// \param pu_mask   The PUs to use for the worker threads.
        /// \param priority  The priority of the worker threads (see
        ///        above).
        /// \param stacksize The stacksize of the worker threads. It can't be
        ///        thread_stacksize::nostack, as the worker threads suspend.
        /// \param sched     The loop schedule of the parallel regions.
        /// \param spin_duration The time a waiting thread spins before it
        ///        suspends.
        explicit basic_suspending_fork_join_executor(
            hpx::threads::mask_cref_type pu_mask,
            threads::thread_priority priority =
                threads::thread_priority::default_,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::small_,
            loop_schedule sched = loop_schedule::dynamic,
            std::chrono::nanoseconds spin_duration = default_spin_duration)
            requires(!Shared)
          : spin_duration_(spin_duration)
          , schedule_(sched)
          , parallel_exec_(priority, stacksize)
        {
            shared_data_.assign(std::make_shared<shared_data>(
                worker_priority(), stacksize, sched, spin_duration, &pu_mask));
        }

        /// \brief Copy an executor. Copying on an HPX thread creates the
        ///        worker threads of \a rhs first, if needed, and the copy
        ///        shares them.
        constexpr basic_suspending_fork_join_executor(
            basic_suspending_fork_join_executor const& rhs)
          : spin_duration_(rhs.spin_duration_)
          , schedule_(rhs.schedule_)
          , parallel_exec_(rhs.parallel_exec_)
        {
            if (!std::is_constant_evaluated())
            {
                copy_data_from(rhs);
            }
        }

        basic_suspending_fork_join_executor(
            basic_suspending_fork_join_executor&&) noexcept = default;

        constexpr basic_suspending_fork_join_executor& operator=(
            basic_suspending_fork_join_executor const& rhs)
        {
            if (this != &rhs)
            {
                if (std::is_constant_evaluated())
                {
                    shared_data_.reset();
                }
                else
                {
                    copy_data_from(rhs);
                }
                spin_duration_ = rhs.spin_duration_;
                schedule_ = rhs.schedule_;
                parallel_exec_ = rhs.parallel_exec_;
            }
            return *this;
        }

        basic_suspending_fork_join_executor& operator=(
            basic_suspending_fork_join_executor&&) noexcept = default;

        constexpr ~basic_suspending_fork_join_executor() = default;

        /// \cond NOINTERNAL
        // The properties apply to the parallel_executor as well (see
        // parallel_exec_).
        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_annotation_t, char const* annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.set_annotation(annotation);
            return exec_with_annotation;
        }

        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_annotation_t, std::string annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.set_annotation(
                hpx::detail::store_function_annotation(HPX_MOVE(annotation)));
            return exec_with_annotation;
        }

        [[nodiscard]] char const* query(
            experimental::get_annotation_t) const noexcept
        {
            return annotation();
        }

        [[nodiscard]] hpx::threads::mask_type query(
            experimental::get_processing_units_mask_t) const
        {
            if (auto const* data = shared_data_.get();
                data && (data->started() || data->has_pu_mask_))
            {
                return data->pu_mask_;
            }
            return default_pu_mask();
        }

        [[nodiscard]] hpx::threads::mask_type query(
            experimental::get_cores_mask_t) const
        {
            return query(experimental::get_processing_units_mask_t{});
        }

        // the PU of the first slot of the regions
        [[nodiscard]] std::size_t query(experimental::get_first_core_t) const
        {
            auto const mask =
                query(experimental::get_processing_units_mask_t{});
            std::size_t const count = hpx::threads::count(mask);
            if (count == 0)
            {
                return 0;
            }

            std::size_t slot = region_first_core() % count;
            auto const size = hpx::threads::mask_size(mask);
            for (std::size_t i = 0; i != size; ++i)
            {
                if (hpx::threads::test(mask, i) && slot-- == 0)
                    return i;
            }
            return 0;
        }

        // The regions run on num_cores worker threads (or on all of them if
        // num_cores is 0 or larger than their number), the others are not
        // woken.
        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_processing_units_count_t tag,
            std::size_t const num_cores) const
        {
            auto exec = *this;
            exec.parallel_exec_ = tag(parallel_exec_, num_cores);
            return exec;
        }

        // The regions start at the given slot (wrapping around). The slots
        // are the PUs of the executor, in the order of the worker threads of
        // the pool: for an executor on all PUs of the pool, first_core is the
        // number of a worker thread of the pool.
        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_first_core_t tag,
            std::size_t const first_core) const
        {
            auto exec = *this;
            exec.parallel_exec_ = tag(parallel_exec_, first_core);
            return exec;
        }

        template <typename Parameters>
            requires(hpx::executor_parameters<Parameters>)
        [[nodiscard]] std::size_t query(experimental::processing_units_count_t,
            Parameters&& params,
            hpx::chrono::steady_duration const& iter_dur =
                hpx::chrono::null_duration,
            std::size_t num_tasks = 0) const
        {
            using exec_type = std::decay_t<decltype(*this)>;
            if constexpr (requires(std::decay_t<Parameters> const& p,
                              exec_type const& e,
                              hpx::chrono::steady_duration const& d) {
                              p.processing_units_count(e, d, std::size_t{});
                          })
            {
                return HPX_FORWARD(Parameters, params)
                    .processing_units_count(*this, iter_dur, num_tasks);
            }
            else
            {
                std::size_t const num_threads = [&] {
                    if (auto const* data = shared_data_.get();
                        data && data->started())
                    {
                        return data->num_threads_;
                    }
                    return hpx::threads::count(
                        query(experimental::get_processing_units_mask_t{}));
                }();
                return (std::min) (region_num_cores(), num_threads);
            }
        }

        /// P2300 get_scheduler bridge
        hpx::execution::experimental::executor_scheduler<
            basic_suspending_fork_join_executor>
        query(hpx::execution::experimental::get_scheduler_t) const noexcept
        {
            // The scheduler shares the worker threads, unless they can't be
            // created now; it then creates its own on first use.
            hpx::detail::try_catch_exception_ptr(
                [&] { (void) data_for_copy(); }, [](std::exception_ptr&&) {});
            return hpx::execution::experimental::executor_scheduler<
                basic_suspending_fork_join_executor>(
                basic_suspending_fork_join_executor(*this, no_init));
        }
        /// \endcond

        [[nodiscard]] threads::thread_priority query(
            experimental::get_priority_t tag) const noexcept
        {
            return tag(parallel_exec_);
        }

        [[nodiscard]] threads::thread_stacksize query(
            experimental::get_stacksize_t tag) const noexcept
        {
            return tag(parallel_exec_);
        }

        /// \cond NOINTERNAL
        // A shared executor runs single tasks on the parallel_executor of
        // this executor, which has its properties.
        template <typename F, typename... Ts>
            requires(Shared)
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return embedded_parallel_executor().sync_execute(
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
            requires(Shared)
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return embedded_parallel_executor().async_execute(
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
            requires(Shared)
        decltype(auto) then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            return embedded_parallel_executor().then_execute(HPX_FORWARD(F, f),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
            requires(Shared)
        void post(F&& f, Ts&&... ts) const
        {
            embedded_parallel_executor().post(
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // The executor of the task policy that to_task maps a policy with
        // this executor to: its parallel_executor. A task policy runs an
        // algorithm asynchronously, which the worker threads of this
        // executor can't do.
        hpx::execution::parallel_executor to_task() const
            requires(Shared)
        {
            return embedded_parallel_executor();
        }

        // The executor of the policy that to_non_par maps a policy with this
        // executor to: the one of its parallel_executor.
        decltype(auto) to_non_par() const
            requires(Shared)
        {
            return embedded_parallel_executor().to_non_par();
        }

        // A continuation runs its region once the predecessor is ready.
        template <typename F, typename S, typename Future, typename... Ts>
            requires(Shared && !std::is_integral_v<S>)
        decltype(auto) bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            return embedded_parallel_executor().bulk_then_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Future, predecessor),
                HPX_FORWARD(Ts, ts)...);
        }

        // The priority and the stacksize of the worker threads select the
        // shared instance, as the other parameters of the constructor do. A
        // suspending_fork_join_executor may have created its worker threads
        // already, so it doesn't support them.
        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_priority_t,
            threads::thread_priority const priority) const
            requires(Shared)
        {
            auto exec = *this;
            exec.set_priority(priority);
            return exec;
        }

        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_stacksize_t,
            threads::thread_stacksize const stacksize) const
            requires(Shared)
        {
            auto exec = *this;
            exec.set_stacksize(stacksize);
            return exec;
        }

        // A hint applies to the parallel_executor only: the worker threads
        // have fixed places (see run_region).
        [[nodiscard]] basic_suspending_fork_join_executor query(
            experimental::with_hint_t,
            threads::thread_schedule_hint const hint) const
        {
            auto exec = *this;
            exec.set_hint(hint);
            return exec;
        }

        [[nodiscard]] threads::thread_schedule_hint query(
            experimental::get_hint_t tag) const
        {
            return tag(parallel_exec_);
        }

        // The parallel_executor that runs single tasks, continuations and the
        // regions that fall back, with the properties of this executor.
        [[nodiscard]] constexpr hpx::execution::parallel_executor const&
        embedded_parallel_executor() const noexcept
        {
            return parallel_exec_;
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        // The configuration is kept in the parallel_executor (see
        // parallel_exec_). A shared executor holds no worker threads: its
        // configuration selects the shared instance (see shared_instance).
        void set_priority(threads::thread_priority const priority)
        {
            parallel_exec_ =
                experimental::with_priority(parallel_exec_, priority);
        }

        void set_stacksize(threads::thread_stacksize const stacksize)
        {
            parallel_exec_ =
                experimental::with_stacksize(parallel_exec_, stacksize);
        }

        // the worker threads don't use a hint, their places are fixed
        void set_hint(threads::thread_schedule_hint const hint)
        {
            parallel_exec_ = experimental::with_hint(parallel_exec_, hint);
        }

        // The annotation is kept in the parallel_executor, it is used only
        // if HPX_HAVE_THREAD_DESCRIPTION is defined.
        void set_annotation([[maybe_unused]] char const* annotation)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            parallel_exec_ =
                experimental::with_annotation(parallel_exec_, annotation);
#endif
        }

        [[nodiscard]] char const* annotation() const noexcept
        {
            return experimental::get_annotation(parallel_exec_);
        }

        // A shared executor can be sent to another locality (for instance as
        // the executor of a policy): its configuration selects a shared
        // instance there. It never holds worker threads.
        friend class hpx::serialization::access;

        template <typename Archive>
            requires(Shared)
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            auto spin = static_cast<std::int64_t>(spin_duration_.count());
            ar & schedule_ & spin & parallel_exec_;
            spin_duration_ = std::chrono::nanoseconds(spin);
        }

        void copy_data_from(basic_suspending_fork_join_executor const& rhs)
        {
            if (auto data = rhs.data_for_copy())
            {
                shared_data_.assign(HPX_MOVE(data));
            }
            else
            {
                shared_data_.reset();
            }
        }

        // a copy that never creates the worker threads of the original
        enum class no_init_t : std::uint8_t
        {
            no_init
        };
        static constexpr no_init_t no_init = no_init_t::no_init;

        basic_suspending_fork_join_executor(
            basic_suspending_fork_join_executor const& rhs, no_init_t) noexcept
          : shared_data_(rhs.shared_data_)
          , spin_duration_(rhs.spin_duration_)
          , schedule_(rhs.schedule_)
          , parallel_exec_(rhs.parallel_exec_)
        {
        }

        // The priority of the worker threads: bound if the priority is the
        // default one. Tasks with bound priority couldn't be stolen, the
        // worker threads have to stay on their cores.
        threads::thread_priority worker_priority() const noexcept
        {
            auto const priority = experimental::get_priority(parallel_exec_);
            return priority == threads::thread_priority::default_ ?
                threads::thread_priority::bound :
                priority;
        }

        threads::thread_stacksize worker_stacksize() const noexcept
        {
            return experimental::get_stacksize(parallel_exec_);
        }

        // the PU mask given to the constructor, if any
        hpx::threads::mask_type const* given_pu_mask() const noexcept
        {
            auto const* data = shared_data_.get();
            return data && data->has_pu_mask_ ? &data->pu_mask_ : nullptr;
        }

        // Created by the constructor that takes a PU mask, or on first use
        // (see data()), so that the executor can be constexpr default
        // constructed and destructed.
        mutable hpx::util::lazy_shared_ptr<shared_data> shared_data_;

        std::chrono::nanoseconds spin_duration_ = default_spin_duration;
        loop_schedule schedule_ = loop_schedule::dynamic;

        // The rest of the configuration: the priority and the stacksize of
        // the worker threads (see worker_priority), and the cores of the
        // regions: they run on processing_units_count(parallel_exec_) worker
        // threads, starting at slot get_first_core(parallel_exec_) (wrapping
        // around). The slots are the PUs of the mask, in the order of the
        // pool's worker threads. A shared executor runs single tasks,
        // continuations and the regions its worker threads can't run (see
        // run_fallback_region) on this executor, and maps a policy to a task
        // policy with it.
        hpx::execution::parallel_executor parallel_exec_{
            threads::thread_priority::default_,
            threads::thread_stacksize::small_};
    };

    /// A suspending_fork_join_executor: one user thread at a time (see
    /// shared_suspending_fork_join_executor in
    /// hpx/executors/shared_suspending_fork_join_executor.hpp for one that
    /// any number of threads can use).
    HPX_CXX_CORE_EXPORT using suspending_fork_join_executor =
        basic_suspending_fork_join_executor<false>;

    /// \cond NOINTERNAL
    template <bool Shared>
    struct is_never_blocking_one_way_executor<
        basic_suspending_fork_join_executor<Shared>> : std::true_type
    {
    };

    template <bool Shared>
    struct is_bulk_one_way_executor<basic_suspending_fork_join_executor<Shared>>
      : std::true_type
    {
    };

    template <bool Shared>
    struct is_bulk_two_way_executor<basic_suspending_fork_join_executor<Shared>>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental

#include <hpx/config/warnings_suffix.hpp>
