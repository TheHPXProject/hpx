//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution/detail/task_size_chunk_size_defaults.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::execution::experimental::detail {

    namespace {

        // Chosen with the sweep in
        // libs/core/algorithms/tests/performance/task_size_chunk_size_sweep.cpp
        // 2.5 us and 8 chunks per core
        constexpr std::chrono::nanoseconds default_min_chunk_time(2500);
        constexpr std::size_t default_chunks_per_core = 8;

        // About the rate at which a core copies data that is not in its
        // caches (cached data is copied faster).
        constexpr std::size_t default_copy_bytes_per_ns = 16;

        // the minimal run time of a chunk in nanoseconds
        std::atomic<std::chrono::nanoseconds::rep> min_chunk_time(
            default_min_chunk_time.count());
        std::atomic<std::size_t> chunks_per_core(default_chunks_per_core);
        std::atomic<std::size_t> copy_bytes_per_ns(default_copy_bytes_per_ns);

        // the memory rates (set_memory_rates): the rows first, then their
        // number, which a reader loads first
        constexpr std::size_t max_memory_rates = 8;
        std::atomic<std::size_t> memory_rate_limits[max_memory_rates] = {};
        std::atomic<std::size_t> memory_rate_values[max_memory_rates] = {};
        std::atomic<std::size_t> memory_rate_count(0);

        // the minimal run times configured for single executors, written at
        // startup only
        std::vector<executor_min_chunk_time>& executor_min_chunk_times()
        {
            static std::vector<executor_min_chunk_time> times;
            return times;
        }
    }    // namespace

    task_size_chunk_size_defaults get_task_size_chunk_size_defaults() noexcept
    {
        return {std::chrono::nanoseconds(
                    min_chunk_time.load(std::memory_order_relaxed)),
            chunks_per_core.load(std::memory_order_relaxed),
            copy_bytes_per_ns.load(std::memory_order_relaxed)};
    }

    void set_task_size_chunk_size_defaults(
        task_size_chunk_size_defaults const& defaults) noexcept
    {
        min_chunk_time.store(
            defaults.min_chunk_time.count(), std::memory_order_relaxed);
        chunks_per_core.store(
            defaults.chunks_per_core == 0 ? 1 : defaults.chunks_per_core,
            std::memory_order_relaxed);
        copy_bytes_per_ns.store(
            defaults.copy_bytes_per_ns == 0 ? 1 : defaults.copy_bytes_per_ns,
            std::memory_order_relaxed);
    }

    void set_memory_rates(std::vector<memory_rate> const& rates) noexcept
    {
        std::size_t const count = (std::min) (rates.size(), max_memory_rates);
        memory_rate_count.store(0, std::memory_order_release);
        for (std::size_t i = 0; i != count; ++i)
        {
            memory_rate_limits[i].store(
                rates[i].bytes_per_core, std::memory_order_relaxed);
            memory_rate_values[i].store(
                rates[i].bytes_per_ns == 0 ? 1 : rates[i].bytes_per_ns,
                std::memory_order_relaxed);
        }
        memory_rate_count.store(count, std::memory_order_release);
    }

    std::size_t get_copy_bytes_per_ns(
        std::size_t const footprint_per_core) noexcept
    {
        std::size_t const count =
            memory_rate_count.load(std::memory_order_acquire);
        for (std::size_t i = 0; i != count; ++i)
        {
            if (footprint_per_core <=
                memory_rate_limits[i].load(std::memory_order_relaxed))
            {
                return memory_rate_values[i].load(std::memory_order_relaxed);
            }
        }
        return copy_bytes_per_ns.load(std::memory_order_relaxed);
    }

    void reset_task_size_chunk_size_defaults() noexcept
    {
        set_task_size_chunk_size_defaults({default_min_chunk_time,
            default_chunks_per_core, default_copy_bytes_per_ns});
        set_memory_rates({});
        executor_min_chunk_times().clear();
    }

    std::chrono::nanoseconds get_executor_min_chunk_time(
        std::string_view const executor, std::size_t const pool_size) noexcept
    {
        std::chrono::nanoseconds result(
            min_chunk_time.load(std::memory_order_relaxed));
        for (auto const& entry : executor_min_chunk_times())
        {
            if (entry.executor != executor)
            {
                continue;
            }
            if (entry.pool_size == pool_size)
            {
                return entry.min_chunk_time;
            }
            if (entry.pool_size == 0)
            {
                result = entry.min_chunk_time;
            }
        }
        return result;
    }

    void set_executor_min_chunk_times(
        std::vector<executor_min_chunk_time> times)
    {
        executor_min_chunk_times() = HPX_MOVE(times);
    }
}    // namespace hpx::execution::experimental::detail
