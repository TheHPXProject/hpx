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
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::execution::experimental {

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
    /// and the copy shares them. Copies made before the HPX runtime runs (or
    /// on a thread that is not an HPX thread) share nothing with the
    /// original until it is used; each of them creates its own worker threads
    /// on first use. The worker threads exit when the last copy that shares
    /// them is destroyed, or when the runtime starts to shut down (in
    /// hpx::finalize, see hpx::register_pre_shutdown_function), whichever
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
    /// not supported.
    HPX_CXX_CORE_EXPORT class suspending_fork_join_executor
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

        // The state shared by all copies of an executor and by its worker
        // threads. It is created on first use.
        struct HPX_CORE_EXPORT shared_data
        {
            using region_function_type = void(
                void*, shared_data&, std::size_t) noexcept;
            using queue_type =
                hpx::concurrency::detail::contiguous_index_queue<std::uint32_t>;

            shared_data(threads::thread_priority priority,
                threads::thread_stacksize stacksize, loop_schedule sched,
                std::chrono::nanoseconds spin_duration,
                hpx::threads::mask_type const* pu_mask, char const* annotation);

            shared_data(shared_data const&) = delete;
            shared_data(shared_data&&) = delete;
            shared_data& operator=(shared_data const&) = delete;
            shared_data& operator=(shared_data&&) = delete;

            ~shared_data();

            // Run one parallel region of size indices: f(body, data, slot)
            // runs the share of the given slot.
            void execute_region(
                region_function_type* f, void* body, std::size_t size);

            // Run the share of the given slot of the current region, calling
            // body(first, last) for ranges of its indices.
            template <typename Body>
            void run_share(std::size_t const slot, Body& body) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&] {
                        if (dynamic_)
                        {
                            // Items are taken in chunks of 1/num_threads of
                            // the items left in a queue (but at least one),
                            // one atomic operation per chunk. The chunks
                            // shrink as a queue drains, so the other slots
                            // can still take a share of an unevenly loaded
                            // queue.
                            auto const divisor =
                                static_cast<std::uint32_t>(num_threads_);
                            hpx::optional<
                                std::pair<std::uint32_t, std::uint32_t>>
                                chunk;

                            // process the own items first
                            queue_type& local_queue = queues_[slot].data_;
                            while (
                                (chunk = local_queue.pop_left_chunk(divisor)))
                            {
                                body(static_cast<std::size_t>(chunk->first),
                                    static_cast<std::size_t>(chunk->second));
                            }

                            // then steal from the other slots
                            for (std::size_t offset = 1; offset != num_threads_;
                                ++offset)
                            {
                                queue_type& queue =
                                    queues_[(slot + offset) % num_threads_]
                                        .data_;
                                while ((chunk = queue.pop_right_chunk(divisor)))
                                {
                                    body(static_cast<std::size_t>(chunk->first),
                                        static_cast<std::size_t>(
                                            chunk->second));
                                }
                            }
                        }
                        else
                        {
                            std::size_t const size = region_size_;
                            body((slot * size) / num_threads_,
                                ((slot + 1) * size) / num_threads_);
                        }
                    },
                    [&](std::exception_ptr&& ep) {
                        store_exception(HPX_MOVE(ep));
                    });
            }

            void store_exception(std::exception_ptr&& ep) noexcept;

            // the main loop of a worker thread, position is its place in the
            // wake-up tree
            void worker_main(std::size_t slot, std::size_t position) noexcept;

            // Wake the worker threads and wait until they have exited. The
            // executor can't be used afterwards. This is done by the
            // destructor, and when the runtime starts to shut down: it can't
            // stop while the worker threads are suspended.
            void stop_workers() noexcept;

            // Stop the worker threads of the given shared data when the
            // runtime starts to shut down, if it still exists then.
            static void stop_on_shutdown(std::weak_ptr<shared_data> data);

            // the shared data registered with stop_on_shutdown
            struct shutdown_registry;

            // Members that are fixed for the lifetime of the executor.
            threads::thread_pool_base* pool_ = nullptr;
            threads::thread_priority priority_;
            threads::thread_stacksize stacksize_;
            loop_schedule schedule_;
            std::uint64_t spin_rounds_ = 0;
            std::size_t num_threads_ = 0;
            std::size_t num_workers_ = 0;
            std::size_t caller_slot_ = static_cast<std::size_t>(-1);
            hpx::threads::mask_type pu_mask_;

            // Members that change for each parallel region. They are written
            // by the calling thread before it starts the region.
            region_function_type* region_function_ = nullptr;
            void* region_body_ = nullptr;
            std::size_t region_size_ = 0;
            bool dynamic_ = false;
            bool in_region_ = false;

            hpx::spinlock exception_mtx_;
            std::exception_ptr exception_;

            // the queues for dynamic scheduling, one per slot
            std::vector<hpx::util::cache_aligned_data<queue_type>> queues_;

            // Incremented for each parallel region (and to stop the worker
            // threads).
            hpx::util::cache_aligned_data<std::atomic<std::uint64_t>> epoch_;
            std::atomic<bool> stop_ = false;

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
            void publish_region() noexcept;
            void wake_workers(std::size_t first) noexcept;
            bool wait_for_region(
                std::uint64_t seen, std::size_t position) noexcept;
            void finish_share() noexcept;
            void wait_for_workers() noexcept;
            void init_threads(char const* annotation);
        };

        template <typename Body>
        static void call_region(
            void* body, shared_data& data, std::size_t const slot) noexcept
        {
            data.run_share(slot, *static_cast<Body*>(body));
        }

        template <std::size_t... Is, typename F, typename A, typename Tuple>
        static decltype(auto) invoke_element(
            hpx::util::index_pack<Is...>, F& f, A&& a, Tuple& t)
        {
            return HPX_INVOKE(f, HPX_FORWARD(A, a), hpx::get<Is>(t)...);
        }

        // Create the shared data, if needed.
        shared_data& data() const
        {
            if (!shared_data_)
            {
                auto data = std::make_shared<shared_data>(priority_, stacksize_,
                    schedule_, spin_duration_, pu_mask_.get(), annotation_);
                shared_data::stop_on_shutdown(data);
                shared_data_ = HPX_MOVE(data);
            }
            return *shared_data_;
        }

        // The shared data a copy of this executor refers to.
        std::shared_ptr<shared_data> const& data_for_copy() const
        {
            if (!shared_data_ && threads::get_self_id_data() != nullptr)
            {
                (void) data();
            }
            return shared_data_;
        }

        template <typename Body>
        void run_region(Body& body, std::size_t const size) const
        {
            data().execute_region(&call_region<Body>, &body, size);
        }

        template <typename... Fs>
        void sync_invoke_impl(Fs&&... fs) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::scoped_annotation annotate(annotation_ ?
                    annotation_ :
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

        template <typename F, typename... Ts>
        void post(F&& f, Ts&&... ts) const
        {
            (void) async_invoke(
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        HPX_CORE_EXPORT static hpx::threads::mask_type default_pu_mask();
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
            hpx::scoped_annotation annotate(annotation_ ?
                    annotation_ :
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
                auto body = [&](std::size_t i, std::size_t const last) {
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
                auto body = [&](std::size_t i, std::size_t const last) {
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

        bool operator==(suspending_fork_join_executor const& rhs) const noexcept
        {
            if (priority_ != rhs.priority_ || stacksize_ != rhs.stacksize_ ||
                schedule_ != rhs.schedule_ ||
                spin_duration_ != rhs.spin_duration_)
            {
                return false;
            }
            if (pu_mask_ == rhs.pu_mask_)
            {
                return true;
            }
            return pu_mask_ && rhs.pu_mask_ && *pu_mask_ == *rhs.pu_mask_;
        }

        bool operator!=(suspending_fork_join_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        [[nodiscard]] suspending_fork_join_executor const& context()
            const noexcept
        {
            return *this;
        }
        /// \endcond

        /// \brief Construct a suspending_fork_join_executor that uses all PUs
        ///        of the pool. The worker threads are created on first use.
        ///
        /// \param priority  The priority of the worker threads.
        /// \param stacksize The stacksize of the worker threads. It can't be
        ///        thread_stacksize::nostack, as the worker threads suspend.
        /// \param sched     The loop schedule of the parallel regions.
        /// \param spin_duration The time a waiting thread spins before it
        ///        suspends.
        explicit constexpr suspending_fork_join_executor(
            threads::thread_priority priority = threads::thread_priority::bound,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::small_,
            loop_schedule sched = loop_schedule::dynamic,
            std::chrono::nanoseconds spin_duration =
                default_spin_duration) noexcept
          : spin_duration_(spin_duration)
          , priority_(priority)
          , stacksize_(stacksize)
          , schedule_(sched)
        {
        }

        /// \brief Construct a suspending_fork_join_executor that uses the
        ///        given PUs. The worker threads are created on first use.
        ///
        /// \param pu_mask   The PUs to use for the worker threads.
        /// \param priority  The priority of the worker threads.
        /// \param stacksize The stacksize of the worker threads. It can't be
        ///        thread_stacksize::nostack, as the worker threads suspend.
        /// \param sched     The loop schedule of the parallel regions.
        /// \param spin_duration The time a waiting thread spins before it
        ///        suspends.
        explicit suspending_fork_join_executor(
            hpx::threads::mask_cref_type pu_mask,
            threads::thread_priority priority = threads::thread_priority::bound,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::small_,
            loop_schedule sched = loop_schedule::dynamic,
            std::chrono::nanoseconds spin_duration = default_spin_duration)
          : pu_mask_(std::make_shared<hpx::threads::mask_type const>(pu_mask))
          , spin_duration_(spin_duration)
          , priority_(priority)
          , stacksize_(stacksize)
          , schedule_(sched)
        {
        }

        /// \brief Copy an executor. Copying on an HPX thread creates the
        ///        worker threads of \a rhs first, if needed, and the copy
        ///        shares them.
        suspending_fork_join_executor(suspending_fork_join_executor const& rhs)
          : shared_data_(rhs.data_for_copy())
          , pu_mask_(rhs.pu_mask_)
          , annotation_(rhs.annotation_)
          , spin_duration_(rhs.spin_duration_)
          , priority_(rhs.priority_)
          , stacksize_(rhs.stacksize_)
          , schedule_(rhs.schedule_)
        {
        }

        suspending_fork_join_executor(
            suspending_fork_join_executor&&) noexcept = default;

        suspending_fork_join_executor& operator=(
            suspending_fork_join_executor const& rhs)
        {
            if (this != &rhs)
            {
                shared_data_ = rhs.data_for_copy();
                pu_mask_ = rhs.pu_mask_;
                annotation_ = rhs.annotation_;
                spin_duration_ = rhs.spin_duration_;
                priority_ = rhs.priority_;
                stacksize_ = rhs.stacksize_;
                schedule_ = rhs.schedule_;
            }
            return *this;
        }

        suspending_fork_join_executor& operator=(
            suspending_fork_join_executor&&) noexcept = default;

        ~suspending_fork_join_executor() = default;

        /// \cond NOINTERNAL
        [[nodiscard]] suspending_fork_join_executor query(
            experimental::with_annotation_t, char const* annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.annotation_ = annotation;
            return exec_with_annotation;
        }

        [[nodiscard]] suspending_fork_join_executor query(
            experimental::with_annotation_t, std::string annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        [[nodiscard]] char const* query(
            experimental::get_annotation_t) const noexcept
        {
            return annotation_;
        }

        [[nodiscard]] hpx::threads::mask_type query(
            experimental::get_processing_units_mask_t) const
        {
            if (shared_data_)
            {
                return shared_data_->pu_mask_;
            }
            return pu_mask_ ? *pu_mask_ : default_pu_mask();
        }

        [[nodiscard]] hpx::threads::mask_type query(
            experimental::get_cores_mask_t) const
        {
            return query(experimental::get_processing_units_mask_t{});
        }

        [[nodiscard]] std::size_t query(experimental::get_first_core_t) const
        {
            auto const mask =
                query(experimental::get_processing_units_mask_t{});
            auto const size = hpx::threads::mask_size(mask);
            for (std::size_t i = 0; i != size; ++i)
            {
                if (hpx::threads::test(mask, i))
                    return i;
            }
            return 0;
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
                if (shared_data_)
                {
                    return shared_data_->num_threads_;
                }
                return hpx::threads::count(
                    query(experimental::get_processing_units_mask_t{}));
            }
        }

        /// P2300 get_scheduler bridge
        hpx::execution::experimental::executor_scheduler<
            suspending_fork_join_executor>
        query(hpx::execution::experimental::get_scheduler_t) const noexcept
        {
            // The scheduler shares the worker threads, unless they can't be
            // created now; it then creates its own on first use.
            hpx::detail::try_catch_exception_ptr(
                [&] { (void) data_for_copy(); }, [](std::exception_ptr&&) {});
            return hpx::execution::experimental::executor_scheduler<
                suspending_fork_join_executor>(
                suspending_fork_join_executor(*this, no_init));
        }
        /// \endcond

    private:
        // a copy that never creates the worker threads of the original
        enum class no_init_t : std::uint8_t
        {
            no_init
        };
        static constexpr no_init_t no_init = no_init_t::no_init;

        suspending_fork_join_executor(
            suspending_fork_join_executor const& rhs, no_init_t) noexcept
          : shared_data_(rhs.shared_data_)
          , pu_mask_(rhs.pu_mask_)
          , annotation_(rhs.annotation_)
          , spin_duration_(rhs.spin_duration_)
          , priority_(rhs.priority_)
          , stacksize_(rhs.stacksize_)
          , schedule_(rhs.schedule_)
        {
        }

        // Created on first use (see data()), so that the executor can be
        // constexpr default constructed.
        mutable std::shared_ptr<shared_data> shared_data_;

        // The PUs to use, all PUs of the pool if empty.
        std::shared_ptr<hpx::threads::mask_type const> pu_mask_;

        char const* annotation_ = nullptr;
        std::chrono::nanoseconds spin_duration_ = default_spin_duration;
        threads::thread_priority priority_ = threads::thread_priority::bound;
        threads::thread_stacksize stacksize_ =
            threads::thread_stacksize::small_;
        loop_schedule schedule_ = loop_schedule::dynamic;
    };

    /// \cond NOINTERNAL
    template <>
    struct is_never_blocking_one_way_executor<suspending_fork_join_executor>
      : std::true_type
    {
    };

    template <>
    struct is_bulk_one_way_executor<suspending_fork_join_executor>
      : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<suspending_fork_join_executor>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental

#include <hpx/config/warnings_suffix.hpp>
