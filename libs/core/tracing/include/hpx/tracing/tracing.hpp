//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(DOXYGEN)
/// \defgroup tracing Tracing API
/// \brief Unconditionally defined tracing and profiling abstractions.
///
/// The HPX tracing API provides a unified, zero-overhead interface for
/// annotating HPX applications. Depending on the build configuration
/// (`HPX_HAVE_TRACY`, `HPX_HAVE_ITTNOTIFY`, `HPX_HAVE_APEX`), the
/// corresponding backend is selected at compile-time. If no backend is
/// selected, all tracing calls are optimized away as `constexpr` no-ops.
///
/// \b Core \b Components:
/// - \b task_timer_data: Opaque context passed to task timers.
/// - \b scoped_task_timer: RAII object for timing execution tasks. Provides
///   `yield()`, `stop()`, and `handle_post_execution()`.
///
/// \b Regions \b & \b Events:
/// - \b region: General scope annotation.
/// - \b fiber_region: Scope annotation specifically tracking HPX user-level
///   threads (fibers) across OS threads.
/// - \b fiber_suspend_region: Records the suspend and yield phases of an HPX fiber.
/// - \b mark_event: Marks a distinct event/message in time.
/// - \b rename_region: Dynamically renames the current active region.
///
/// \b Synchronization \b & \b Threading:
/// - \b lock_context: Annotates the acquisition and release of locks.
/// - \b set_thread_name: Names the underlying OS thread for the profiler.
///
/// \b Performance \b Counters:
/// - \b create_counter: Registers a continuous performance metric.
/// - \b sample_counter: Updates a registered metric with a new double value.
#endif

#if defined(HPX_HAVE_MODULE_TRACING)

namespace hpx::threads {

    HPX_CXX_CORE_EXPORT struct thread_description;
    HPX_CXX_CORE_EXPORT struct thread_id;
}    // namespace hpx::threads

namespace hpx::util::external_timer {

    HPX_CXX_CORE_EXPORT struct task_wrapper;
}    // namespace hpx::util::external_timer

#if defined(HPX_HAVE_TRACY)
#include <hpx/tracing/backends/tracy.hpp>
#elif defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0
#include <hpx/tracing/backends/ittnotify.hpp>
#elif defined(HPX_HAVE_APEX)
#include <hpx/tracing/backends/apex.hpp>
#else
#include <hpx/tracing/backends/empty.hpp>
#endif

#endif
