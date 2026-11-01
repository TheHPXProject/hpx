//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/thread_pool_scheduler.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/thread_support.hpp>
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
#include <memory>
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

    // For bulk_chunked: create exactly num_threads large chunks (one per worker).
    // Unlike get_bulk_scheduler_chunk_size which creates ~8x more chunks per
    // thread for fine-grained work stealing, this variant maximises spatial
    // locality and minimises work-stealing queue overhead for the chunked case.
    // Work stealing is still attempted but rarely needed for uniform workloads.
    HPX_CXX_CORE_EXPORT constexpr std::uint32_t
    get_bulk_scheduler_chunk_size_chunked(
        std::uint32_t const num_threads, std::size_t const n) noexcept
    {
        if (num_threads == 0)
            return static_cast<std::uint32_t>(n);
        // ceiling division: ceil(n / num_threads) -> one chunk per worker thread
        return static_cast<std::uint32_t>(
            (n + static_cast<std::size_t>(num_threads) - 1) / num_threads);
    }

    /// Round a chunk up to a multiple of 16 when it is
    /// smaller than size
    HPX_CXX_CORE_EXPORT constexpr std::uint32_t align_chunk_for_vectorization(
        std::uint32_t chunk, std::uint32_t const size) noexcept
    {
        constexpr std::uint32_t g = 16;
        if (chunk == 0 || chunk >= size)
            return chunk;
        std::uint64_t c = chunk;
        if (c % g != 0)
        {
            c = ((c + g - 1) / g) * g;
        }
        if (c > size)
            c = size;
        return static_cast<std::uint32_t>(c);
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

    // The part of a bulk operation that its worker tasks touch before they
    // have taken a chunk: the index queues and the number of chunks not done
    // yet. The operation completes once all of its chunks are done, and its
    // operation state may be destroyed right after that, while a worker task
    // that started late is still running. The tasks therefore share this
    // block, and touch the operation state only after they have taken a chunk.
    HPX_CXX_CORE_EXPORT struct bulk_work
    {
        explicit bulk_work(std::size_t const num_queues)
          : queues(num_queues)
        {
        }

        std::atomic<std::uint32_t>& chunks_remaining() noexcept
        {
            return counters_.data_.chunks_remaining;
        }

        // Set the number of references while the operation state holds the
        // only one, so that the worker tasks can adopt theirs without
        // touching the counters again.
        void set_references(std::size_t const count) noexcept
        {
            counters_.data_.references.store(
                static_cast<long>(count), std::memory_order_release);
        }

        // Adopt one of the references set by set_references.
        hpx::intrusive_ptr<bulk_work> adopt_reference() noexcept
        {
            return hpx::intrusive_ptr<bulk_work>(this, false);
        }

    private:
        // The two counters that every worker task changes: the chunks not
        // done yet and the references to this block. They share a cache line,
        // which the padding keeps apart from the members below, which the
        // tasks only read.
        struct counters
        {
            std::atomic<std::uint32_t> chunks_remaining{0};
            hpx::util::atomic_count references{1};
        };
        hpx::util::cache_aligned_data<counters> counters_;

    public:
        std::vector<hpx::util::cache_aligned_data<
            hpx::concurrency::detail::non_contiguous_index_queue<>>>
            queues;
        std::uint32_t num_worker_threads = 0;
        bool reverse_placement = false;
        bool allow_stealing = false;

    private:
        friend void intrusive_ptr_add_ref(bulk_work* p) noexcept
        {
            p->counters_.data_.references.increment();
        }

        friend void intrusive_ptr_release(bulk_work* p) noexcept
        {
            if (p->counters_.data_.references.decrement() == 0)
            {
                // The thread that decrements the reference count to zero must
                // perform an acquire to ensure that it doesn't start
                // destructing the object until all previous writes have
                // drained.
                std::atomic_thread_fence(std::memory_order_acquire);

                delete p;
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename OperationState>
    struct task_function;

    HPX_CXX_CORE_EXPORT template <typename OperationState,
        hpx::concurrency::detail::queue_end Which>
    struct set_value_loop_visitor
    {
        OperationState* const op_state;
        task_function<OperationState> const* const task_f;
        std::uint32_t const first_chunk;
        std::uint32_t& done;

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
            using index_pack_type = hpx::detail::fused_index_pack_t<Ts>;

            HPX_TRACING_MARK_EVENT(
                "set_value_loop_visitor::do_work_chunk(chunking)");

            auto const i_begin =
                static_cast<std::size_t>(index) * op_state->chunk_size;
            auto const i_end = (std::min) (i_begin + op_state->chunk_size,
                static_cast<std::size_t>(op_state->size));

            if constexpr (OperationState::is_chunked)
            {
                // bulk_chunked: f(start, end, values...)
                bulk_scheduler_invoke_helper_chunked(
                    index_pack_type{}, op_state->f, i_begin, i_end, ts);
            }
            else
            {
                // bulk_unchunked: one element call f(shape_index, values...) per i.
                auto it = std::ranges::next(
                    hpx::util::begin(op_state->shape), i_begin);
                for (auto i = i_begin; i < i_end; ++i, ++it)
                {
                    bulk_scheduler_invoke_helper(
                        index_pack_type{}, op_state->f, *it, ts);
                }
            }
        }

    public:
        // Visit the values sent from the predecessor sender. This function
        // runs the chunk the worker has already taken, then all chunks in the
        // queue owned by worker_thread. It then tries to steal chunks from
        // neighboring threads. Each chunk is counted in done before it runs.
        //
        template <typename Ts>
            requires(!std::same_as<std::decay_t<Ts>, hpx::monostate>)
        void operator()(Ts& ts) const
        {
            do_work_chunk(ts, first_chunk);

            bulk_work& work = *task_f->work;
            auto const worker_thread = task_f->worker_thread;
            auto& local_queue = work.queues[worker_thread].data_;

            // Handle local queue first
            hpx::optional<std::uint32_t> index;
            while ((index = local_queue.template pop<Which>()))
            {
                ++done;
                do_work_chunk(ts, *index);
            }

            if (work.allow_stealing)
            {
                // Then steal from the opposite end of the neighboring queues
                static constexpr auto opposite_end =
                    hpx::concurrency::detail::opposite_end_v<Which>;

                for (std::uint32_t offset = 1;
                    offset != work.num_worker_threads; ++offset)
                {
                    std::size_t neighbor_thread =
                        (worker_thread + offset) % work.num_worker_threads;
                    auto& neighbor_queue = work.queues[neighbor_thread].data_;

                    while (
                        (index = neighbor_queue.template pop<opposite_end>()))
                    {
                        ++done;
                        do_work_chunk(ts, *index);
                    }
                }
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
        // not const, so that the task can be moved without touching the
        // reference count
        hpx::intrusive_ptr<bulk_work> work;
        std::uint32_t const worker_thread;

        // Take a chunk from the local queue, or else from the opposite end of
        // one of the neighboring queues.
        template <hpx::concurrency::detail::queue_end Which>
        hpx::optional<std::uint32_t> take_chunk() const
        {
            if (auto index =
                    work->queues[worker_thread].data_.template pop<Which>())
            {
                return index;
            }

            if (work->allow_stealing)
            {
                constexpr auto opposite_end =
                    hpx::concurrency::detail::opposite_end_v<Which>;

                auto const num_threads = work->num_worker_threads;
                for (std::uint32_t offset = 1; offset != num_threads; ++offset)
                {
                    std::size_t neighbor_thread = worker_thread + offset;
                    if (neighbor_thread >= num_threads)
                        neighbor_thread -= num_threads;

                    if (auto index = work->queues[neighbor_thread]
                            .data_.template pop<opposite_end>())
                    {
                        return index;
                    }
                }
            }
            return {};
        }

        // Visit the values sent by the predecessor sender. Each chunk taken
        // from a queue is counted in done before it runs.
        template <hpx::concurrency::detail::queue_end Which>
        void do_work(std::uint32_t& done) const
        {
            // A worker that starts after all chunks are done has nothing to do.
            if (work->chunks_remaining().load(std::memory_order_acquire) == 0)
            {
                return;
            }

            // Take a chunk before touching the operation state: the operation
            // can't complete before the chunk is counted in finish, so the
            // operation state is alive while the chunks run. A worker that
            // finds no chunk doesn't touch it.
            auto const first = take_chunk<Which>();
            if (!first)
            {
                return;
            }
            ++done;

            auto visitor = set_value_loop_visitor<OperationState, Which>{
                op_state, this, *first, done};
            hpx::visit(HPX_MOVE(visitor), op_state->ts);
        }

        // Store an exception and mark that an exception was thrown in the
        // operation state. This function assumes that there is a current
        // exception. Storing allocates: if that fails, the operation reports
        // bad_alloc instead.
        template <typename Exception>
        void store_exception(Exception e) const noexcept
        {
            try
            {
                // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
                op_state->exceptions.add(HPX_MOVE(e));
            }
            catch (...)
            {
                op_state->bad_alloc_thrown = true;
            }
        }

        // After an exception, the chunks left in the worker's own queue are
        // not run, only counted in done. The other workers go on with their
        // chunks and may store exceptions of their own.
        template <hpx::concurrency::detail::queue_end Which>
        void skip_work(std::uint32_t& done) const noexcept
        {
            auto& local_queue = work->queues[worker_thread].data_;
            while (local_queue.template pop<Which>())
            {
                ++done;
            }
        }

        void skip_own_work(std::uint32_t& done) const noexcept
        {
            using hpx::concurrency::detail::queue_end;
            if (work->reverse_placement)
            {
                skip_work<queue_end::right>(done);
            }
            else
            {
                skip_work<queue_end::left>(done);
            }
        }

        // Do the work of one worker thread, adding the chunks it takes to
        // done. An exception can only come from a chunk, so done is not zero
        // when one is stored.
        void run(std::uint32_t& done) const
        {
            using hpx::concurrency::detail::queue_end;
            try
            {
                // schedule chunks from the end, if needed
                if (work->reverse_placement)
                {
                    do_work<queue_end::right>(done);
                }
                else
                {
                    do_work<queue_end::left>(done);
                }
            }
            catch (std::bad_alloc const&)
            {
                HPX_ASSERT(done != 0);
                op_state->bad_alloc_thrown = true;
                skip_own_work(done);
            }
            catch (...)
            {
                HPX_ASSERT(done != 0);
                store_exception(std::current_exception());
                skip_own_work(done);
            }
        }

        // Finish the work for one worker thread, which has taken done chunks.
        // The operation completes once all of its chunks are done, so it
        // doesn't wait for a worker thread that started only after the other
        // workers took all of its chunks: that thread finds nothing to do. The
        // worker that finishes the last chunks calls set_error if there is an
        // exception, otherwise set_value on the connected receiver. The other
        // workers must not touch the operation state after counting their
        // chunks, as it may be destroyed as soon as the receiver completes.
        void finish(std::uint32_t const done) const
        {
            if (done != 0 &&
                work->chunks_remaining().fetch_sub(
                    done, std::memory_order_acq_rel) == done)
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
        // work, catch any exceptions, and then count its chunks, calling
        // set_value or set_error on the connected receiver if they were the
        // last ones.
        void operator()() const
        {
            std::uint32_t done = 0;
            run(done);
            finish(done);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename OperationState, typename F,
        typename Shape>
    struct bulk_receiver
    {
        using receiver_concept = hpx::execution::experimental::receiver_t;
        OperationState* op_state;

        template <typename E>
        void set_error(E&& e) & noexcept
        {
            hpx::execution::experimental::set_error(
                HPX_MOVE(op_state->receiver), HPX_FORWARD(E, e));
        }

        template <typename E>
        void set_error(E&& e) && noexcept
        {
            static_cast<bulk_receiver&>(*this).set_error(HPX_FORWARD(E, e));
        }

        void set_stopped() & noexcept
        {
            hpx::execution::experimental::set_stopped(
                HPX_MOVE(op_state->receiver));
        }

        void set_stopped() && noexcept
        {
            static_cast<bulk_receiver&>(*this).set_stopped();
        }

        // Initialize a queue for a worker thread.
        void init_queue_depth_first(std::size_t const worker_thread,
            std::size_t const size, std::size_t num_threads) noexcept
        {
            auto& queue = op_state->work->queues[worker_thread].data_;
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
            auto& queue = op_state->work->queues[worker_thread].data_;
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
            auto& queue = op_state->work->queues[worker_thread].data_;
            if (queue.empty())
            {
                // If the queue is empty we don't spawn a task: it has no chunks
                // to count.
                return;
            }

            auto hint =
                hpx::execution::experimental::get_hint(op_state->scheduler);
            if (hint.mode() == hpx::threads::thread_schedule_hint_mode::none &&
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
        // need a new task since it already runs on a task. The chunks it takes
        // are added to done, and counted by the caller.
        template <typename Task>
        void do_work_local(Task const& task_f, std::uint32_t& done) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            if (char const* scheduler_annotation =
                    hpx::execution::experimental::get_annotation(
                        op_state->scheduler))
            {
                hpx::scoped_annotation ann(scheduler_annotation);
                task_f.run(done);
            }
            else
#endif
            {
                hpx::scoped_annotation ann(op_state->f);
                task_f.run(done);
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

            // Calculate chunk size based on execution mode and sequential policy
            std::uint32_t chunk_size;
            std::uint32_t num_chunks;

            // For sequential policy: single chunk covering entire range
            if constexpr (!OperationState::is_parallel)
            {
                if constexpr (OperationState::is_chunked)
                {
                    chunk_size = size;
                    num_chunks = 1;
                }
                else
                {
                    chunk_size = 1;
                    num_chunks = size;
                }
                op_state->num_worker_threads = 1;
            }
            else if constexpr (OperationState::is_chunked)
            {
                // One large chunk per worker thread: minimises queue overhead
                // and maximises locality for memory-bound work.
                chunk_size = get_bulk_scheduler_chunk_size_chunked(
                    op_state->num_worker_threads, size);
                num_chunks = (size + chunk_size - 1) / chunk_size;
            }
            else
            {
                chunk_size = get_bulk_scheduler_chunk_size(
                    op_state->num_worker_threads, size);
                num_chunks = (size + chunk_size - 1) / chunk_size;
            }

            if constexpr (OperationState::is_unsequenced &&
                OperationState::is_parallel)
            {
                chunk_size = align_chunk_for_vectorization(chunk_size, size);
                num_chunks = (size + chunk_size - 1) / chunk_size;
            }

            // launch only as many tasks as we have chunks
            std::size_t const num_pus = op_state->num_worker_threads;
            if constexpr (!OperationState::is_parallel)
            {
                // Sequential: force single task execution
                op_state->pu_mask = detail::limit_mask(op_state->pu_mask, 1);
            }
            else if (num_chunks <
                static_cast<std::uint32_t>(op_state->num_worker_threads))
            {
                op_state->num_worker_threads = num_chunks;
                op_state->pu_mask =
                    detail::limit_mask(op_state->pu_mask, num_chunks);
            }

            op_state->size = size;
            op_state->chunk_size = chunk_size;

            // The operation completes once all chunks are done. This thread
            // holds one more count until it has spawned the worker tasks and
            // done its own share, as it uses the operation state until then.
            bulk_work& work = *op_state->work;
            work.num_worker_threads =
                static_cast<std::uint32_t>(op_state->num_worker_threads);
            work.chunks_remaining().store(
                num_chunks + 1, std::memory_order_relaxed);

            // One reference for the operation state and one for the task of
            // each worker thread, this one included. The tasks adopt them, so
            // that spawning a task doesn't touch the counters while the
            // spawned tasks use them.
            work.set_references(1 + op_state->num_worker_threads);

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

            work.reverse_placement =
                hint.placement_mode() == placement::depth_first_reverse ||
                hint.placement_mode() == placement::breadth_first_reverse;
            work.allow_stealing =
                !hpx::threads::do_not_share_function(hint.sharing_mode());

            // The chunks this thread takes or skips, plus the count it holds.
            std::uint32_t done = 1;

            // The references adopted by the worker tasks, this thread's included.
            std::size_t adopted = 0;

            try
            {
                spawn_tasks(worker_thread, local_worker_thread, main_thread_ok,
                    main_pu_num, num_pus, adopted);
            }
            catch (...)
            {
                // The workers from worker_thread on got no task (including the
                // local thread, if it wasn't numbered yet). Count the chunks of
                // their queues as skipped, so the operation completes with the
                // exception once the spawned tasks are done.
                store_current_exception();
                for (std::uint32_t w = worker_thread;
                    w != op_state->num_worker_threads; ++w)
                {
                    auto& queue = work.queues[w].data_;
                    while (queue.template pop<
                        hpx::concurrency::detail::queue_end::left>())
                    {
                        ++done;
                    }
                }
            }

            // Handle the queue for the local thread.
            task_function<OperationState> const local_task{
                op_state, work.adopt_reference(), local_worker_thread};
            ++adopted;

            // Correct the references if not every worker thread got a task:
            // after an exception, the ones not adopted are returned, and if
            // this thread had no queue of its own, it takes one more.
            for (; adopted < op_state->num_worker_threads; ++adopted)
            {
                intrusive_ptr_release(&work);
            }
            if (adopted > op_state->num_worker_threads)
            {
                intrusive_ptr_add_ref(&work);
            }
            if (main_thread_ok)
            {
                try
                {
                    do_work_local(local_task, done);
                }
                catch (...)
                {
                    // only setting up the annotation can throw here
                    store_current_exception();
                    local_task.skip_own_work(done);
                }
            }
            local_task.finish(done);
        }

        // Spawn the worker tasks for all queues except the one of the local
        // thread, numbering the workers from worker_thread on.
        void spawn_tasks(std::uint32_t& worker_thread,
            std::uint32_t& local_worker_thread, bool& main_thread_ok,
            std::size_t& main_pu_num, std::size_t const num_pus,
            std::size_t& adopted) const
        {
            auto const& rp = hpx::resource::get_partitioner();

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

                // Schedule task for this worker thread. If it isn't spawned,
                // destroying it returns its reference.
                ++adopted;
                do_work_task(task_function<OperationState>{op_state,
                    op_state->work->adopt_reference(), worker_thread});

                ++worker_thread;
            }

            // there have to be as many HPX threads as there are set bits in
            // the PU-mask
            HPX_ASSERT(worker_thread == op_state->num_worker_threads);
        }

        // Store the current exception in the operation state.
        void store_current_exception() const noexcept
        {
            try
            {
                throw;
            }
            catch (std::bad_alloc const&)
            {
                op_state->bad_alloc_thrown = true;
            }
            catch (...)
            {
                try
                {
                    op_state->exceptions.add(std::current_exception());
                }
                catch (...)
                {
                    op_state->bad_alloc_thrown = true;
                }
            }
        }

        template <typename... Ts>
            requires((OperationState::is_chunked &&
                         std::invocable<F, range_value_type, range_value_type,
                             std::add_lvalue_reference_t<Ts>...>) ||
                (!OperationState::is_chunked &&
                    std::invocable<F, range_value_type,
                        std::add_lvalue_reference_t<Ts>...>) )
        void set_value(Ts&&... ts) & noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() { this->execute(HPX_FORWARD(Ts, ts)...); },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(op_state->receiver), HPX_MOVE(ep));
                });
        }

        template <typename... Ts>
            requires((OperationState::is_chunked &&
                         std::invocable<F, range_value_type, range_value_type,
                             std::add_lvalue_reference_t<Ts>...>) ||
                (!OperationState::is_chunked &&
                    std::invocable<F, range_value_type,
                        std::add_lvalue_reference_t<Ts>...>) )
        void set_value(Ts&&... ts) && noexcept
        {
            static_cast<bulk_receiver&>(*this).set_value(
                HPX_FORWARD(Ts, ts)...);
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
        typename Shape, typename F, bool IsChunked, bool IsParallel,
        bool IsUnsequenced>
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
                this->scheduler.query(
                    hpx::execution::experimental::get_first_core_t{}),
                this->scheduler.query(
                    hpx::execution::experimental::processing_units_count_t{},
                    hpx::execution::experimental::null_parameters,
                    hpx::chrono::null_duration, 0)))
        {
        }

        thread_pool_bulk_sender(thread_pool_bulk_sender&&) = default;
        thread_pool_bulk_sender(thread_pool_bulk_sender const&) = default;
        thread_pool_bulk_sender& operator=(thread_pool_bulk_sender&&) = default;
        thread_pool_bulk_sender& operator=(
            thread_pool_bulk_sender const&) = default;

        using sender_concept = hpx::execution::experimental::sender_t;

        template <typename Self, typename Env>
        static consteval auto get_completion_signatures() noexcept
            -> decltype(hpx::execution::experimental::
                    transform_completion_signatures(
                        hpx::execution::experimental::
                            completion_signatures_of_t<Sender, Env>{},
                        hpx::execution::experimental::keep_completion<
                            hpx::execution::experimental::set_value_t>{},
                        hpx::execution::experimental::keep_completion<
                            hpx::execution::experimental::set_error_t>{},
                        hpx::execution::experimental::keep_completion<
                            hpx::execution::experimental::set_stopped_t>{},
                        hpx::execution::experimental::completion_signatures<
                            hpx::execution::experimental::set_error_t(
                                std::exception_ptr)>{}))
        {
            return {};
        }

        struct env
        {
            std::decay_t<Sender> const& pred_snd;
            thread_pool_policy_scheduler<Policy> const& sch;

            constexpr auto query(
                hpx::execution::experimental::get_completion_scheduler_t<
                    hpx::execution::experimental::set_value_t>) const noexcept
            {
                return sch;
            }

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
            auto query(
                hpx::execution::experimental::get_completion_domain_t<CPO>)
                const noexcept
            {
                return sch.query(
                    hpx::execution::experimental::get_completion_domain_t<
                        CPO>{});
            }

            // P2300 get_allocator query
            constexpr auto query(
                hpx::execution::experimental::get_allocator_t) const noexcept
            {
                return std::allocator<std::byte>{};
            }
        };

        // It may also be correct to forward the entire env of the
        // pred. sender.
        constexpr auto get_env() const noexcept
        {
            return env{sender, scheduler};
        }

    private:
        template <typename Receiver>
        struct operation_state
        {
            static constexpr bool is_chunked = IsChunked;
            static constexpr bool is_parallel = IsParallel;
            static constexpr bool is_unsequenced = IsUnsequenced;

            using operation_state_type =
                hpx::execution::experimental::connect_result_t<Sender,
                    bulk_receiver<operation_state, F, Shape>>;

            thread_pool_policy_scheduler<Policy> scheduler;
            operation_state_type op_state;
            std::size_t first_thread;
            std::size_t num_worker_threads;
            std::size_t size = 0;
            std::uint32_t chunk_size = 0;
            hpx::threads::mask_type pu_mask;

            // shared with the worker tasks, which may outlive this
            hpx::intrusive_ptr<bulk_work> work;

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

            using value_types = value_types_of_t<Sender,
                hpx::execution::experimental::empty_env, decayed_tuple,
                hpx::variant>;
            hpx::util::detail::prepend_t<value_types, hpx::monostate> ts;
            std::atomic<bool> bad_alloc_thrown{false};
            hpx::exception_list exceptions;

            template <typename Scheduler_, typename Sender_, typename Shape_,
                typename F_, typename Receiver_>
            operation_state(Scheduler_&& scheduler_, Sender_&& sender,
                Shape_&& shape, F_&& f, hpx::threads::mask_type pumask,
                Receiver_&& receiver)
              : scheduler(HPX_FORWARD(Scheduler_, scheduler_))
              , op_state(hpx::execution::experimental::connect(
                    HPX_FORWARD(Sender_, sender),
                    bulk_receiver<operation_state, F, Shape>{this}))
              , first_thread(scheduler.query(
                    hpx::execution::experimental::get_first_core_t{}))
              , num_worker_threads(scheduler.query(
                    hpx::execution::experimental::processing_units_count_t{},
                    hpx::execution::experimental::null_parameters,
                    hpx::chrono::null_duration, 0))
              , pu_mask(HPX_MOVE(pumask))
              , work(new bulk_work(num_worker_threads), false)
              , shape(HPX_FORWARD(Shape_, shape))
              , f(HPX_FORWARD(F_, f))
              , receiver(HPX_FORWARD(Receiver_, receiver))
            {
                HPX_ASSERT(hpx::threads::count(pu_mask) == num_worker_threads);
            }

            void start() & noexcept
            {
                auto stop_token =
                    stdexec::get_stop_token(stdexec::get_env(receiver));
                if (stop_token.stop_requested())
                {
                    stdexec::set_stopped(HPX_MOVE(receiver));
                    return;
                }
                hpx::execution::experimental::start(op_state);
            }
        };

    public:
        template <typename Receiver>
        operation_state<std::decay_t<Receiver>> connect(Receiver&& receiver) &&
        {
            return operation_state<std::decay_t<Receiver>>{HPX_MOVE(scheduler),
                HPX_MOVE(sender), HPX_MOVE(shape), HPX_MOVE(f),
                HPX_MOVE(pu_mask), HPX_FORWARD(Receiver, receiver)};
        }

        template <typename Receiver>
        operation_state<std::decay_t<Receiver>> connect(Receiver&& receiver) &
        {
            return operation_state<std::decay_t<Receiver>>{scheduler, sender,
                shape, f, pu_mask, HPX_FORWARD(Receiver, receiver)};
        }
    };
}    // namespace hpx::execution::experimental::detail

// Bulk operations are customized through the domain system in
// thread_pool_scheduler.hpp. thread_pool_domain<Policy> intercepts bulk
// operations and creates thread_pool_bulk_sender instances.

// A worker's task runs its share of the elements of the shape, calling the
// bulk function for each of them (bulk_chunked passes a range of them to a
// call), so its size is derived from the size of that function.
template <typename OperationState, hpx::threads::task_size Size>
struct hpx::traits::get_task_size<
    hpx::execution::experimental::detail::task_function<OperationState>, Size>
{
    static constexpr threads::task_size value = Size;

    [[nodiscard]] static constexpr threads::task_size call(
        hpx::execution::experimental::detail::task_function<
            OperationState> const& f) noexcept
    {
        using function_type = std::decay_t<decltype(f.op_state->f)>;

        std::size_t const num_threads = f.op_state->num_worker_threads;
        std::size_t const size = f.op_state->size;
        std::size_t const calls =
            num_threads == 0 ? size : (size + num_threads - 1) / num_threads;

        threads::task_size const derived = threads::bulk_task_size(
            get_task_size<function_type>::call(f.op_state->f), calls);
        return derived == threads::task_size::unknown ? Size : derived;
    }
};
