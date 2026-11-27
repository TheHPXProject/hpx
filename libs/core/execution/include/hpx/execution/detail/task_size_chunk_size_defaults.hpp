//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <chrono>
#include <cstddef>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::execution::experimental::detail {

    /// \cond NOINTERNAL
    // The parameters a task_size_chunk_size uses unless it is given others.
    // They are compiled into the library, so that changing them recompiles
    // only its source file, and the runtime replaces them at startup with
    // hpx.task_size_chunk_size.min_chunk_time_us,
    // hpx.task_size_chunk_size.chunks_per_core and
    // hpx.task_size_chunk_size.copy_bytes_per_ns, if they are not empty.
    HPX_CXX_CORE_EXPORT struct task_size_chunk_size_defaults
    {
        std::chrono::nanoseconds min_chunk_time;
        std::size_t chunks_per_core;

        // The number of bytes a core copies in a nanosecond, from which the
        // algorithms that copy trivially copyable elements derive the number
        // of elements a copy handles in the time of a nano task.
        std::size_t copy_bytes_per_ns;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT task_size_chunk_size_defaults
    get_task_size_chunk_size_defaults() noexcept;

    // A chunks_per_core or copy_bytes_per_ns of 0 is taken as 1.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_task_size_chunk_size_defaults(
        task_size_chunk_size_defaults const& defaults) noexcept;

    // The rate at which a core copies data whose footprint on that core is
    // at most bytes_per_core: a core's share of a cache, or the largest
    // std::size_t for data in memory.
    HPX_CXX_CORE_EXPORT struct memory_rate
    {
        std::size_t bytes_per_core;
        std::size_t bytes_per_ns;
    };

    // The rates by footprint per core, by ascending bytes_per_core (at most
    // eight are kept); empty, copy_bytes_per_ns is the rate for every
    // footprint. The runtime sets them at startup from the cache shares of
    // the topology and hpx.task_size_chunk_size.copy_bytes_per_ns_l1 to
    // _memory. A rate of 0 is taken as 1.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_memory_rates(
        std::vector<memory_rate> const& rates) noexcept;

    // The rate at which a core copies data of the given footprint on that
    // core, in bytes per nanosecond: that of the first of the memory rates
    // the footprint fits, else copy_bytes_per_ns.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_copy_bytes_per_ns(
        std::size_t footprint_per_core) noexcept;

    // Restore the defaults compiled into the library (and no memory rates).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void
    reset_task_size_chunk_size_defaults() noexcept;
    /// \endcond
}    // namespace hpx::execution::experimental::detail
