//  Copyright (c) 2023-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread_pools/detail/scheduling_callbacks.hpp>
#include <hpx/thread_pools/detail/scheduling_counters.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace hpx::threads::detail {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
    HPX_CXX_CORE_EXPORT struct background_work_exec_time
    {
        explicit constexpr background_work_exec_time(
            scheduling_counters& counters) noexcept
          : timer(counters.background_work_duration_)
        {
        }

        std::int64_t& timer;
    };
#else
    HPX_CXX_CORE_EXPORT struct background_work_exec_time
    {
        constexpr explicit background_work_exec_time(
            scheduling_counters&) noexcept
        {
        }
    };
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Create a new background thread
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void create_background_thread(
        thread_id_ref_type& background_thread,
        threads::policies::scheduler_base& scheduler_base,
        std::size_t num_thread, scheduling_callbacks const& callbacks,
        std::shared_ptr<bool>& background_running,
        std::int64_t& idle_loop_count);

    ///////////////////////////////////////////////////////////////////////////
    // This function tries to invoke the background work thread. It returns
    // false when we need to give the background thread back to scheduler and
    // create a new one that is supposed to be executed inside the
    // scheduling_loop, true otherwise
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool call_background_thread(
        thread_id_ref_type& background_thread, thread_id_ref_type& next_thrd,
        threads::policies::scheduler_base& scheduler_base,
        std::size_t num_thread, background_work_exec_time& exec_time,
        hpx::execution_base::this_thread::detail::agent_storage*
            context_storage);

    ///////////////////////////////////////////////////////////////////////////
    // Call background thread and if that was suspended, create a new background
    // thread to be used instead. Returns if a new thread was created.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool call_and_create_background_thread(
        thread_id_ref_type& background_thread, thread_id_ref_type& next_thrd,
        threads::policies::scheduler_base& scheduler_base,
        std::size_t num_thread, background_work_exec_time& exec_time,
        hpx::execution_base::this_thread::detail::agent_storage*
            context_storage,
        scheduling_callbacks const& callbacks, std::shared_ptr<bool>& running,
        std::int64_t& idle_loop_count);

    ///////////////////////////////////////////////////////////////////////////
    // Tell the background thread to exit and run it one last time. If it is
    // still in the middle of its work afterwards (it yielded or was suspended),
    // leave it to the scheduler to run it to completion as a regular thread.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void stop_background_thread(
        thread_id_ref_type& background_thread, thread_id_ref_type& next_thrd,
        threads::policies::scheduler_base& scheduler_base,
        std::size_t num_thread, background_work_exec_time& exec_time,
        hpx::execution_base::this_thread::detail::agent_storage*
            context_storage,
        std::shared_ptr<bool>& running);
}    // namespace hpx::threads::detail
