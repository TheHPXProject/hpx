//  Copyright (c) 2019-2020 ETH Zurich
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/detail/bulk_invoke_helper.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <hpx/modules/memory.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parallel::execution::detail {

    ///////////////////////////////////////////////////////////////////////////
    // This struct encapsulates the work done by one worker thread.
    template <typename SharedState>
    struct task_function_result
    {
        hpx::intrusive_ptr<SharedState> const state;
        std::size_t const size;
        std::uint32_t const chunk_size;
        std::uint32_t const worker_thread;
        bool const reverse_placement;
        bool const allow_stealing;

        // Perform the work in one element indexed by index. The index
        // represents a range of indices (iterators) in the given shape.
        template <hpx::concurrency::detail::queue_end Which>
        void do_work() const
        {
            // explicitly copy function (object) and arguments
            auto f = state->f;
            auto ts = state->ts;
            auto& results = state->results;

            using index_pack_type =
                hpx::detail::fused_index_pack_t<decltype(ts)>;

            hpx::optional<std::uint32_t> index;

            // Handle local queue first
            auto& local_queue = state->queues[worker_thread].data_;
            while ((index = local_queue.template pop<Which>()))
            {
                HPX_TRACING_MARK_EVENT("do_work_chunk");

                auto const i_begin = *index * chunk_size;
                auto const i_end = (std::min) (i_begin + chunk_size,
                    static_cast<std::uint32_t>(size));

                auto it = std::next(hpx::util::begin(state->shape), i_begin);
                for (std::uint32_t i = i_begin; i != i_end; (void) ++it, ++i)
                {
                    results[i] = hpx::execution::experimental::detail::
                        bulk_invoke_helper(index_pack_type{}, f, *it, ts);
                }
            }

            if (allow_stealing)
            {
                // Then steal from the opposite end of the neighboring queues
                constexpr auto opposite_end =
                    hpx::concurrency::detail::opposite_end_v<Which>;

                auto const num_threads = state->num_threads;
                for (std::uint32_t offset = 1; offset != num_threads; ++offset)
                {
                    std::size_t neighbor_thread = worker_thread + offset;
                    if (neighbor_thread >= num_threads)
                        neighbor_thread -= num_threads;

                    auto& neighbor_queue = state->queues[neighbor_thread].data_;
                    while (
                        (index = neighbor_queue.template pop<opposite_end>()))
                    {
                        HPX_TRACING_MARK_EVENT("do_work_chunk (stealing)");

                        auto const i_begin = *index * chunk_size;
                        auto const i_end = (std::min) (i_begin + chunk_size,
                            static_cast<std::uint32_t>(size));

                        auto it =
                            std::next(hpx::util::begin(state->shape), i_begin);
                        for (std::uint32_t i = i_begin; i != i_end;
                            (void) ++it, ++i)
                        {
                            results[i] = hpx::execution::experimental::detail::
                                bulk_invoke_helper(
                                    index_pack_type{}, f, *it, ts);
                        }
                    }
                }
            }
        }

        // Store an exception and mark that an exception was thrown in the
        // operation state. This function assumes that there is a current
        // exception.
        template <typename Exception>
        void store_exception(Exception e) const
        {
            // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
            state->exceptions.add(HPX_MOVE(e));
        }

        // Finish the work for one worker thread. If this is not the last worker
        // thread to finish, it will only decrement the counter. If it is the
        // last thread it will call set_exception if there is an exception.
        // Otherwise, it will call set_value on the shared state.
        void finish() const noexcept
        {
            HPX_TRACING_MARK_EVENT("finish");

            std::uint32_t const prev_value =
                state->tasks_remaining.data_.fetch_sub(
                    1, std::memory_order_acq_rel);
            if (prev_value == 1)
            {
                if (state->bad_alloc_thrown.load(std::memory_order_relaxed))
                {
                    try
                    {
                        throw std::bad_alloc();
                    }
                    catch (...)
                    {
                        state->set_exception(std::current_exception());
                    }
                }
                else if (state->exceptions.size() != 0)
                {
                    state->set_exception(
                        hpx::detail::construct_lightweight_exception(
                            HPX_MOVE(state->exceptions)));
                }
                else
                {
                    state->set_data(HPX_MOVE(state->results));
                }
            }

            // tell the scheduler that this task has finished running
            auto const* thread_data =
                hpx::threads::get_thread_id_data(hpx::threads::get_self_id());
            if (thread_data != nullptr)
            {
                thread_data->get_scheduler_base()->do_some_work(
                    static_cast<std::size_t>(-1));
            }
        }

        // Entry point for the worker thread. It will attempt to do its local
        // work, catch any exceptions, and then call set_value or set_exception
        // on the shared state.
        void operator()() const noexcept
        {
            try
            {
                // Execute task function
                if (reverse_placement)
                {
                    // schedule chunks from the end, if needed
                    do_work<hpx::concurrency::detail::queue_end::right>();
                }
                else
                {
                    do_work<hpx::concurrency::detail::queue_end::left>();
                }
            }
            catch (std::bad_alloc const&)
            {
                state->bad_alloc_thrown = true;
            }
            catch (...)
            {
                store_exception(std::current_exception());
            }

            finish();
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    // Extend the shared state of the returned future allowing to keep alive all
    // data needed for the scheduling
    template <bool Sync, typename Result, typename Launch, typename F,
        typename Shape, typename... Ts>
    struct index_queue_bulk_state_result final
      : lcos::detail::future_data_base<std::vector<Result>>
    {
    private:
        using base_type = lcos::detail::future_data_base<std::vector<Result>>;
        using init_no_addref = base_type::init_no_addref;

        // Compute a chunk size given a number of worker threads and a total
        // number of items n.
        constexpr std::uint32_t get_chunk_size(
            std::uint32_t const n) const noexcept
        {
            std::uint32_t chunk_size = 1;
            std::uint32_t upper_bound = num_threads << 5;
            while (upper_bound < n)
            {
                chunk_size <<= 1;
                upper_bound <<= 1;
            }
            return chunk_size;
        }

        HPX_FORCEINLINE constexpr std::uint32_t wrapped_pu_num(
            std::uint32_t pu, bool const needs_wraparound) const noexcept
        {
            if (!needs_wraparound || pu < available_threads)
            {
                return pu;
            }

            while (pu >= available_threads)
            {
                pu -= available_threads;
            }
            return pu;
        }

        static hpx::threads::mask_type limit_mask(
            hpx::threads::mask_cref_type orgmask,
            std::uint32_t const num_threads)
        {
            std::uint32_t const num_cores =    //-V101
                hpx::threads::hardware_concurrency();

            auto mask = hpx::threads::mask_type();
            hpx::threads::resize(mask, num_cores);
            for (std::uint32_t i = 0, j = 0; i != num_threads && j != num_cores;
                ++j)
            {
                if (hpx::threads::test(orgmask, j))
                {
                    hpx::threads::set(mask, j);
                    ++i;
                }
            }
            return mask;
        }

        // Initialize a queue for a worker thread.
        void init_queue_depth_first(
            std::size_t const worker_thread, std::size_t const size) noexcept
        {
            auto& queue = queues[worker_thread].data_;
            auto const part_begin = (worker_thread * size) / num_threads;
            auto const part_end = (worker_thread * size + size) / num_threads;

            HPX_ASSERT_MSG(
                size <= static_cast<std::size_t>(
                            (std::numeric_limits<std::uint32_t>::max)()),
                "init_queue_depth_first: ranges larger than "
                "UINT32_MAX are not supported");

            queue.reset(static_cast<std::uint32_t>(part_begin),
                static_cast<std::uint32_t>(part_end));
        }

        void init_queue_breadth_first(
            std::size_t const worker_thread, std::size_t const size) noexcept
        {
            auto& queue = queues[worker_thread].data_;
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

        // Prepare policy to be used for spawning new HPX threads
        auto prepare_launch_policy(bool const dont_bind_to_core)
        {
            auto post_policy = policy;

            // run task on small stack (except if stackless threads should be
            // used)
            auto current_stacksize =
                hpx::execution::experimental::get_stacksize(policy);
            if (current_stacksize != threads::thread_stacksize::nostack)
            {
                post_policy = hpx::execution::experimental::with_stacksize(
                    policy, threads::thread_stacksize::small_);
            }

            if (dont_bind_to_core)
            {
                // Make sure the new task is not bound to a particular core, if
                // requested. This prevents the main thread from potentially
                // being occupied in asynchronous scenarios.
                hpx::threads::thread_priority const priority =
                    hpx::execution::experimental::get_priority(post_policy);
                if (priority == hpx::threads::thread_priority::bound)
                {
                    post_policy =
                        hpx::execution::experimental::with_priority(post_policy,
                            hpx::threads::thread_priority::initially_bound);
                }
            }

            return post_policy;
        }

        // Spawn a task which will process a number of chunks. If the queue
        // contains no chunks no task will be spawned.
        template <typename Policy, typename Task>
        void do_work_task(Policy&& post_policy,
            hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, bool const needs_wraparound,
            Task&& task_f) const
        {
            std::uint32_t const worker_thread = task_f.worker_thread;
            if (queues[worker_thread].data_.empty())
            {
                // If the queue is empty we don't spawn a task. We only signal
                // that this "task" is ready.
                task_f.finish();
                return;
            }

            // launch task on new HPX-thread
            auto hint = hpx::execution::experimental::get_hint(policy);
            if (hint.mode == hpx::threads::thread_schedule_hint_mode::none &&
                hint.hint == -1)
            {
                // apply hint if none was given
                hint.mode = hpx::threads::thread_schedule_hint_mode::thread;
                hint.hint = static_cast<std::int16_t>(
                    wrapped_pu_num(worker_thread, needs_wraparound) +
                    first_thread);

                hpx::detail::post_policy_dispatch<Launch>::call(
                    hpx::execution::experimental::with_hint(
                        HPX_FORWARD(Policy, post_policy), hint),
                    desc, pool, HPX_FORWARD(Task, task_f));
            }
            else
            {
                hpx::detail::post_policy_dispatch<Launch>::call(
                    HPX_FORWARD(Policy, post_policy), desc, pool,
                    HPX_FORWARD(Task, task_f));
            }
        }

    public:
        template <typename F_, typename... Ts_>
        index_queue_bulk_state_result(std::size_t const first_thread_,
            std::size_t const num_threads_,
            std::size_t const available_threads_, Launch l, F_&& f,
            Shape const& shape, hpx::threads::mask_cref_type mask,
            Ts_&&... ts) noexcept
          : base_type(init_no_addref{})
          , first_thread(static_cast<std::uint32_t>(first_thread_))
          , num_threads(static_cast<std::uint32_t>(num_threads_))
          , available_threads(
                (std::min) (static_cast<std::uint32_t>(available_threads_),
                    num_threads))
          , policy(HPX_MOVE(l))
          , f(HPX_FORWARD(F_, f))
          , shape(shape)
          , ts(HPX_FORWARD(Ts_, ts)...)
          , pu_mask(mask)
          , queues(num_threads)
        {
            tasks_remaining.data_.store(num_threads, std::memory_order_relaxed);
            HPX_ASSERT(hpx::threads::count(pu_mask) == available_threads);
        }

        void execute(hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool)
        {
            HPX_TRACING_MARK_EVENT("index_queue_spawning::execute");

            auto const size =
                static_cast<std::uint32_t>(hpx::util::size(shape));

            // Calculate chunk size and number of chunks
            std::uint32_t chunk_size = get_chunk_size(size);
            std::uint32_t const num_chunks =
                (size + chunk_size - 1) / chunk_size;

            // launch only as many tasks as we have chunks
            std::uint32_t const num_pus = num_threads;
            if (num_chunks < static_cast<std::uint32_t>(num_threads))
            {
                num_threads = num_chunks;
                tasks_remaining.data_ = num_chunks;
                pu_mask = limit_mask(pu_mask, num_chunks);

                available_threads = (std::min) (available_threads, num_threads);
            }

            HPX_ASSERT(hpx::threads::count(pu_mask) == available_threads);

            // thread placement
            hpx::threads::thread_schedule_hint const hint =
                hpx::execution::experimental::get_hint(policy);

            using placement = hpx::threads::thread_placement_hint;

            // Initialize the queues for all worker threads so that worker
            // threads can start stealing immediately when they start.

            bool const breadth_first_placement =
                hint.placement_mode() == placement::breadth_first ||
                hint.placement_mode() == placement::breadth_first_reverse;
            for (std::uint32_t worker_thread = 0; worker_thread != num_threads;
                ++worker_thread)
            {
                if (breadth_first_placement)
                {
                    init_queue_breadth_first(worker_thread, num_chunks);
                }
                else
                {
                    // the default for this executor is depth-first placement
                    init_queue_depth_first(worker_thread, num_chunks);
                }
            }

            // make room for generated results
            results.resize(size);

            // Spawn the worker threads for all except the local queue.
            auto local_worker_thread =
                static_cast<std::uint32_t>(hpx::get_local_worker_thread_num());
            std::uint32_t worker_thread = 0;
            bool main_thread_ok = false;

            bool const needs_wraparound =
                num_threads > available_threads || first_thread != 0;

            auto const& rp = hpx::resource::get_partitioner();
            std::size_t main_pu_num;
            if (local_worker_thread != static_cast<std::uint32_t>(-1))
            {
                main_pu_num = rp.get_pu_num(
                    wrapped_pu_num(local_worker_thread, needs_wraparound));
            }
            else
            {
                main_pu_num = hpx::threads::find_first(pu_mask);
            }

            if (num_threads == 1 || !hpx::threads::test(pu_mask, main_pu_num))
            {
                main_thread_ok = true;
                local_worker_thread = worker_thread++;
                main_pu_num = rp.get_pu_num(
                    wrapped_pu_num(local_worker_thread, needs_wraparound) +
                    first_thread);
            }

            // prepare launch policy for all new HPX threads once
            auto post_policy = prepare_launch_policy(false);

            bool const reverse_placement =
                hint.placement_mode() == placement::depth_first_reverse ||
                hint.placement_mode() == placement::breadth_first_reverse;
            bool const allow_stealing =
                !hpx::threads::do_not_share_function(hint.sharing_mode());

            for (std::uint32_t pu = 0;
                worker_thread != num_threads && pu != num_pus; ++pu)
            {
                std::size_t const pu_num = rp.get_pu_num(
                    wrapped_pu_num(pu, needs_wraparound) + first_thread);

                // The queue for the local thread is handled later inline.
                if (!main_thread_ok && pu == local_worker_thread &&
                    allow_stealing)
                {
                    // The initializing thread is expected to participate in
                    // evaluating parallel regions, but only if stealing is
                    // allowed.
                    HPX_ASSERT(hpx::threads::test(pu_mask, pu_num));
                    main_thread_ok = true;
                    local_worker_thread = worker_thread++;
                    main_pu_num = rp.get_pu_num(
                        wrapped_pu_num(local_worker_thread, needs_wraparound) +
                        first_thread);
                    continue;
                }

                // don't double-book the core that runs the main thread
                if (main_thread_ok && main_pu_num == pu_num)
                {
                    continue;
                }

                // create an HPX thread only for cores in the given PU-mask
                if (!hpx::threads::test(pu_mask, pu_num))
                {
                    continue;
                }

                // Schedule task for this worker thread
                do_work_task(post_policy, desc, pool, needs_wraparound,
                    task_function_result<index_queue_bulk_state_result>{
                        hpx::intrusive_ptr<index_queue_bulk_state_result>(this),
                        size, chunk_size, worker_thread, reverse_placement,
                        allow_stealing});

                ++worker_thread;
            }

            // there have to be as many HPX threads as there are set bits in
            // the PU-mask
            HPX_ASSERT(worker_thread == num_threads);

            // the main thread should have been associated with a queue
            if (main_thread_ok)
            {
                // Handle the queue for the local thread.
                hpx::intrusive_ptr<index_queue_bulk_state_result> this_(this);
                if constexpr (Sync)
                {
                    // execute directly, if non-async
                    task_function_result<index_queue_bulk_state_result> f{
                        HPX_MOVE(this_), size, chunk_size, local_worker_thread,
                        reverse_placement, allow_stealing};

                    if (queues[local_worker_thread].data_.empty())
                    {
                        // If the queue is empty we don't execute the task. We
                        // only signal that this "task" is ready.
                        f.finish();
                        return;
                    }

                    f();
                }
                else
                {
                    do_work_task(prepare_launch_policy(true), desc, pool,
                        needs_wraparound,
                        task_function_result<index_queue_bulk_state_result>{
                            HPX_MOVE(this_), size, chunk_size,
                            local_worker_thread, reverse_placement,
                            allow_stealing});
                }
            }
        }

        std::uint32_t first_thread;
        std::uint32_t num_threads;
        std::uint32_t available_threads;
        Launch policy;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;
        Shape shape;
        HPX_NO_UNIQUE_ADDRESS hpx::tuple<std::decay_t<Ts>...> ts;
        std::vector<Result> results;

        hpx::threads::mask_type pu_mask;
        std::vector<hpx::util::cache_aligned_data<
            hpx::concurrency::detail::non_contiguous_index_queue<>>>
            queues;
        hpx::util::cache_aligned_data<std::atomic<std::uint32_t>>
            tasks_remaining;

        std::atomic<bool> bad_alloc_thrown{false};
        hpx::exception_list exceptions;
    };

    ///////////////////////////////////////////////////////////////////////////
    // This specialization avoids creating a future for each of the scheduled
    // tasks. It also avoids an additional allocation by directly returning a
    // hpx::future.
    template <typename Launch, typename F, typename S, typename... Ts>
    auto index_queue_bulk_sync_execute_result(
        hpx::threads::thread_description const& desc,
        threads::thread_pool_base* pool, std::size_t first_thread,
        std::size_t num_threads, Launch policy, F&& f, S const& shape,
        hpx::threads::mask_cref_type mask, Ts&&... ts)
    {
        HPX_ASSERT(pool);

        using result_type = detail::bulk_function_result_t<F, S, Ts...>;

        // Don't spawn tasks if there is no work to be done
        if (hpx::util::begin(shape) != hpx::util::end(shape))
        {
            std::size_t available_threads = pool->get_active_os_thread_count();
            using shared_state = index_queue_bulk_state_result<true,
                result_type, Launch, F, S, Ts...>;
            hpx::intrusive_ptr<shared_state> p(
                new shared_state(first_thread, num_threads, available_threads,
                    HPX_MOVE(policy), HPX_FORWARD(F, f), shape, mask,
                    HPX_FORWARD(Ts, ts)...),
                false);

            p->execute(desc, pool);
            return *p->get_result();    // waits and rethrows, if needed
        }

        return std::vector<result_type>();
    }

    ////////////////////////////////////////////////////////////////////////////
    template <typename Launch, typename F, typename S, typename... Ts>
    decltype(auto) index_queue_bulk_async_execute_result(
        hpx::threads::thread_description const& desc,
        threads::thread_pool_base* pool, std::size_t first_thread,
        std::size_t num_threads, Launch policy, F&& f, S const& shape,
        hpx::threads::mask_cref_type mask, Ts&&... ts)
    {
        HPX_ASSERT(pool);

        using result_type = detail::bulk_function_result_t<F, S, Ts...>;

        // Don't spawn tasks if there is no work to be done
        if (hpx::util::begin(shape) == hpx::util::end(shape))
        {
            return hpx::make_ready_future(std::vector<result_type>());
        }

        std::size_t available_threads = pool->get_active_os_thread_count();
        using shared_state = index_queue_bulk_state_result<false, result_type,
            Launch, F, S, Ts...>;
        hpx::intrusive_ptr<shared_state> p(
            new shared_state(first_thread, num_threads, available_threads,
                HPX_MOVE(policy), HPX_FORWARD(F, f), shape, mask,
                HPX_FORWARD(Ts, ts)...),
            false);

        p->execute(desc, pool);

        return hpx::traits::future_access<
            hpx::future<std::vector<result_type>>>::create(HPX_MOVE(p));
    }
}    // namespace hpx::parallel::execution::detail

#include <hpx/config/warnings_suffix.hpp>
