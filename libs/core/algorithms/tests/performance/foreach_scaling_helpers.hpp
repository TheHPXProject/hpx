//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2021-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/type_support.hpp>

#include "worker_timed.hpp"

#include <cstddef>
#include <iterator>
#include <memory>
#include <random>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
inline int delay = 1000;
inline int test_count = 100;
inline int chunk_size = 0;
inline int num_overlapping_loops = 0;
inline bool disable_stealing = false;
inline bool fast_idle_mode = false;
inline unsigned int seed = std::random_device{}();

inline std::mt19937 gen(seed);

struct disable_stealing_parameter
{
    template <typename Executor>
    void mark_begin_execution(Executor&&)
    {
        hpx::threads::add_remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::enable_stealing,
            hpx::threads::policies::scheduler_mode::enable_idle_backoff);
    }

    template <typename Executor>
    void mark_end_of_scheduling(Executor&&)
    {
        hpx::threads::remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::enable_stealing);
    }

    template <typename Executor>
    void mark_end_execution(Executor&&)
    {
        hpx::threads::add_remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::enable_idle_backoff,
            hpx::threads::policies::scheduler_mode::enable_stealing);
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    disable_stealing_parameter> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
inline void measure_plain_for(
    std::vector<std::size_t> const& data_representation)
{
    std::size_t const num = data_representation.size();

    std::size_t size = num & static_cast<std::size_t>(-4);    // -V112
    for (std::size_t i = 0; i < size; i += 4)
    {
        worker_timed(delay);
        worker_timed(delay);
        worker_timed(delay);
        worker_timed(delay);
    }
    for (/**/; size < num; ++size)
    {
        worker_timed(delay);
    }
}

inline void measure_plain_for_iter(
    std::vector<std::size_t> const& data_representation)
{
    for (auto&& v : data_representation)
    {
        HPX_UNUSED(v);
        worker_timed(delay);
    }
}

///////////////////////////////////////////////////////////////////////////////
inline void measure_sequential_foreach(
    std::vector<std::size_t> const& data_representation)
{
    if (disable_stealing)
    {
        // disable stealing from inside the algorithm
        disable_stealing_parameter dsp;

        // invoke sequential for_each
        hpx::ranges::for_each(hpx::execution::seq.with(dsp),
            data_representation, [](std::size_t) { worker_timed(delay); });
    }
    else
    {
        // invoke sequential for_each
        hpx::ranges::for_each(hpx::execution::seq, data_representation,
            [](std::size_t) { worker_timed(delay); });
    }
}

template <typename Executor>
void measure_parallel_foreach(
    std::vector<std::size_t> const& data_representation, Executor&& exec)
{
    // create executor parameters object
    hpx::execution::experimental::static_chunk_size cs(chunk_size);

    if constexpr (hpx::traits::is_scheduler_executor_v<Executor>)
    {
        if (disable_stealing)
        {
            // disable stealing from inside the algorithm
            disable_stealing_parameter dsp;

            // invoke parallel for_each
            hpx::ranges::for_each(hpx::execution::par.with(cs, dsp).on(exec),
                data_representation, [](std::size_t) { worker_timed(delay); });
        }
        else
        {
            // invoke parallel for_each
            hpx::ranges::for_each(hpx::execution::par.with(cs).on(exec),
                data_representation, [](std::size_t) { worker_timed(delay); });
        }
    }
    else
    {
        if (disable_stealing)
        {
            // disable stealing from inside the algorithm
            disable_stealing_parameter dsp;

            // invoke parallel for_each
            hpx::ranges::for_each(hpx::execution::par.with(cs, dsp).on(exec),
                data_representation, [](std::size_t) { worker_timed(delay); });
        }
        else
        {
            // invoke parallel for_each
            hpx::ranges::for_each(hpx::execution::par.with(cs).on(exec),
                data_representation, [](std::size_t) { worker_timed(delay); });
        }
    }
}

// Calls f with the loop body given the task size that comes closest to the
// delay of an iteration: nano stands for about 1 ns, small for 100 ns, medium
// for 10 us and large for 1 ms (see hpx::task_size_duration).
template <typename F>
void with_sized_foreach_body(F&& f)
{
    auto const body = [](std::size_t) { worker_timed(delay); };
    if (delay <= 10)
    {
        f(hpx::nano_task(body));
    }
    else if (delay <= 1000)
    {
        f(hpx::small_task(body));
    }
    else if (delay <= 100000)
    {
        f(hpx::medium_task(body));
    }
    else
    {
        f(hpx::large_task(body));
    }
}

// Runs the loop with the task_size_chunk_size executor parameters, which
// chunk it based on the task size of the loop body.
template <typename Executor>
void measure_parallel_foreach_task_size(
    std::vector<std::size_t> const& data_representation, Executor&& exec)
{
    hpx::execution::experimental::task_size_chunk_size const tscs;

    with_sized_foreach_body([&](auto const& body) {
        if (disable_stealing)
        {
            // disable stealing from inside the algorithm
            disable_stealing_parameter dsp;

            hpx::ranges::for_each(hpx::execution::par.with(tscs, dsp).on(exec),
                data_representation, body);
        }
        else
        {
            hpx::ranges::for_each(hpx::execution::par.with(tscs).on(exec),
                data_representation, body);
        }
    });
}

template <typename Executor>
hpx::future<void> measure_task_foreach(
    std::shared_ptr<std::vector<std::size_t>> data_representation,
    Executor&& exec)
{
    // create executor parameters object
    hpx::execution::experimental::static_chunk_size cs(chunk_size);

    if (disable_stealing)
    {
        // disable stealing from inside the algorithm
        disable_stealing_parameter dsp;

        // invoke parallel for_each
        return hpx::ranges::for_each(
            hpx::execution::par(hpx::execution::task).with(cs, dsp).on(exec),
            *data_representation, [](std::size_t) { worker_timed(delay); })
            .then([data_representation](
                      hpx::future<void>) { HPX_UNUSED(data_representation); });
    }

    // invoke parallel for_each
    return hpx::ranges::for_each(
        hpx::execution::par(hpx::execution::task).with(cs).on(exec),
        *data_representation, [](std::size_t) { worker_timed(delay); })
        .then([data_representation](
                  hpx::future<void>) { HPX_UNUSED(data_representation); });
}

///////////////////////////////////////////////////////////////////////////////
inline void measure_sequential_forloop(
    std::vector<std::size_t> const& data_representation)
{
    using iterator = std::vector<std::size_t>::const_iterator;

    if (disable_stealing)
    {
        // disable stealing from inside the algorithm
        disable_stealing_parameter dsp;

        // invoke sequential for_loop
        hpx::experimental::for_loop(hpx::execution::seq.with(dsp),
            std::begin(data_representation), std::end(data_representation),
            [](iterator) { worker_timed(delay); });
    }
    else
    {
        // invoke sequential for_loop
        hpx::experimental::for_loop(hpx::execution::seq,
            std::begin(data_representation), std::end(data_representation),
            [](iterator) { worker_timed(delay); });
    }
}

template <typename Executor>
void measure_parallel_forloop(
    std::vector<std::size_t> const& data_representation, Executor&& exec)
{
    using iterator = typename std::vector<std::size_t>::const_iterator;

    // create executor parameters object
    hpx::execution::experimental::static_chunk_size cs(chunk_size);

    if constexpr (hpx::traits::is_scheduler_executor_v<Executor>)
    {
        if (disable_stealing)
        {
            // disable stealing from inside the algorithm
            disable_stealing_parameter dsp;

            // invoke parallel for_loop
            hpx::experimental::for_loop(
                hpx::execution::par.with(cs, dsp).on(exec),
                std::begin(data_representation), std::end(data_representation),
                [](iterator) { worker_timed(delay); });
        }
        else
        {
            // invoke parallel for_loop
            hpx::experimental::for_loop(hpx::execution::par.with(cs).on(exec),
                std::begin(data_representation), std::end(data_representation),
                [](iterator) { worker_timed(delay); });
        }
    }
    else
    {
        if (disable_stealing)
        {
            // disable stealing from inside the algorithm
            disable_stealing_parameter dsp;

            // invoke parallel for_loop
            hpx::experimental::for_loop(
                hpx::execution::par.with(cs, dsp).on(exec),
                std::begin(data_representation), std::end(data_representation),
                [](iterator) { worker_timed(delay); });
        }
        else
        {
            // invoke parallel for_loop
            hpx::experimental::for_loop(hpx::execution::par.with(cs).on(exec),
                std::begin(data_representation), std::end(data_representation),
                [](iterator) { worker_timed(delay); });
        }
    }
}

template <typename Executor>
hpx::future<void> measure_task_forloop(
    std::shared_ptr<std::vector<std::size_t>> data_representation,
    Executor&& exec)
{
    using iterator = typename std::vector<std::size_t>::const_iterator;

    // create executor parameters object
    hpx::execution::experimental::static_chunk_size cs(chunk_size);

    if (disable_stealing)
    {
        // disable stealing from inside the algorithm
        disable_stealing_parameter dsp;

        // invoke parallel for_loop
        return hpx::experimental::for_loop(
            hpx::execution::par(hpx::execution::task).with(cs, dsp).on(exec),
            std::begin(*data_representation), std::end(*data_representation),
            [](iterator) { worker_timed(delay); })
            .then([data_representation](
                      hpx::future<void>) { HPX_UNUSED(data_representation); });
    }
    else
    {
        // invoke parallel for_loop
        return hpx::experimental::for_loop(
            hpx::execution::par(hpx::execution::task).with(cs).on(exec),
            std::begin(*data_representation), std::end(*data_representation),
            [](iterator) { worker_timed(delay); })
            .then([data_representation](
                      hpx::future<void>) { HPX_UNUSED(data_representation); });
    }
}

#endif
