//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file get_min_chunk_time.hpp
/// \page hpx::execution::experimental::get_min_chunk_time
/// \headerfile hpx/execution.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/detail/task_size_chunk_size_defaults.hpp>
#include <hpx/modules/async_base.hpp>

#include <chrono>

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    namespace detail {

        HPX_CXX_CORE_EXPORT struct get_min_chunk_time_fallback
        {
            template <typename Target, typename... Args>
            HPX_FORCEINLINE std::chrono::nanoseconds operator()(
                Target&&, Args&&...) const noexcept
            {
                return get_task_size_chunk_size_defaults().min_chunk_time;
            }
        };
    }    // namespace detail
    /// \endcond

    /// The minimal run time of a chunk for the loops of the executor: the
    /// default minimal run time of \a task_size_chunk_size (see
    /// \a hpx.task_size_chunk_size.min_chunk_time_us) unless the executor asks
    /// for another one. \a task_size_chunk_size uses it unless a minimal run
    /// time was given to the \a task_size_chunk_size object: an executor that
    /// starts a chunk with less overhead than \a parallel_executor (a task and
    /// a future for each chunk) can run smaller chunks.
    ///
    /// The executors of HPX return the value configured with
    /// \a hpx.task_size_chunk_size.<executor>.<pool size>.min_chunk_time_us,
    /// or else with \a hpx.task_size_chunk_size.<executor>.min_chunk_time_us
    /// (in microseconds), where \a <executor> is the name of the executor
    /// (\a parallel for \a parallel_executor, \a fork_join for
    /// \a fork_join_executor, \a suspending_fork_join and
    /// \a shared_suspending_fork_join for the suspending fork-join executors)
    /// and \a <pool size> the number of worker threads it runs on. If neither is configured, they return the default:
    ///
    /// \code
    ///     [hpx.task_size_chunk_size.fork_join]
    ///     min_chunk_time_us = 0.5
    ///
    ///     [hpx.task_size_chunk_size.fork_join.32]
    ///     min_chunk_time_us = 0.8
    /// \endcode
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct get_min_chunk_time_t final
      : hpx::execution::experimental::detail::query_first_tag_fallback<
            get_min_chunk_time_t, detail::get_min_chunk_time_fallback>
    {
        constexpr get_min_chunk_time_t() = default;
    } get_min_chunk_time{};
}    // namespace hpx::execution::experimental
