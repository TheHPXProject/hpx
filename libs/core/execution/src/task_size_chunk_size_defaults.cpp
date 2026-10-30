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

        constexpr std::chrono::microseconds default_min_chunk_time(5);
        constexpr std::size_t default_chunks_per_core = 4;

        // the minimal run time of a chunk in nanoseconds
        std::atomic<std::chrono::nanoseconds::rep> min_chunk_time(
            std::chrono::nanoseconds(default_min_chunk_time).count());
        std::atomic<std::size_t> chunks_per_core(default_chunks_per_core);
    }    // namespace

    task_size_chunk_size_defaults get_task_size_chunk_size_defaults() noexcept
    {
        return {std::chrono::nanoseconds(
                    min_chunk_time.load(std::memory_order_relaxed)),
            chunks_per_core.load(std::memory_order_relaxed)};
    }

    void set_task_size_chunk_size_defaults(
        task_size_chunk_size_defaults const& defaults) noexcept
    {
        min_chunk_time.store(
            defaults.min_chunk_time.count(), std::memory_order_relaxed);
        chunks_per_core.store(
            defaults.chunks_per_core == 0 ? 1 : defaults.chunks_per_core,
            std::memory_order_relaxed);
    }

    void reset_task_size_chunk_size_defaults() noexcept
    {
        set_task_size_chunk_size_defaults(
            {default_min_chunk_time, default_chunks_per_core});
    }
}    // namespace hpx::execution::experimental::detail
