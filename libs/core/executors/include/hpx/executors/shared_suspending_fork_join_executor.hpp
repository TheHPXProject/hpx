//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file shared_suspending_fork_join_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/executors/suspending_fork_join_executor.hpp>
#include <hpx/modules/execution.hpp>

#include <type_traits>

namespace hpx::execution::experimental {

    /// A suspending_fork_join_executor that can be used from any number of
    /// threads at the same time, and from inside its own parallel regions.
    ///
    /// All shared_suspending_fork_join_executors with the same configuration
    /// (priority, stacksize, loop schedule and spin duration) use the same
    /// worker threads: one set for each thread pool, created when an HPX
    /// thread of the pool uses such an executor first, and stopped when the
    /// runtime starts to shut down. A parallel region runs on these worker
    /// threads if they are free. If they are not, because they run a region
    /// for another thread already, or because the region is started from
    /// inside one of their regions (nested parallelism), the region runs
    /// through a parallel_executor instead: one task for each worker thread
    /// of the pool, each with a contiguous range of the indices. The same
    /// happens if the calling thread is not an HPX thread, and after the
    /// worker threads were stopped. Exceptions are reported as the executor
    /// that runs the region reports them: the first exception from the
    /// worker threads, or an hpx::exception_list from the
    /// parallel_executor.
    ///
    /// Copies of the executor never hold worker threads themselves, so it
    /// can be a constexpr variable. The executor of hpx::execution::par and
    /// hpx::execution::par_unseq is a shared_suspending_fork_join_executor if
    /// HPX is configured with
    /// HPX_WITH_SUSPENDING_FORK_JOIN_EXECUTOR_AS_DEFAULT=ON. Mapping a
    /// policy with this executor to a task policy (to_task, as in
    /// par(task)) gives it a parallel_executor instead: a task policy runs
    /// an algorithm asynchronously, which the worker threads of this
    /// executor can't do.
    ///
    /// As for every suspending_fork_join_executor, the calling thread takes
    /// part in a region (see suspending_fork_join_executor), and
    /// bulk_async_execute and async_invoke run the region before they
    /// return a ready future. Single tasks (sync_execute, async_execute,
    /// then_execute, post) and continuations (bulk_then_execute) run on a
    /// parallel_executor. The properties (with_processing_units_count,
    /// with_first_core, with_priority, with_stacksize, with_hint,
    /// with_annotation) give a shared_suspending_fork_join_executor and
    /// apply to its worker threads (except for the hint) and to that
    /// parallel_executor, which also runs the regions that fall back and is
    /// the executor of the task policies. The worker threads get
    /// thread_priority::bound if the priority is thread_priority::default_
    /// (as it is by default), so that they stay on their cores, while the
    /// tasks of the parallel_executor can be stolen.
    HPX_CXX_CORE_EXPORT using shared_suspending_fork_join_executor =
        basic_suspending_fork_join_executor<true>;
}    // namespace hpx::execution::experimental

namespace hpx::execution {

    /// The algorithms that need a task for each of their parts (for instance
    /// to clean up the parts that succeeded if another part threw, as the
    /// uninitialized algorithms do, or because the parts wait for each other,
    /// as in an spmd_block) ask for an executor that spawns its tasks
    /// hierarchically. For a shared_suspending_fork_join_executor, this is
    /// its parallel_executor (see embedded_parallel_executor), with its
    /// properties.
    HPX_CXX_CORE_EXPORT inline auto to_hierarchical_spawning(
        experimental::shared_suspending_fork_join_executor const& exec)
    {
        return to_hierarchical_spawning(exec.embedded_parallel_executor());
    }

    HPX_CXX_CORE_EXPORT inline auto to_hierarchical_spawning(
        experimental::shared_suspending_fork_join_executor& exec)
    {
        return to_hierarchical_spawning(exec.embedded_parallel_executor());
    }

    HPX_CXX_CORE_EXPORT inline auto to_hierarchical_spawning(
        experimental::shared_suspending_fork_join_executor&& exec)
    {
        return to_hierarchical_spawning(exec.embedded_parallel_executor());
    }
}    // namespace hpx::execution
