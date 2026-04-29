//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution_base/stdexec_forward.hpp>

#include <hpx/assert.hpp>
#include <hpx/concepts/concepts.hpp>
#include <hpx/concurrency/cache_line_data.hpp>
#include <hpx/concurrency/detail/non_contiguous_index_queue.hpp>
#include <hpx/coroutines/thread_enums.hpp>
#include <hpx/datastructures/tuple.hpp>
#include <hpx/datastructures/variant.hpp>
#include <hpx/errors/exception.hpp>
#include <hpx/errors/exception_list.hpp>
#include <hpx/errors/try_catch_exception_ptr.hpp>
#include <hpx/execution/algorithms/bulk.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/execution_base/completion_scheduler.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/receiver.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/executors/thread_pool_scheduler.hpp>
#include <hpx/functional/bind_front.hpp>
#include <hpx/functional/detail/tag_fallback_invoke.hpp>
#include <hpx/functional/tag_invoke.hpp>
#include <hpx/iterator_support/counting_iterator.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/iterator_support/traits/is_range.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Compute a chunk size given a number of worker threads and a total number
    // of items. Returns a power-of-2 chunk size that results in at least 8
    // chunks per worker thread and at least 4 chunks per worker thread.
    // If items < num_threads, returns a power-of-2 chunk
    // size that results in at most 8 and
    // at least 4 chunks per worker thread.
    HPX_CXX_CORE_EXPORT constexpr std::uint32_t get_bulk_scheduler_chunk_size(
        std::uint32_t const num_threads, std::size_t const n) noexcept
    {
        std::uint64_t chunk_size = 1;
        while (chunk_size * num_threads * 8 < n)
        {
            chunk_size *= 2;
        }
        return static_cast<std::uint32_t>(chunk_size);
    }

    // For bulk_unchunked: f(index, ...)
    HPX_CXX_CORE_EXPORT template <std::size_t... Is, typename F, typename T,
        typename Ts>
    constexpr void bulk_scheduler_invoke_helper(
        hpx::util::index_pack<Is...>, F&& f, T&& t, Ts& ts)
    {
        HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(T, t), hpx::get<Is>(ts)...);
    }

    // For bulk_chunked: f(start, end, ...)
    HPX_CXX_CORE_EXPORT template <std::size_t... Is, typename F, typename Start,
        typename End, typename Ts>
    constexpr void bulk_scheduler_invoke_helper_chunked(
        hpx::util::index_pack<Is...>, F&& f, Start&& start, End&& end, Ts& ts)
    {
        HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Start, start),
            HPX_FORWARD(End, end), hpx::get<Is>(ts)...);
    }

    HPX_CXX_CORE_EXPORT inline hpx::threads::mask_type full_mask(
        std::size_t first_thread, std::size_t num_threads)
    {
        auto const& rp = hpx::resource::get_partitioner();

        std::size_t const overall_threads =
            hpx::threads::hardware_concurrency();
        auto mask = hpx::threads::mask_type();
        hpx::threads::resize(mask, overall_threads);
        for (std::size_t i = 0; i != num_threads; ++i)
        {
            auto thread_mask = rp.get_pu_mask(i + first_thread);
            for (std::size_t j = 0; j != overall_threads; ++j)
            {
                if (threads::test(thread_mask, j))
                {
                    threads::set(mask, j);
                }
            }
        }
        return mask;
    }

    HPX_CXX_CORE_EXPORT inline hpx::threads::mask_type limit_mask(
        hpx::threads::mask_cref_type orgmask, std::size_t num_threads)
    {
        std::size_t const num_cores = hpx::threads::hardware_concurrency();

        auto mask = hpx::threads::mask_type();
        hpx::threads::resize(mask, num_cores);
        for (std::size_t i = 0, j = 0; i != num_threads && j != num_cores; ++j)
        {
            if (hpx::threads::test(orgmask, j))
            {
                hpx::threads::set(mask, j);
                ++i;
            }
        }
        return mask;
    }

    HPX_CXX_CORE_EXPORT template <typename OperationState>
    struct task_function;

    HPX_CXX_CORE_EXPORT template <typename OperationState>
    struct set_value_loop_visitor
    {
        OperationState* const op_state;
        task_function<OperationState> const* const task_f;

        [[noreturn]] void operator()(hpx::monostate) const noexcept
        {
            HPX_UNREACHABLE;
        }

    private:
        // Perform the work in one element indexed by index. The index
        // represents a range of indices (iterators) in the given shape.
        template <typename Ts>
        void do_work_chunk(Ts& ts, std::uint32_t const index) const
        {
#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
            static hpx::util::itt::event notify_event(
                "set_value_loop_visitor::do_work_chunk(chunking)");

            hpx::util::itt::mark_event e(notify_event);
#endif

            using index_pack_type = hpx::detail::fused_index_pack_t<Ts>;

            auto const i_begin =
                static_cast<std::size_t>(index) * task_f->chunk_size;
            auto const i_end =
                (std::min) (i_begin + task_f->chunk_size, task_f->size);

            if constexpr (OperationState::is_chunked)
            {
                // bulk_chunked: f(start, end, values...)
                bulk_scheduler_invoke_helper_chunked(
                    index_pack_type{}, op_state->f, i_begin, i_end, ts);
            }
            else
            {
                // bulk_unchunked: f(index, values...) for each element
                // In unchunked case, chunk_size is 1
                // so each chunk will only have one element.
                // The index used for invocation is i_begin.
                bulk_scheduler_invoke_helper(
                    index_pack_type{}, op_state->f, i_begin, ts);
            }
        }

        template <hpx::concurrency::detail::queue_end Which, typename Ts>
        void do_work(Ts& ts) const
        {
            auto worker_thread = task_f->worker_thread;
            auto& local_queue = op_state->queues[worker_thread].data_;

            // Handle local queue first
            hpx::optional<std::uint32_t> index;
            while ((index = local_queue.template pop<Which>()))
            {
                do_work_chunk(ts, *index);
            }

            if (task_f->allow_stealing)
            {
                // Then steal from the opposite end of the neighboring queues
                static constexpr auto opposite_end =
                    hpx::concurrency::detail::opposite_end_v<Which>;

                for (std::uint32_t offset = 1;
                    offset != op_state->num_worker_threads; ++offset)
                {
                    std::size_t neighbor_thread =
                        (worker_thread + offset) % op_state->num_worker_threads;
                    auto& neighbor_queue =
                        op_state->queues[neighbor_thread].data_;

                    while (
                        (index = neighbor_queue.template pop<opposite_end>()))
                    {
                        do_work_chunk(ts, *index);
                    }
                }
            }
        }

    public:
        // Visit the values sent from the predecessor sender. This function
        // first tries to handle all chunks in the queue owned by worker_thread.
        // It then tries to steal chunks from neighboring threads.
        //
        template <typename Ts>
            requires(!std::same_as<std::decay_t<Ts>, hpx::monostate>)
        void operator()(Ts& ts) const
        {
            // schedule chunks from the end, if needed
            if (task_f->reverse_placement)
            {
                do_work<hpx::concurrency::detail::queue_end::right>(ts);
            }
            else
            {
                do_work<hpx::concurrency::detail::queue_end::left>(ts);
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename OperationState>
    struct set_value_end_loop_visitor
    {
        OperationState* const op_state;

        [[noreturn]] void operator()(hpx::monostate) const noexcept
        {
            HPX_UNREACHABLE;
        }

        // Visit the values sent from the predecessor sender. This function is
        // called once all worker threads have processed their chunks and the
        // connected receiver should be signaled.
        //
        template <typename Ts>
            requires(!std::same_as<std::decay_t<Ts>, hpx::monostate>)
        void operator()(Ts&& ts) const
        {
            hpx::invoke_fused(
                hpx::bind_front(hpx::execution::experimental::set_value,
                    HPX_MOVE(op_state->receiver)),
                HPX_FORWARD(Ts, ts));
        }
    };

    // This struct encapsulates the work done by one worker thread.
    HPX_CXX_CORE_EXPORT template <typename OperationState>
    struct task_function
    {
        OperationState* const op_state;
        std::size_t const size;
        std::uint32_t const chunk_size;
        std::uint32_t const worker_thread;
        bool reverse_placement;
        bool allow_stealing;

        // Visit the values sent by the predecessor sender.
        void do_work() const
        {
            auto visitor =
                set_value_loop_visitor<OperationState>{op_state, this};
            hpx::visit(HPX_MOVE(visitor), op_state->ts);
        }

        // Store an exception and mark that an exception was thrown in the
        // operation state. This function assumes that there is a current
        // exception.
        template <typename Exception>
        void store_exception(Exception e) const
        {
            // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
            op_state->exceptions.add(HPX_MOVE(e));
        }

        // Finish the work for one worker thread. If this is not the last worker
        // thread to finish, it will only decrement the counter. If it is the
        // last thread it will call set_error if there is an exception.
        // Otherwise, it will call set_value on the connected receiver.
        void finish() const
        {
            if (--(op_state->tasks_remaining.data_) == 0)
            {
                if (op_state->bad_alloc_thrown.load(std::memory_order_relaxed))
                {
                    try
                    {
                        throw std::bad_alloc();
                    }
                    catch (...)
                    {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(op_state->receiver),
                            std::current_exception());
                    }
                }
                else if (op_state->exceptions.size() != 0)
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(op_state->receiver),
                        hpx::detail::construct_lightweight_exception(
                            HPX_MOVE(op_state->exceptions)));
                }
                else
                {
                    auto visitor =
                        set_value_end_loop_visitor<OperationState>{op_state};
                    hpx::visit(HPX_MOVE(visitor), HPX_MOVE(op_state->ts));
                }
            }
        }

        // Entry point for the worker thread. It will attempt to do its local
        // work, catch any exceptions, and then call set_value or set_error on
        // the connected receiver.
        void operator()() const
        {
            try
            {
                do_work();
            }
            catch (std::bad_alloc const&)
            {
                op_state->bad_alloc_thrown = true;
            }
            catch (...)
            {
                store_exception(std::current_exception());
            }

            finish();
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename OperationState, typename F,
        typename Shape>
    struct bulk_receiver
    {
        using receiver_concept = hpx::execution::experimental::receiver_t;
        OperationState* op_state;

        template <typename Receiver, typename E>
            requires std::same_as<std::remove_cvref_t<Receiver>, bulk_receiver>
        friend void tag_invoke(hpx::execution::experimental::set_error_t,
            Receiver&& r, E&& e) noexcept
        {
            hpx::execution::experimental::set_error(
                HPX_MOVE(r.op_state->receiver), HPX_FORWARD(E, e));
        }

        template <typename Receiver>
            requires std::same_as<std::remove_cvref_t<Receiver>, bulk_receiver>
        friend void tag_invoke(
            hpx::execution::experimental::set_stopped_t, Receiver&& r) noexcept
        {
            hpx::execution::experimental::set_stopped(
                HPX_MOVE(r.op_state->receiver));
        }

        // Initialize a queue for a worker thread.
        void init_queue_depth_first(std::size_t const worker_thread,
            std::size_t const size, std::size_t num_threads) noexcept
        {
            auto& queue = op_state->queues[worker_thread].data_;
            auto const part_begin = (worker_thread * size) / num_threads;
            auto const part_end = ((worker_thread + 1) * size) / num_threads;

            HPX_ASSERT_MSG(
                size <= static_cast<std::size_t>(
                            (std::numeric_limits<std::uint32_t>::max)()),
                "init_queue_depth_first: ranges larger than "
                "UINT32_MAX are not supported");

            queue.reset(static_cast<std::uint32_t>(part_begin),
                static_cast<std::uint32_t>(part_end));
        }

        void init_queue_breadth_first(std::size_t const worker_thread,
            std::size_t const size, std::size_t num_threads) noexcept
        {
            auto& queue = op_state->queues[worker_thread].data_;
            auto const num_steps = size / num_threads + 1;
            auto const part_begin = worker_thread;
            auto part_end = (std::min) (size + num_threads - 1,
                part_begin + num_steps * num_threads);
            auto const remainder = (part_end - part_begin) % num_threads;
            if (remainder != 0)
            {
                part_end -= remainder;
            }

            HPX_ASSERT_MSG(
                size <= static_cast<std::size_t>(
                            (std::numeric_limits<std::uint32_t>::max)()),
                "init_queue_breadth_first: ranges larger than "
                "UINT32_MAX are not supported");

            queue.reset(static_cast<std::uint32_t>(part_begin),
                static_cast<std::uint32_t>(part_end),
                static_cast<std::uint32_t>(num_threads));
        }

        // Spawn a task which will process a number of chunks. If the queue
        // contains no chunks no task will be spawned.
        template <typename Task>
        void do_work_task(Task&& task_f) const
        {
            std::uint32_t const worker_thread = task_f.worker_thread;
            auto& queue = op_state->queues[worker_thread].data_;
            if (queue.empty())
            {
                // If the queue is empty we don't spawn a task. We only signal
                // that this "task" is ready.
                task_f.finish();
                return;
            }

            auto hint =
                hpx::execution::experimental::get_hint(op_state->scheduler);
            if (hint.mode == hpx::threads::thread_schedule_hint_mode::none &&
                hint.hint == -1)
            {
                // apply hint if none was given
                hint.schedule_hint(worker_thread + op_state->first_thread);

                auto policy = hpx::execution::experimental::with_hint(
                    op_state->scheduler.policy(), hint);

                op_state->scheduler.execute(HPX_FORWARD(Task, task_f), policy);
            }
            else
            {
                op_state->scheduler.execute(HPX_FORWARD(Task, task_f));
            }
        }

        // Do the work on the worker thread that called set_value from the
        // predecessor sender. This thread participates in the work and does not
        // need a new task since it already runs on a task.
        template <typename Task>
        void do_work_local(Task&& task_f) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            if (char const* scheduler_annotation =
                    hpx::execution::experimental::get_annotation(
                        op_state->scheduler))
            {
                hpx::scoped_annotation ann(scheduler_annotation);
                task_f();
            }
            else
#endif
            {
                hpx::scoped_annotation ann(op_state->f);
                task_f();
            }
        }

        using range_value_type =
            hpx::traits::iter_value_t<std::ranges::iterator_t<Shape>>;

        template <typename... Ts>
        void execute(Ts&&... ts)
        {
            // Don't spawn tasks if there is no work to be done
            auto const size =
                static_cast<std::uint32_t>(hpx::util::size(op_state->shape));
            if (size == 0)
            {
                hpx::execution::experimental::set_value(
                    HPX_MOVE(op_state->receiver), HPX_FORWARD(Ts, ts)...);
                return;
            }

            // Calculate chunk size based on execution mode
            std::uint32_t chunk_size;
            std::uint32_t num_chunks;
            if constexpr (OperationState::is_chunked)
            {
                chunk_size = get_bulk_scheduler_chunk_size(
                    op_state->num_worker_threads, size);
                num_chunks = (size + chunk_size - 1) / chunk_size;
            }
            else
            {
                chunk_size = 1;
                num_chunks = size;
            }

            // launch only as many tasks as we have chunks
            std::size_t const num_pus = op_state->num_worker_threads;
            if (num_chunks <
                static_cast<std::uint32_t>(op_state->num_worker_threads))
            {
                op_state->num_worker_threads = num_chunks;
                op_state->tasks_remaining.data_ = num_chunks;
                op_state->pu_mask =
                    detail::limit_mask(op_state->pu_mask, num_chunks);
            }

            HPX_ASSERT(hpx::threads::count(op_state->pu_mask) ==
                op_state->num_worker_threads);

            // Store sent values in the operation state
            op_state->ts.template emplace<hpx::tuple<Ts...>>(
                HPX_FORWARD(Ts, ts)...);

            // thread placement
            hpx::threads::thread_schedule_hint const hint =
                hpx::execution::experimental::get_hint(op_state->scheduler);

            using placement = hpx::threads::thread_placement_hint;

            // Initialize the queues for all worker threads so that worker
            // threads can start stealing immediately when they start.
            for (std::uint32_t worker_thread = 0;
                worker_thread != op_state->num_worker_threads; ++worker_thread)
            {
                if (hint.placement_mode() == placement::breadth_first ||
                    hint.placement_mode() == placement::breadth_first_reverse)
                {
                    init_queue_breadth_first(worker_thread, num_chunks,
                        op_state->num_worker_threads);
                }
                else
                {
                    // the default for this scheduler is depth-first placement
                    init_queue_depth_first(worker_thread, num_chunks,
                        op_state->num_worker_threads);
                }
            }

            // Spawn the worker threads for all except the local queue.
            auto const& rp = hpx::resource::get_partitioner();

            auto local_worker_thread =
                static_cast<std::uint32_t>(hpx::get_local_worker_thread_num());

            std::uint32_t worker_thread = 0;
            bool main_thread_ok = false;
            std::size_t main_pu_num = rp.get_pu_num(local_worker_thread);
            if (!hpx::threads::test(op_state->pu_mask, main_pu_num) ||
                op_state->num_worker_threads == 1)
            {
                main_thread_ok = true;
                local_worker_thread = worker_thread++;
                main_pu_num =
                    rp.get_pu_num(local_worker_thread + op_state->first_thread);
            }

            bool reverse_placement =
                hint.placement_mode() == placement::depth_first_reverse ||
                hint.placement_mode() == placement::breadth_first_reverse;
            bool allow_stealing =
                !hpx::threads::do_not_share_function(hint.sharing_mode());

            for (std::uint32_t pu = 0;
                worker_thread != op_state->num_worker_threads && pu != num_pus;
                ++pu)
            {
                std::size_t pu_num = rp.get_pu_num(pu + op_state->first_thread);

                // The queue for the local thread is handled later inline.
                if (!main_thread_ok && pu == local_worker_thread)
                {
                    // the initializing thread is expected to participate in
                    // evaluating parallel regions
                    HPX_ASSERT(hpx::threads::test(op_state->pu_mask, pu_num));
                    main_thread_ok = true;
                    local_worker_thread = worker_thread++;
                    main_pu_num = rp.get_pu_num(
                        local_worker_thread + op_state->first_thread);
                    continue;
                }

                // don't double-book core that runs main thread
                if (main_thread_ok && main_pu_num == pu_num)
                {
                    continue;
                }

                // create an HPX thread only for cores in the given PU-mask
                if (!hpx::threads::test(op_state->pu_mask, pu_num))
                {
                    continue;
                }

                // Schedule task for this worker thread
                do_work_task(
                    task_function<OperationState>{op_state, size, chunk_size,
                        worker_thread, reverse_placement, allow_stealing});

                ++worker_thread;
            }

            // there have to be as many HPX threads as there are set bits in
            // the PU-mask
            HPX_ASSERT(worker_thread == op_state->num_worker_threads);

            // Handle the queue for the local thread.
            if (main_thread_ok)
            {
                do_work_local(task_function<OperationState>{this->op_state,
                    size, chunk_size, local_worker_thread, reverse_placement,
                    allow_stealing});
            }
        }

        template <typename Receiver, typename... Ts>
            requires std::same_as<std::remove_cvref_t<Receiver>, bulk_receiver>
        friend void tag_invoke(hpx::execution::experimental::set_value_t,
            Receiver&& r, Ts&&... ts) noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() { r.execute(HPX_FORWARD(Ts, ts)...); },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(r.op_state->receiver), HPX_MOVE(ep));
                });
        }
    };

    // This sender represents bulk work that will be performed using the
    // thread_pool_scheduler.
    //
    // The work is chunked into a number of chunks larger than the number of
    // worker threads available on the underlying thread pool. The chunks are
    // then assigned to worker thread-specific thread-safe index queues. One HPX
    // thread is spawned for each underlying worker (OS) thread. The HPX thread
    // is responsible for work in one queue. If the queue is empty, no HPX
    // thread will be spawned. Once the HPX thread has finished working on its
    // own queue, it will attempt to steal work from other queues.
    //
    // Since predecessor sender must complete on an HPX thread (the completion
    // scheduler is a thread_pool_scheduler; otherwise the customization defined
    // in this file is not chosen) it will be reused as one of the worker
    // threads.
    //
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Sender,
        typename Shape, typename F, bool IsChunked = false>
    class thread_pool_bulk_sender
    {
    private:
        thread_pool_policy_scheduler<Policy> scheduler;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;
        hpx::threads::mask_type pu_mask;

    public:
        template <typename Sender_, typename Shape_, typename F_>
        thread_pool_bulk_sender(thread_pool_policy_scheduler<Policy> scheduler,
            Sender_&& sender, Shape_&& shape, F_&& f,
            hpx::threads::mask_rvref_type pu_mask)
          : scheduler(HPX_MOVE(scheduler))
          , sender(HPX_FORWARD(Sender_, sender))
          , shape(HPX_FORWARD(Shape_, shape))
          , f(HPX_FORWARD(F_, f))
          , pu_mask(HPX_MOVE(pu_mask))
        {
        }

        template <typename Sender_, typename Shape_, typename F_>
        thread_pool_bulk_sender(thread_pool_policy_scheduler<Policy> sched,
            Sender_&& sender, Shape_&& shape, F_&& f)
          : scheduler(HPX_MOVE(sched))
          , sender(HPX_FORWARD(Sender_, sender))
          , shape(HPX_FORWARD(Shape_, shape))
          , f(HPX_FORWARD(F_, f))
          , pu_mask(detail::full_mask(
                hpx::execution::experimental::get_first_core(scheduler),
                hpx::execution::experimental::processing_units_count(
                    hpx::execution::experimental::null_parameters, scheduler,
                    hpx::chrono::null_duration, 0)))
        {
        }

        thread_pool_bulk_sender(thread_pool_bulk_sender&&) = default;
        thread_pool_bulk_sender(thread_pool_bulk_sender const&) = default;
        thread_pool_bulk_sender& operator=(thread_pool_bulk_sender&&) = default;
        thread_pool_bulk_sender& operator=(
            thread_pool_bulk_sender const&) = default;

        using sender_concept = hpx::execution::experimental::sender_t;

        template <typename Env>
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        friend auto tag_invoke(
            hpx::execution::experimental::get_completion_signatures_t,
            thread_pool_bulk_sender const&, Env const&)
            -> hpx::execution::experimental::transform_completion_signatures_of<
                Sender, Env,
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>>;
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif

        struct env
        {
            std::decay_t<Sender> const& pred_snd;
            thread_pool_policy_scheduler<Policy> const& sch;

            template <typename CPO>
                requires(meta::value<meta::one_of<CPO,
                        hpx::execution::experimental::set_value_t>>)
            friend constexpr auto query(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>,
                env const& e, auto&&...) noexcept
            {
                return e.sch;
            }

            template <typename CPO>
                requires(meta::value<meta::one_of<CPO,
                             hpx::execution::experimental::set_error_t,
                             hpx::execution::experimental::set_stopped_t>> &&
                    hpx::execution::experimental::detail::
                        has_completion_scheduler_v<CPO, std::decay_t<Sender>>)
            friend constexpr auto query(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>
                    tag,
                env const& e, auto&&...) noexcept
            {
                return tag(hpx::execution::experimental::get_env(e.pred_snd));
            }

            // P3826R5: report the completion domain for this bulk sender
            template <typename CPO>
            auto query(stdexec::get_completion_domain_t<CPO>) const noexcept
            {
                return sch.query(stdexec::get_completion_domain_t<CPO>{});
            }
        };

        // It may be also be correct to forward the entire env of the
        // pred. sender.
        friend constexpr auto tag_invoke(
            hpx::execution::experimental::get_env_t,
            thread_pool_bulk_sender const& s) noexcept
        {
            return env{s.sender, s.scheduler};
        }

    private:
        template <typename Receiver>
        struct operation_state
        {
            static constexpr bool is_chunked = IsChunked;

            using operation_state_type =
                hpx::execution::experimental::connect_result_t<Sender,
                    bulk_receiver<operation_state, F, Shape>>;

            thread_pool_policy_scheduler<Policy> scheduler;
            operation_state_type op_state;
            std::size_t first_thread;
            std::size_t num_worker_threads;
            hpx::threads::mask_type pu_mask;
            std::vector<hpx::util::cache_aligned_data<
                hpx::concurrency::detail::non_contiguous_index_queue<>>>
                queues;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
            hpx::util::cache_aligned_data<std::atomic<std::size_t>>
                tasks_remaining;

            using value_types = value_types_of_t<Sender,
                hpx::execution::experimental::empty_env, decayed_tuple,
                hpx::variant>;
            hpx::util::detail::prepend_t<value_types, hpx::monostate> ts;
            std::atomic<bool> bad_alloc_thrown{false};
            hpx::exception_list exceptions;

            template <typename Scheduler_, typename Sender_, typename Shape_,
                typename F_, typename Receiver_>
            operation_state(Scheduler_&& scheduler, Sender_&& sender,
                Shape_&& shape, F_&& f, hpx::threads::mask_type pumask,
                Receiver_&& receiver)
              : scheduler(HPX_FORWARD(Scheduler_, scheduler))
              , op_state(hpx::execution::experimental::connect(
                    HPX_FORWARD(Sender_, sender),
                    bulk_receiver<operation_state, F, Shape>{this}))
              , first_thread(
                    hpx::execution::experimental::get_first_core(scheduler))
              , num_worker_threads(
                    hpx::execution::experimental::processing_units_count(
                        hpx::execution::experimental::null_parameters,
                        scheduler, hpx::chrono::null_duration, 0))
              , pu_mask(HPX_MOVE(pumask))
              , queues(num_worker_threads)
              , shape(HPX_FORWARD(Shape_, shape))
              , f(HPX_FORWARD(F_, f))
              , receiver(HPX_FORWARD(Receiver_, receiver))
            {
                tasks_remaining.data_.store(
                    num_worker_threads, std::memory_order_relaxed);
                HPX_ASSERT(hpx::threads::count(pu_mask) == num_worker_threads);
            }

            friend void tag_invoke(start_t, operation_state& os) noexcept
            {
                hpx::execution::experimental::start(os.op_state);
            }
        };

    public:
        template <typename Receiver>
        friend auto tag_invoke(
            connect_t, thread_pool_bulk_sender&& s, Receiver&& receiver)
        {
            return operation_state<std::decay_t<Receiver>>{
                HPX_MOVE(s.scheduler), HPX_MOVE(s.sender), HPX_MOVE(s.shape),
                HPX_MOVE(s.f), HPX_MOVE(s.pu_mask),
                HPX_FORWARD(Receiver, receiver)};
        }

        template <typename Receiver>
        friend auto tag_invoke(
            connect_t, thread_pool_bulk_sender& s, Receiver&& receiver)
        {
            return operation_state<std::decay_t<Receiver>>{s.scheduler,
                s.sender, s.shape, s.f, s.pu_mask,
                HPX_FORWARD(Receiver, receiver)};
        }
    };
}    // namespace hpx::execution::experimental::detail

// Note: With stdexec integration, bulk operations are now customized
// through the domain system in thread_pool_scheduler.hpp rather than
// direct tag_invoke customizations. The thread_pool_domain<Policy>
// intercepts hpx::execution::experimental::bulk_chunked_t operations and creates
// thread_pool_bulk_sender instances for parallel execution.
