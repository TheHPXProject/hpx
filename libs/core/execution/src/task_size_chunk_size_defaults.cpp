//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution/detail/task_size_chunk_size_defaults.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>

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

    void reset_task_size_chunk_size_defaults() noexcept
    {
        set_task_size_chunk_size_defaults({default_min_chunk_time,
            default_chunks_per_core, default_copy_bytes_per_ns});
    }
}    // namespace hpx::execution::experimental::detail
